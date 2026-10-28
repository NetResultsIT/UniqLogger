#!/bin/bash
#
# Builds UniqLogger as an XCFramework for iOS (Qt 6).
#
# Produces:
#   last_build/UniqLogger.xcframework
#     ├── ios-arm64/              (device – iPhoneOS SDK)
#     ├── ios-arm64_x86_64-simulator/ (simulator – iPhoneSimulator SDK)
#     └── Headers/                (public headers, shared)
#
# Prerequisites:
#   - Xcode installed (xcodebuild, clang, libtool in PATH)
#   - Qt 6.x for iOS and macOS installed side-by-side
#
# Usage:
#   QT_ROOT=/path/to/Qt/6.x.x  ./build_ios_xcframework.sh [release|debug]
#
# Example:
#   QT_ROOT=~/Qt/6.8.6 ./build_ios_xcframework.sh release

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_TYPE="${1:-release}"

# ---- Validate arguments -------------------------------------------------- #

if [[ "$BUILD_TYPE" != "release" && "$BUILD_TYPE" != "debug" ]]; then
    echo "Usage: QT_ROOT=/path/to/Qt/6.x.x $0 [release|debug]"
    exit 1
fi

if [[ -z "${QT_ROOT:-}" ]]; then
    echo "Error: QT_ROOT is not set."
    echo "  Set QT_ROOT to your Qt version directory."
    echo "  Example: QT_ROOT=~/Qt/6.8.6 $0"
    exit 1
fi

QMAKE="${QT_ROOT}/macos/bin/qmake6"
QTCONF="${QT_ROOT}/ios/bin/target_qt.conf"

if [[ ! -x "$QMAKE" ]]; then
    echo "Error: qmake6 not found at $QMAKE"
    exit 1
fi
if [[ ! -f "$QTCONF" ]]; then
    echo "Error: iOS target_qt.conf not found at $QTCONF"
    exit 1
fi

# ---- Path layout --------------------------------------------------------- #

if [[ "$BUILD_TYPE" == "debug" ]]; then
    LIB_BASENAME="libUniqLogger_iOS_debug.a"
else
    LIB_BASENAME="libUniqLogger_iOS.a"
fi

BUILD_ROOT="${SCRIPT_DIR}/xcframework_build"
DEVICE_DIR="${BUILD_ROOT}/device"
SIMULATOR_DIR="${BUILD_ROOT}/simulator"
HEADERS_DIR="${BUILD_ROOT}/headers"
OUTPUT_DIR="${SCRIPT_DIR}/last_build"
XCFRAMEWORK="${OUTPUT_DIR}/UniqLogger.xcframework"

DEVICE_LIB="${DEVICE_DIR}/${BUILD_TYPE}/bin/${LIB_BASENAME}"
SIMULATOR_LIB="${SIMULATOR_DIR}/${BUILD_TYPE}/bin/${LIB_BASENAME}"

NPROC=$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

echo "========================================================"
echo "  UniqLogger iOS XCFramework Builder"
echo "========================================================"
echo "  Qt root   : $QT_ROOT"
echo "  Build type: $BUILD_TYPE"
echo "  Output    : $XCFRAMEWORK"
echo "========================================================"

# ---- Step 1: Device slice (arm64, iPhoneOS) ------------------------------ #

echo ""
echo "[1/4] Building device slice (arm64 · iPhoneOS)..."
rm -rf "$DEVICE_DIR"
mkdir -p "$DEVICE_DIR"
pushd "$DEVICE_DIR" > /dev/null

"$QMAKE" "${SCRIPT_DIR}/uniqlogger.pro" \
    -qtconf "$QTCONF" \
    -spec macx-ios-clang \
    "CONFIG+=${BUILD_TYPE}" \
    "CONFIG+=iphoneos" \
    "CONFIG+=device" \
    "XCFRAMEWORK_SLICE=device" \
    "IOS_NOT_MOVE=1"

make -j"$NPROC" "$BUILD_TYPE"
popd > /dev/null

if [[ ! -f "$DEVICE_LIB" ]]; then
    echo "Error: device library not found at $DEVICE_LIB"
    exit 1
fi
lipo -info "$DEVICE_LIB"

# ---- Step 2: Simulator slice (arm64 + x86_64, iPhoneSimulator) ----------- #

echo ""
echo "[2/4] Building simulator slice (arm64 + x86_64 · iPhoneSimulator)..."
rm -rf "$SIMULATOR_DIR"
mkdir -p "$SIMULATOR_DIR"
pushd "$SIMULATOR_DIR" > /dev/null

"$QMAKE" "${SCRIPT_DIR}/uniqlogger.pro" \
    -qtconf "$QTCONF" \
    -spec macx-ios-clang \
    "CONFIG+=${BUILD_TYPE}" \
    "CONFIG+=iphonesimulator" \
    "CONFIG+=simulator" \
    "XCFRAMEWORK_SLICE=simulator" \
    "IOS_NOT_MOVE=1"

make -j"$NPROC" "$BUILD_TYPE"
popd > /dev/null

if [[ ! -f "$SIMULATOR_LIB" ]]; then
    echo "Error: simulator library not found at $SIMULATOR_LIB"
    exit 1
fi
lipo -info "$SIMULATOR_LIB"

# ---- Step 3: Collect public headers -------------------------------------- #

echo ""
echo "[3/4] Collecting public headers..."
rm -rf "$HEADERS_DIR"
mkdir -p "$HEADERS_DIR"

PUBLIC_HEADERS=(
    "${SCRIPT_DIR}/src/UniqLogger.h"
    "${SCRIPT_DIR}/src/Logger.h"
    "${SCRIPT_DIR}/src/LogWriter.h"
    "${SCRIPT_DIR}/src/LogMessage.h"
    "${SCRIPT_DIR}/src/unqlog_common.h"
    "${SCRIPT_DIR}/src/bufferofstrings.h"
    "${SCRIPT_DIR}/src/ConsoleColorScheme.h"
    "${SCRIPT_DIR}/src/WriterConfig.h"
)

for h in "${PUBLIC_HEADERS[@]}"; do
    if [[ -f "$h" ]]; then
        cp -f "$h" "$HEADERS_DIR/"
    else
        echo "  Warning: header not found: $h"
    fi
done

# ---- Step 4: Assemble the XCFramework ------------------------------------ #

echo ""
echo "[4/4] Creating XCFramework..."
mkdir -p "$OUTPUT_DIR"
rm -rf "$XCFRAMEWORK"

xcodebuild -create-xcframework \
    -library "$DEVICE_LIB"    -headers "$HEADERS_DIR" \
    -library "$SIMULATOR_LIB" -headers "$HEADERS_DIR" \
    -output  "$XCFRAMEWORK"

echo ""
echo "========================================================"
echo "  Done!"
echo "  $XCFRAMEWORK"
echo "========================================================"
