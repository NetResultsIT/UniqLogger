#!/usr/bin/env bash
set -euo pipefail

# build_qt_ios_xcframework.sh
#
# Build Qt/qmake iOS library as XCFramework:
#   - ios-arm64                  -> device            (arm64)
#   - ios-arm64_x86_64-simulator -> simulator (fat)   (arm64 + x86_64)
#
# The .pro file must distinguish the two builds using:
#
# equals(XCFRAMEWORK_SLICE, device) {
#     QMAKE_APPLE_DEVICE_ARCHS    = arm64
#     QMAKE_APPLE_SIMULATOR_ARCHS =
# } else:equals(XCFRAMEWORK_SLICE, simulator) {
#     QMAKE_APPLE_DEVICE_ARCHS    =
#     QMAKE_APPLE_SIMULATOR_ARCHS = arm64 x86_64
# } else {
#     QMAKE_APPLE_DEVICE_ARCHS    = arm64
#     QMAKE_APPLE_SIMULATOR_ARCHS = arm64 x86_64
# }

SCRIPT_NAME="$(basename "$0")"

QMAKE_PATH="${QMAKE:-}"
PROJECT_FILE=""
PROJECT_DIR=""
TARGET_NAME=""
OUTPUT_ROOT=""

LIB_TYPE="framework"

HEADERS_DIR=""
HEADERS_RELATIVE=""
HEADERS_RELATIVE_BASE="slice"
STAGE_HEADERS=1

LIBRARY_NAME=""
LIBRARY_RELATIVE=""
DEVICE_LIBRARY_RELATIVE=""
SIMULATOR_LIBRARY_RELATIVE=""

CONFIGURATION="release"
DEPLOYMENT_TARGET=""
CLEAN_BUILD=1
CREATE_DSYM=1
STATIC_DSYM=0
QMAKE_SPEC="macx-ios-clang"
SLICE_VARIABLE_NAME="XCFRAMEWORK_SLICE"

EXTRA_QMAKE_ARGS=()
EXTRA_QMAKE_ARGS_COUNT=0

usage() {
  cat <<EOF
Usage:
  ${SCRIPT_NAME} -q <qmake> -p <project.pro> -t <target> -o <output-dir> --type <static|framework> [options]

Required:
  -q, --qmake <path>          Path to Qt iOS qmake.
                              Can also be provided with QMAKE=/path/to/qmake.

  -p, --project <file.pro>    Path to the .pro file.

  -t, --target <name>         XCFramework output name.
                              Example:
                                -t unqilogger
                              produces:
                                <project>/last_build/unqilogger.xcframework

  -o, --output <dir>          Temporary output root directory used for the two builds.

  --type <static|framework>   Type produced by the .pro file.

Static library output:
  --library-relative <path>   Static library path relative to each slice build directory.
                              Example:
                                --library-relative release/bin/libUniqLogger_iOS.a

  --device-library-relative <path>
                              Device static library path relative to ios-arm64 build dir.

  --simulator-library-relative <path>
                              Simulator static library path relative to ios-simulator-arm64_x86_64 build dir.

  --library-name <name>       Static library product name to search.
                              Accepts:
                                UniqLogger_iOS
                                libUniqLogger_iOS.a

Static library headers:
  --headers <dir>             Public headers directory.
                              Can be absolute or relative to current working directory.

  --headers-relative <path>   Headers directory relative to selected base.
                              Example:
                                --headers-relative last_build/include

  --headers-relative-base <base>
                              Base used by --headers-relative:
                                slice    -> <output>/ios-arm64/<path>
                                            <output>/ios-simulator-arm64_x86_64/<path>

                                project  -> <directory-of-.pro>/<path>
                                            same headers for both slices

                                output   -> <output>/<path>
                                            same headers for both slices

                              Default:
                                slice

  --no-stage-headers          Do not copy headers into:
                                <output>/xcframework_headers/<slice>

Optional:
  -c, --configuration <name>  release or debug.
                              Default:
                                release

  --deployment-target <ver>   iOS deployment target.
                              Example:
                                --deployment-target 13.0

  --qmake-spec <spec>         qmake spec.
                              Default:
                                macx-ios-clang

  --slice-variable <name>     Variable injected into .pro.
                              Default:
                                XCFRAMEWORK_SLICE

  --no-clean                  Do not remove output directory before build.

  --no-dsym                   Do not create dSYM.
                              For static libraries this is effectively the default behavior.

  --static-dsym               Try to generate dSYM for static libraries.
                              Usually not needed.

  --                          Everything after -- is passed directly to qmake.

Example static library:

  ${SCRIPT_NAME} \\
    -q /Users/marzucca/Qt6/6.8.3/ios/bin/qmake \\
    -p /Users/marzucca/projects/UniqLogger/lib/UniqLogger.pro \\
    -t unqilogger \\
    --type static \\
    --library-relative release/bin/libUniqLogger_iOS.a \\
    --headers-relative last_build/include \\
    --headers-relative-base project \\
    -o /Users/marzucca/projects/UniqLogger/lib/build_xcframework

Example framework:

  ${SCRIPT_NAME} \\
    -q /Users/marzucca/Qt6/6.8.3/ios/bin/qmake \\
    -p /path/to/vdk.pro \\
    -t vdk \\
    --type framework \\
    -o /path/to/build/vdk

EOF
}

die() {
  echo "ERROR: $*" >&2
  exit 1
}

log() {
  echo >&2
  echo "==> $*" >&2
}

abs_path() {
  local path="$1"

  if [[ -d "$path" ]]; then
    (cd "$path" && pwd)
  else
    local dir
    local base

    dir="$(dirname "$path")"
    base="$(basename "$path")"

    (cd "$dir" && printf "%s/%s\n" "$(pwd)" "$base")
  fi
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "Command not found: $1"
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -q|--qmake)
        QMAKE_PATH="${2:-}"
        shift 2
        ;;

      -p|--project)
        PROJECT_FILE="${2:-}"
        shift 2
        ;;

      -t|--target)
        TARGET_NAME="${2:-}"
        shift 2
        ;;

      -o|--output)
        OUTPUT_ROOT="${2:-}"
        shift 2
        ;;

      --type)
        LIB_TYPE="${2:-}"
        shift 2
        ;;

      --headers)
        HEADERS_DIR="${2:-}"
        shift 2
        ;;

      --headers-relative)
        HEADERS_RELATIVE="${2:-}"
        shift 2
        ;;

      --headers-relative-base)
        HEADERS_RELATIVE_BASE="${2:-}"
        shift 2
        ;;

      --no-stage-headers)
        STAGE_HEADERS=0
        shift
        ;;

      --library-name)
        LIBRARY_NAME="${2:-}"
        shift 2
        ;;

      --library-relative)
        LIBRARY_RELATIVE="${2:-}"
        shift 2
        ;;

      --device-library-relative)
        DEVICE_LIBRARY_RELATIVE="${2:-}"
        shift 2
        ;;

      --simulator-library-relative)
        SIMULATOR_LIBRARY_RELATIVE="${2:-}"
        shift 2
        ;;

      -c|--configuration)
        CONFIGURATION="${2:-}"
        shift 2
        ;;

      --deployment-target)
        DEPLOYMENT_TARGET="${2:-}"
        shift 2
        ;;

      --qmake-spec)
        QMAKE_SPEC="${2:-}"
        shift 2
        ;;

      --slice-variable)
        SLICE_VARIABLE_NAME="${2:-}"
        shift 2
        ;;

      --no-clean)
        CLEAN_BUILD=0
        shift
        ;;

      --no-dsym)
        CREATE_DSYM=0
        shift
        ;;

      --static-dsym)
        STATIC_DSYM=1
        shift
        ;;

      -h|--help)
        usage
        exit 0
        ;;

      --)
        shift
        EXTRA_QMAKE_ARGS=("$@")
        EXTRA_QMAKE_ARGS_COUNT=$#
        break
        ;;

      *)
        die "Unknown argument: $1. Use --help."
        ;;
    esac
  done
}

validate_args() {
  [[ -n "$QMAKE_PATH" ]] || die "Missing qmake path. Use -q/--qmake or QMAKE=/path/to/qmake."
  [[ -x "$QMAKE_PATH" ]] || die "qmake not executable: $QMAKE_PATH"

  [[ -n "$PROJECT_FILE" ]] || die "Missing project file. Use -p/--project."
  [[ -f "$PROJECT_FILE" ]] || die ".pro file not found: $PROJECT_FILE"

  [[ -n "$TARGET_NAME" ]] || die "Missing target/output name. Use -t/--target."
  [[ -n "$OUTPUT_ROOT" ]] || die "Missing output directory. Use -o/--output."

  case "$LIB_TYPE" in
    static|framework)
      ;;
    *)
      die "--type must be either 'static' or 'framework'."
      ;;
  esac

  case "$CONFIGURATION" in
    release|debug)
      ;;
    *)
      die "--configuration must be either 'release' or 'debug'."
      ;;
  esac

  case "$HEADERS_RELATIVE_BASE" in
    slice|project|output)
      ;;
    *)
      die "--headers-relative-base must be one of: slice, project, output."
      ;;
  esac

  [[ -n "$SLICE_VARIABLE_NAME" ]] || die "--slice-variable cannot be empty."
}

run_qmake_build() {
  local slice="$1"
  local sdk="$2"
  local qmake_slice_value="$3"
  local build_dir="$4"

  log "Configuring ${TARGET_NAME} [${slice}]"
  log "SDK: ${sdk}"
  log "${SLICE_VARIABLE_NAME}=${qmake_slice_value}"

  mkdir -p "$build_dir"
  cd "$build_dir"

  local qmake_args=(
    "$PROJECT_FILE"
    -spec "$QMAKE_SPEC"
    "CONFIG+=${CONFIGURATION}"
    "CONFIG+=force_debug_info"
    "QMAKE_MAC_SDK=${sdk}"
    "${SLICE_VARIABLE_NAME}=${qmake_slice_value}"
  )

  if [[ -n "$DEPLOYMENT_TARGET" ]]; then
    qmake_args+=("QMAKE_IOS_DEPLOYMENT_TARGET=${DEPLOYMENT_TARGET}")
    qmake_args+=("QMAKE_MACOSX_DEPLOYMENT_TARGET=${DEPLOYMENT_TARGET}")
  fi

  if [[ "$EXTRA_QMAKE_ARGS_COUNT" -gt 0 ]]; then
    qmake_args+=("${EXTRA_QMAKE_ARGS[@]}")
  fi

  "$QMAKE_PATH" "${qmake_args[@]}"

  log "Building ${TARGET_NAME} [${slice}]"

  make -j"$(sysctl -n hw.ncpu)"
}

static_library_candidate_names() {
  local name="$1"

  if [[ "$name" == *.a ]]; then
    echo "$name"
    return 0
  fi

  echo "lib${name}.a"
  echo "${name}.a"
}

find_static_library_by_name() {
  local build_dir="$1"
  local name="$2"

  local candidate
  local libname

  while IFS= read -r libname; do
    local candidates=(
      "${build_dir}/last_build/${libname}"
      "${build_dir}/last_build/lib/${libname}"
      "${build_dir}/last_build/bin/${libname}"
      "${build_dir}/release/bin/${libname}"
      "${build_dir}/debug/bin/${libname}"
      "${build_dir}/release/${libname}"
      "${build_dir}/debug/${libname}"
      "${build_dir}/${libname}"
    )

    for candidate in "${candidates[@]}"; do
      if [[ -f "$candidate" ]]; then
        echo "$candidate"
        return 0
      fi
    done

    candidate="$(find "$build_dir" -type f -name "$libname" | head -n 1)"

    if [[ -n "$candidate" && -f "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done < <(static_library_candidate_names "$name")

  return 1
}

find_static_library_auto() {
  local build_dir="$1"

  local libs
  local count

  libs="$(find "$build_dir" -type f -name "*.a" | sort)"
  count="$(printf "%s\n" "$libs" | sed '/^$/d' | wc -l | tr -d ' ')"

  if [[ "$count" = "1" ]]; then
    printf "%s\n" "$libs"
    return 0
  fi

  if [[ "$count" = "0" ]]; then
    return 1
  fi

  echo "ERROR: multiple static libraries found under ${build_dir}:" >&2
  printf "%s\n" "$libs" >&2
  echo "Use --library-relative or --library-name to disambiguate." >&2

  return 1
}

find_static_library() {
  local build_dir="$1"
  local slice="$2"
  local relative=""

  if [[ "$slice" == "device" && -n "$DEVICE_LIBRARY_RELATIVE" ]]; then
    relative="$DEVICE_LIBRARY_RELATIVE"
  elif [[ "$slice" == "simulator" && -n "$SIMULATOR_LIBRARY_RELATIVE" ]]; then
    relative="$SIMULATOR_LIBRARY_RELATIVE"
  elif [[ -n "$LIBRARY_RELATIVE" ]]; then
    relative="$LIBRARY_RELATIVE"
  fi

  if [[ -n "$relative" ]]; then
    local explicit_path="${build_dir}/${relative}"

    if [[ -f "$explicit_path" ]]; then
      echo "$explicit_path"
      return 0
    fi

    die "Static library not found at relative path: ${explicit_path}"
  fi

  if [[ -n "$LIBRARY_NAME" ]]; then
    if find_static_library_by_name "$build_dir" "$LIBRARY_NAME"; then
      return 0
    fi

    die "Static library named '${LIBRARY_NAME}' not found under ${build_dir}."
  fi

  if find_static_library_by_name "$build_dir" "$TARGET_NAME"; then
    return 0
  fi

  if find_static_library_auto "$build_dir"; then
    return 0
  fi

  return 1
}

dir_has_headers() {
  local dir="$1"

  [[ -d "$dir" ]] || return 1

  find "$dir" -type f \( \
    -name "*.h" -o \
    -name "*.hh" -o \
    -name "*.hpp" -o \
    -name "*.hxx" \
  \) | head -n 1 | grep -q .
}

find_headers_dir_for_slice() {
  local build_dir="$1"
  local slice="$2"

  if [[ -n "$HEADERS_DIR" ]]; then
    if dir_has_headers "$HEADERS_DIR"; then
      echo "$HEADERS_DIR"
      return 0
    fi

    die "Headers not found in --headers directory: ${HEADERS_DIR}"
  fi

  if [[ -n "$HEADERS_RELATIVE" ]]; then
    local headers_path=""

    case "$HEADERS_RELATIVE_BASE" in
      slice)
        headers_path="${build_dir}/${HEADERS_RELATIVE}"
        ;;
      project)
        headers_path="${PROJECT_DIR}/${HEADERS_RELATIVE}"
        ;;
      output)
        headers_path="${OUTPUT_ROOT}/${HEADERS_RELATIVE}"
        ;;
    esac

    if dir_has_headers "$headers_path"; then
      echo "$headers_path"
      return 0
    fi

    die "Headers not found in relative path: ${headers_path}"
  fi

  local candidates=(
    "${build_dir}/last_build/include"
    "${build_dir}/last_build/headers"
    "${build_dir}/last_build"
    "${PROJECT_DIR}/last_build/include"
    "${PROJECT_DIR}/last_build/headers"
    "${PROJECT_DIR}/last_build"
    "${OUTPUT_ROOT}/last_build/include"
    "${OUTPUT_ROOT}/last_build/headers"
    "${OUTPUT_ROOT}/last_build"
    "${build_dir}/include"
    "${build_dir}/headers"
  )

  local candidate

  for candidate in "${candidates[@]}"; do
    if dir_has_headers "$candidate"; then
      echo "$candidate"
      return 0
    fi
  done

  local found

  found="$(
    find "$build_dir" "$PROJECT_DIR" "$OUTPUT_ROOT" \
      -type d \( -name include -o -name headers \) 2>/dev/null |
    while read -r d; do
      if dir_has_headers "$d"; then
        echo "$d"
        break
      fi
    done
  )"

  if [[ -n "$found" ]]; then
    echo "$found"
    return 0
  fi

  die "Public headers directory not found for ${slice}. Use --headers or --headers-relative."
}

stage_headers_for_slice() {
  local source_headers="$1"
  local slice="$2"

  if [[ "$STAGE_HEADERS" -ne 1 ]]; then
    echo "$source_headers"
    return 0
  fi

  local staged="${OUTPUT_ROOT}/xcframework_headers/${slice}"

  log "Staging headers for ${slice}: ${source_headers} -> ${staged}"

  rm -rf "$staged"
  mkdir -p "$staged"

  ditto "$source_headers" "$staged"

  if ! dir_has_headers "$staged"; then
    die "Staged headers directory is empty or invalid: ${staged}"
  fi

  echo "$staged"
}

find_framework() {
  local build_dir="$1"
  local fw1="${build_dir}/${TARGET_NAME}.framework"

  if [[ -d "$fw1" ]]; then
    echo "$fw1"
  else
    find "$build_dir" -type d -name "${TARGET_NAME}.framework" | head -n 1
  fi
}

create_dsym_for_binary() {
  local binary_path="$1"
  local dsym_path="$2"

  [[ "$CREATE_DSYM" -eq 1 ]] || return 0

  log "Creating dSYM: ${dsym_path}"

  rm -rf "$dsym_path"
  xcrun dsymutil "$binary_path" -o "$dsym_path"
}

verify_platform() {
  local binary_path="$1"
  local expected_platform_code="$2"
  local expected_platform_name="$3"

  log "Verifying Mach-O platform for ${binary_path}"
  log "Expected platform: ${expected_platform_name} (${expected_platform_code})"

  local platforms

  platforms="$(
    xcrun otool -l "$binary_path" 2>/dev/null |
      awk '
        /^[[:space:]]*platform[[:space:]]+/ {
          print $2
        }
      ' |
      sort -u
  )"

  if [[ -z "$platforms" ]]; then
    echo "WARNING: no LC_BUILD_VERSION platform found in ${binary_path}." >&2
    echo "         This can happen with some static archives or older object files." >&2
    return 0
  fi

  echo "Found platform value(s):" >&2
  echo "$platforms" >&2

  local platform

  while IFS= read -r platform; do
    [[ -z "$platform" ]] && continue

    if [[ "$platform" != "$expected_platform_code" && "$platform" != "$expected_platform_name" ]]; then
      echo "ERROR: unexpected platform in ${binary_path}" >&2
      echo "       found:    ${platform}" >&2
      echo "       expected: ${expected_platform_code} (${expected_platform_name})" >&2
      return 1
    fi
  done <<< "$platforms"

  log "Platform check OK: ${expected_platform_name}"
}

verify_architectures() {
  local binary_path="$1"
  shift

  local expected_archs=("$@")
  local actual_output

  actual_output="$(xcrun lipo -archs "$binary_path")"

  local actual_archs=()
  read -r -a actual_archs <<< "$actual_output"

  log "Verifying architectures for ${binary_path}"
  log "Expected architectures: ${expected_archs[*]}"
  log "Found architectures: ${actual_archs[*]}"

  if [[ "${#actual_archs[@]}" -ne "${#expected_archs[@]}" ]]; then
    die "Unexpected architecture count in ${binary_path}"
  fi

  local expected_arch
  local actual_arch
  local found

  for expected_arch in "${expected_archs[@]}"; do
    found=0
    for actual_arch in "${actual_archs[@]}"; do
      if [[ "$actual_arch" == "$expected_arch" ]]; then
        found=1
        break
      fi
    done

    [[ "$found" -eq 1 ]] || die "Missing architecture ${expected_arch} in ${binary_path}"
  done

  log "Architecture check OK"
}

create_xcframework_static() {
  local device_lib="$1"
  local simulator_lib="$2"
  local device_headers="$3"
  local simulator_headers="$4"
  local device_dsym="$5"
  local simulator_dsym="$6"
  local xcframework_path="$7"

  local args=(
    -create-xcframework
    -library "$device_lib"
    -headers "$device_headers"
  )

  if [[ "$CREATE_DSYM" -eq 1 && -n "$device_dsym" && -d "$device_dsym" ]]; then
    args+=(-debug-symbols "$(abs_path "$device_dsym")")
  fi

  args+=(
    -library "$simulator_lib"
    -headers "$simulator_headers"
  )

  if [[ "$CREATE_DSYM" -eq 1 && -n "$simulator_dsym" && -d "$simulator_dsym" ]]; then
    args+=(-debug-symbols "$(abs_path "$simulator_dsym")")
  fi

  args+=(-output "$xcframework_path")

  log "Creating XCFramework: ${xcframework_path}"

  rm -rf "$xcframework_path"
  xcodebuild "${args[@]}"
}

create_xcframework_framework() {
  local device_fw="$1"
  local simulator_fw="$2"
  local device_dsym="$3"
  local simulator_dsym="$4"
  local xcframework_path="$5"

  local args=(
    -create-xcframework
    -framework "$device_fw"
  )

  if [[ "$CREATE_DSYM" -eq 1 ]]; then
    args+=(-debug-symbols "$(abs_path "$device_dsym")")
  fi

  args+=(
    -framework "$simulator_fw"
  )

  if [[ "$CREATE_DSYM" -eq 1 ]]; then
    args+=(-debug-symbols "$(abs_path "$simulator_dsym")")
  fi

  args+=(-output "$xcframework_path")

  log "Creating XCFramework: ${xcframework_path}"

  rm -rf "$xcframework_path"
  xcodebuild "${args[@]}"
}

main() {
  parse_args "$@"
  validate_args

  require_cmd xcrun
  require_cmd xcodebuild
  require_cmd make
  require_cmd sysctl
  require_cmd ditto

  PROJECT_FILE="$(abs_path "$PROJECT_FILE")"
  PROJECT_DIR="$(cd "$(dirname "$PROJECT_FILE")" && pwd)"
  OUTPUT_ROOT="$(mkdir -p "$OUTPUT_ROOT" && cd "$OUTPUT_ROOT" && pwd)"

  if [[ -n "$HEADERS_DIR" ]]; then
    HEADERS_DIR="$(abs_path "$HEADERS_DIR")"
  fi

  local device_build_dir="${OUTPUT_ROOT}/ios-arm64"
  local simulator_build_dir="${OUTPUT_ROOT}/ios-simulator-arm64_x86_64"

  local xcframework_dir="${PROJECT_DIR}/last_build"
  local xcframework_path="${xcframework_dir}/${TARGET_NAME}.xcframework"

  if [[ "$CLEAN_BUILD" -eq 1 ]]; then
    log "Cleaning temporary output root: ${OUTPUT_ROOT}"

    rm -rf "$OUTPUT_ROOT"

    mkdir -p "$device_build_dir"
    mkdir -p "$simulator_build_dir"
  else
    mkdir -p "$device_build_dir"
    mkdir -p "$simulator_build_dir"
  fi

  mkdir -p "$xcframework_dir"

  log "qmake: $QMAKE_PATH"
  log ".pro: $PROJECT_FILE"
  log "project dir: $PROJECT_DIR"
  log "target/output: $TARGET_NAME"
  log "type: $LIB_TYPE"
  log "temporary output: $OUTPUT_ROOT"
  log "final XCFramework dir: $xcframework_dir"
  log "slice variable: $SLICE_VARIABLE_NAME"

  if [[ "$LIB_TYPE" == "static" ]]; then
    if [[ -n "$LIBRARY_RELATIVE" ]]; then
      log "library-relative: $LIBRARY_RELATIVE"
    elif [[ -n "$DEVICE_LIBRARY_RELATIVE" || -n "$SIMULATOR_LIBRARY_RELATIVE" ]]; then
      log "device-library-relative: ${DEVICE_LIBRARY_RELATIVE:-<auto>}"
      log "simulator-library-relative: ${SIMULATOR_LIBRARY_RELATIVE:-<auto>}"
    elif [[ -n "$LIBRARY_NAME" ]]; then
      log "library-name: $LIBRARY_NAME"
    else
      log "library: search by target, then auto-detect if exactly one .a exists"
    fi

    if [[ -n "$HEADERS_DIR" ]]; then
      log "headers: $HEADERS_DIR"
    elif [[ -n "$HEADERS_RELATIVE" ]]; then
      log "headers-relative: $HEADERS_RELATIVE"
      log "headers-relative-base: $HEADERS_RELATIVE_BASE"
    else
      log "headers: auto-detect after build"
    fi
  fi

  run_qmake_build "ios-arm64" "iphoneos" "device" "$device_build_dir"
  run_qmake_build "ios-simulator-arm64_x86_64" "iphonesimulator" "simulator" "$simulator_build_dir"

  if [[ "$LIB_TYPE" == "static" ]]; then
    local device_lib
    local simulator_lib

    device_lib="$(find_static_library "$device_build_dir" "device")"
    simulator_lib="$(find_static_library "$simulator_build_dir" "simulator")"

    [[ -n "$device_lib" && -f "$device_lib" ]] || die "Device static library not found."
    [[ -n "$simulator_lib" && -f "$simulator_lib" ]] || die "Simulator static library not found."

    local device_headers_source
    local simulator_headers_source
    local device_headers
    local simulator_headers

    device_headers_source="$(find_headers_dir_for_slice "$device_build_dir" "ios-arm64")"
    simulator_headers_source="$(find_headers_dir_for_slice "$simulator_build_dir" "ios-simulator-arm64_x86_64")"

    device_headers="$(stage_headers_for_slice "$device_headers_source" "ios-arm64")"
    simulator_headers="$(stage_headers_for_slice "$simulator_headers_source" "ios-simulator-arm64_x86_64")"

    log "Device library: ${device_lib}"
    log "Simulator library: ${simulator_lib}"
    log "Device headers: ${device_headers}"
    log "Simulator headers: ${simulator_headers}"

    local device_dsym=""
    local simulator_dsym=""

    if [[ "$CREATE_DSYM" -eq 1 && "$STATIC_DSYM" -eq 1 ]]; then
      device_dsym="${device_lib}.dSYM"
      simulator_dsym="${simulator_lib}.dSYM"

      create_dsym_for_binary "$device_lib" "$device_dsym"
      create_dsym_for_binary "$simulator_lib" "$simulator_dsym"
    else
      log "Skipping dSYM for static libraries"
      echo "Static libraries are archives of object files, not final linked Mach-O products." >&2
      echo "Keep debug info in the object files; the final app/framework dSYM will contain the linked symbols." >&2
    fi

    verify_architectures "$device_lib" arm64
    verify_architectures "$simulator_lib" arm64 x86_64
    verify_platform "$device_lib" "2" "IOS"
    verify_platform "$simulator_lib" "7" "IOSSIMULATOR"

    create_xcframework_static \
      "$device_lib" \
      "$simulator_lib" \
      "$device_headers" \
      "$simulator_headers" \
      "$device_dsym" \
      "$simulator_dsym" \
      "$xcframework_path"

  else
    local device_fw
    local simulator_fw

    device_fw="$(find_framework "$device_build_dir")"
    simulator_fw="$(find_framework "$simulator_build_dir")"

    [[ -n "$device_fw" && -d "$device_fw" ]] || die "Device framework not found for target ${TARGET_NAME}."
    [[ -n "$simulator_fw" && -d "$simulator_fw" ]] || die "Simulator framework not found for target ${TARGET_NAME}."

    local device_binary="${device_fw}/${TARGET_NAME}"
    local simulator_binary="${simulator_fw}/${TARGET_NAME}"

    [[ -f "$device_binary" ]] || die "Device framework binary not found: ${device_binary}"
    [[ -f "$simulator_binary" ]] || die "Simulator framework binary not found: ${simulator_binary}"

    local device_dsym="${device_fw}.dSYM"
    local simulator_dsym="${simulator_fw}.dSYM"

    create_dsym_for_binary "$device_binary" "$device_dsym"
    create_dsym_for_binary "$simulator_binary" "$simulator_dsym"

    verify_architectures "$device_binary" arm64
    verify_architectures "$simulator_binary" arm64 x86_64
    verify_platform "$device_binary" "2" "IOS"
    verify_platform "$simulator_binary" "7" "IOSSIMULATOR"

    create_xcframework_framework \
      "$device_fw" \
      "$simulator_fw" \
      "$device_dsym" \
      "$simulator_dsym" \
      "$xcframework_path"
  fi

  log "Done"

  echo "XCFramework:"
  echo "  ${xcframework_path}"
}

main "$@"
