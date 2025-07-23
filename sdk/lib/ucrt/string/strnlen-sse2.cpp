//
// strnlen-sse2.cpp
//
//      Copyright (c) Timo Kreuzer
//
// Explicit template instantiations for SSE2 str(n)len code
//

#pragma GCC target("sse2")
#define _UCRT_BUILD_SSE2
#include "strnlen.cpp"

template
size_t __cdecl common_strnlen_simd<bounded, __crt_simd_isa::sse2, uint8_t>(
    uint8_t const* const string,
    size_t         const maximum_count
    ) throw();

template
size_t __cdecl common_strnlen_simd<bounded, __crt_simd_isa::sse2, uint16_t>(
    uint16_t const* const string,
    size_t         const maximum_count
    ) throw();

template
size_t __cdecl common_strnlen_simd<unbounded, __crt_simd_isa::sse2, uint16_t>(
    uint16_t const* const string,
    size_t         const maximum_count
    ) throw();
