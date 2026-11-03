/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM NEON intrinsics
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#include <arm64_neon.h>
#else
/* GCC and Clang ship their own */
#include_next <arm_neon.h>
#endif
