# Building UniqLogger for mobile platforms

Read this from skill `uniqlogger-build` when the target is iOS or Android. Both need Qt 6 (Qt 5 stops the
`.pro` with an error). Commands below are the ones a Qt Creator kit generates, checked against existing Makefiles.

## iOS
- The library is static (`libUniqLogger_iOS.a`, `libUniqLogger_iOS_debug.a` for debug).
- Plain qmake build, device kit:
  `$QT_ROOT/macos/bin/qmake6 -o Makefile ../../uniqlogger.pro -qtconf $QT_ROOT/ios/bin/target_qt.conf
  -spec macx-ios-clang CONFIG+=release CONFIG+=iphoneos CONFIG+=device`
  The default build is device arm64 plus simulator x86_64; it has no arm64 simulator slice.
- XCFramework (device arm64 plus simulator arm64 and x86_64): run
  `QT_ROOT=$QT_ROOT lib/build_ios_xcframework.sh [release|debug]`. It needs Xcode (`xcodebuild`, `lipo`) and both
  `$QT_ROOT/macos` and `$QT_ROOT/ios`. It builds two slices in `lib/xcframework_build` (not git-ignored: delete
  it yourself when you created it, never commit it) and writes `lib/last_build/UniqLogger.xcframework`
  with the public headers. Its header list repeats `INCLUDE_HEADERS` of the `.pro`: keep the two in step.
- The script passes `XCFRAMEWORK_SLICE` and `IOS_NOT_MOVE` to qmake; do not set them by hand otherwise.

## Android
- Needs Qt 6.8.4 or newer and the Android NDK configured for the Qt kit (not verified here); ABIs arm64-v8a and
  x86_64 get the 16 KB page size linker flag; armeabi and x86 stop the build, and armeabi-v7a is not blocked by the
  `.pro`.
- Command a Qt Creator kit generates, in a build folder:
  `$QT_ROOT/macos/bin/qmake6 -o Makefile ../../uniqlogger.pro -qtconf $QT_ROOT/android_arm64_v8a/bin/target_qt.conf
  -spec android-clang`
  then `make`. The library is `libUniqLogger[_debug]_<abi>.so`.
- `AndroidWriter` is built automatically (`ENABLE_UNQL_ANDROIDLOG`).
