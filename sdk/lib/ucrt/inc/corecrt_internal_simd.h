//
// corecrt_internal_simd.h
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This internal header defines internal SIMD utilities.  This header may only
// be included in C++ translation units.
//
#pragma once

#include <intrin.h>
#include <isa_availability.h>
#include <stdint.h>

#if (defined _M_IX86 || defined _M_X64) && !defined(_M_HYBRID_X86_ARM64) && !defined(_M_ARM64EC)
    #define _CRT_SIMD_SUPPORT_AVAILABLE
#endif

#if defined _CRT_SIMD_SUPPORT_AVAILABLE

#if defined(__clang__)
#define _UCRT_ENABLE_EXTENDED_ISA \
    _Pragma("clang attribute push(__attribute__((target(\"sse2,avx,avx2\"))), apply_to=function)")
#define _UCRT_RESTORE_DEFAULT_ISA \
    _Pragma("clang attribute pop")
#elif defined(__GNUC__)
#define _UCRT_ENABLE_EXTENDED_ISA \
    _Pragma("GCC push_options") \
    _Pragma("GCC target(\"avx2\")")
#define _UCRT_RESTORE_DEFAULT_ISA \
    _Pragma("GCC pop_options")
#else
#define _UCRT_ENABLE_EXTENDED_ISA
#define _UCRT_RESTORE_DEFAULT_ISA
#endif

_UCRT_ENABLE_EXTENDED_ISA

    extern "C" int __isa_available;

    enum class __crt_simd_isa
    {
        sse2,
        avx2
    };

    template <__crt_simd_isa Isa>
    struct __crt_simd_cleanup_guard;

    template <__crt_simd_isa Isa>
    struct __crt_simd_pack_traits;

    template <__crt_simd_isa Isa, typename Element>
    struct __crt_simd_traits;



    template <__crt_simd_isa Isa, typename Element>
    struct __crt_simd_element_traits
        : __crt_simd_pack_traits<Isa>
    {
        using element_type = Element;
        using __crt_simd_pack_traits<Isa>::pack_size;

        enum : size_t
        {
            element_size      = sizeof(element_type),
            elements_per_pack = pack_size / element_size
        };
    };



    template <>
    struct __crt_simd_cleanup_guard<__crt_simd_isa::sse2>
    {
        // No cleanup required for SSE2 usage, however we still need to define
        // the no-op destructor in order to avoid unreferened local variable
        // warnings when this cleanup guard is used.
        ~__crt_simd_cleanup_guard() throw()
        {
        }
    };

    template <>
    struct __crt_simd_pack_traits<__crt_simd_isa::sse2>
    {
        using pack_type = __m128i;

        enum : size_t { pack_size = sizeof(pack_type) };

        static __forceinline pack_type get_zero_pack() throw()
        {
            return _mm_setzero_si128();
        }

        static __forceinline int compute_byte_mask(pack_type const x) throw()
        {
            return _mm_movemask_epi8(x);
        }
    };

    template <>
    struct __crt_simd_traits<__crt_simd_isa::sse2, uint8_t>
        : __crt_simd_element_traits<__crt_simd_isa::sse2, uint8_t>
    {
        static __forceinline pack_type compare_equals(pack_type const x, pack_type const y) throw()
        {
            return _mm_cmpeq_epi8(x, y);
        }
    };

    template <>
    struct __crt_simd_traits<__crt_simd_isa::sse2, uint16_t>
        : __crt_simd_element_traits<__crt_simd_isa::sse2, uint16_t>
    {
        static __forceinline pack_type compare_equals(pack_type const x, pack_type const y) throw()
        {
            return _mm_cmpeq_epi16(x, y);
        }
    };



    template <>
    struct __crt_simd_cleanup_guard<__crt_simd_isa::avx2>
    {
        ~__crt_simd_cleanup_guard()
        {
            // After executing AVX2 instructions, we must zero the upper halves
            // of the YMM registers before returning.  See the Intel article
            // "Intel AVX State Transitions: Migrating SSE Code to AVX" for
            // further details.
            _mm256_zeroupper();
        }
    };

    template <>
    struct __crt_simd_pack_traits<__crt_simd_isa::avx2>
    {
        using pack_type = __m256i;

        enum : size_t { pack_size = sizeof(pack_type) };

        static __forceinline pack_type get_zero_pack() throw()
        {
            return _mm256_setzero_si256();
        }

        static __forceinline int compute_byte_mask(pack_type const x) throw()
        {
            return _mm256_movemask_epi8(x);
        }
    };

    template <>
    struct __crt_simd_traits<__crt_simd_isa::avx2, uint8_t>
        : __crt_simd_element_traits<__crt_simd_isa::avx2, uint8_t>
    {
        static __forceinline pack_type compare_equals(pack_type const x, pack_type const y) throw()
        {
            return _mm256_cmpeq_epi8(x, y);
        }
    };

    template <>
    struct __crt_simd_traits<__crt_simd_isa::avx2, uint16_t>
        : __crt_simd_element_traits<__crt_simd_isa::avx2, uint16_t>
    {
        static __forceinline pack_type compare_equals(pack_type const x, pack_type const y) throw()
        {
            return _mm256_cmpeq_epi16(x, y);
        }
    };

_UCRT_RESTORE_DEFAULT_ISA

#endif // _CRT_SIMD_SUPPORT_AVAILABLE
