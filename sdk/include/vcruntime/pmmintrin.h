/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Intrinsics for the SSE3 instruction set
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#define _INCLUDED_PMM

/* When building with Clang, use Clang's own intrinsics headers instead. */
#if defined(__clang__) && !defined(_MSC_VER)
#include_next <pmmintrin.h>
#else

#include <emmintrin.h>

#define _MM_DENORMALS_ZERO_MASK 0x0040
#define _MM_DENORMALS_ZERO_ON   0x0040
#define _MM_DENORMALS_ZERO_OFF  0x0000

#endif /* !(__clang__ && !_MSC_VER) */
