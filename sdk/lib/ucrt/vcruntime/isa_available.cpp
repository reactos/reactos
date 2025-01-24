//
// isa_available.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __isa_available_init.
//
// SPDX-License-Identifier: MIT
//

#include <isa_availability.h>
#include <intrin.h>
#include <windef.h>
#include <winbase.h>

extern "C" { int __isa_available = 0; }

extern "C"
int
__cdecl
__isa_available_init(void)
{
#if defined(_M_IX86) || defined(_M_X64)
    if (IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE))
    {
        __isa_available = __ISA_AVAILABLE_AVX512;
    }
    else if (IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE))
    {
        __isa_available = __ISA_AVAILABLE_AVX2;
    }
    else if (IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE))
    {
        __isa_available = __ISA_AVAILABLE_AVX;
    }
    else if (IsProcessorFeaturePresent(PF_SSE4_2_INSTRUCTIONS_AVAILABLE))
    {
        __isa_available = __ISA_AVAILABLE_SSE42;
    }
    else if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
    {
        __isa_available = __ISA_AVAILABLE_SSE2;
    }
    else
    {
        __isa_available = __ISA_AVAILABLE_X86;
    }
#elif defined(_M_ARM) || defined(_M_ARM64)
    // CHECKME: Is this correct?
    if (IsProcessorFeaturePresent(PF_ARM_V8_INSTRUCTIONS_AVAILABLE))
    {
#ifdef _M_ARM64
        __isa_available = __ISA_AVAILABLE_NEON_ARM64;
#else
        __isa_available = __ISA_AVAILABLE_NEON;
#endif
    }
    else
    {
        __isa_available = __ISA_AVAILABLE_ARMNT;
    }
#endif

    return 0;
}
