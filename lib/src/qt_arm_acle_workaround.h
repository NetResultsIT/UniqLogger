/*
 * Copyright (C) 2026 Kalliope S.r.l.
 */

#pragma once

/*
    Qt 6.x / Apple Clang workaround.

    Some Qt 6 versions call __yield() from QtCore/qyieldcpu.h without a
    visible declaration. With recent Apple Clang versions this can fail with:

        error: implicitly declaring library function '__yield'

    Do NOT force-include <arm_acle.h> directly: on non-ACLE targets, for example
    x86_64 simulator or unsupported compiler contexts, Apple's header can fail
    with:

        error: "ACLE intrinsics support not enabled."

    This wrapper is intentionally defensive:
      - it only does something on ARM/AArch64 targets;
      - it includes <arm_acle.h> only when ACLE is reported as enabled;
      - otherwise it provides __yield() through the Clang builtin when available.
*/

#if defined(__arm64__) || defined(__aarch64__) || defined(__ARM_ARCH)

#  if defined(__has_include)
#    if defined(__ARM_ACLE) && __has_include(<arm_acle.h>)
#      include <arm_acle.h>
#    endif
#  endif

#  if !defined(__yield)
#    if defined(__has_builtin)
#      if __has_builtin(__builtin_arm_yield)
#        define __yield() __builtin_arm_yield()
#      endif
#    endif
#  endif

#endif
