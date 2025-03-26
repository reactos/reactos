/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CRT - ISA availability
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#define __ISA_AVAILABILITY__H__

enum ISA_AVAILABILITY
{
    __ISA_AVAILABLE_X86   = 0,
    __ISA_AVAILABLE_SSE2  = 1,
    __ISA_AVAILABLE_SSE42 = 2,
    __ISA_AVAILABLE_AVX   = 3,
    __ISA_AVAILABLE_ENFSTRG = 4,
    __ISA_AVAILABLE_AVX2 = 5,
    __ISA_AVAILABLE_AVX512 = 6,

    __ISA_AVAILABLE_ARMNT   = 0,
    __ISA_AVAILABLE_NEON    = 1,
    __ISA_AVAILABLE_NEON_ARM64 = 2,
};

#if defined(_M_IX86)
#define __FAVOR_ATOM    0
#define __FAVOR_ENFSTRG 1
#elif defined(_M_X64)
#define __FAVOR_ATOM    0
#define __FAVOR_ENFSTRG 1
#endif
