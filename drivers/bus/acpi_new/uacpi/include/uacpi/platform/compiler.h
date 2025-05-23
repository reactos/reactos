#pragma once

/*
 * Compiler-specific attributes/macros go here. This is the default placeholder
 * that should work for MSVC/GCC/clang.
 */

#ifdef UACPI_OVERRIDE_COMPILER
#include "uacpi_compiler.h"
#else

#define UACPI_ALIGN(x) __declspec(align(x))

#ifdef _MSC_VER
    #include <intrin.h>

    #define UACPI_ALWAYS_INLINE __forceinline

    #define UACPI_PACKED(decl)  \
        __pragma(pack(push, 1)) \
        decl;                   \
        __pragma(pack(pop))
#else
    #define UACPI_ALWAYS_INLINE inline __attribute__((always_inline))
    #define UACPI_PACKED(decl) decl __attribute__((packed));
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define uacpi_unlikely(expr) __builtin_expect(!!(expr), 0)
    #define uacpi_likely(expr)   __builtin_expect(!!(expr), 1)

    #if __has_attribute(__fallthrough__)
        #define UACPI_FALLTHROUGH __attribute__((__fallthrough__))
    #endif

    #define UACPI_MAYBE_UNUSED __attribute__ ((unused))

    #define UACPI_NO_UNUSED_PARAMETER_WARNINGS_BEGIN             \
        _Pragma("GCC diagnostic push")                           \
        _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")

    #define UACPI_NO_UNUSED_PARAMETER_WARNINGS_END \
        _Pragma("GCC diagnostic pop")

    #ifdef __clang__
        #define UACPI_PRINTF_DECL(fmt_idx, args_idx) \
            __attribute__((format(printf, fmt_idx, args_idx)))
    #else
        #define UACPI_PRINTF_DECL(fmt_idx, args_idx) \
            __attribute__((format(gnu_printf, fmt_idx, args_idx)))
    #endif

    #define UACPI_COMPILER_HAS_BUILTIN_MEMCPY
    #define UACPI_COMPILER_HAS_BUILTIN_MEMMOVE
    #define UACPI_COMPILER_HAS_BUILTIN_MEMSET
    #define UACPI_COMPILER_HAS_BUILTIN_MEMCMP
#else
    #define uacpi_unlikely(expr) expr
    #define uacpi_likely(expr)   expr

    #define UACPI_MAYBE_UNUSED

    #define UACPI_NO_UNUSED_PARAMETER_WARNINGS_BEGIN
    #define UACPI_NO_UNUSED_PARAMETER_WARNINGS_END

    #define UACPI_PRINTF_DECL(fmt_idx, args_idx)
#endif

#ifndef UACPI_FALLTHROUGH
    #define UACPI_FALLTHROUGH do {} while (0)
#endif

#ifndef UACPI_POINTER_SIZE
    #ifdef _WIN32
        #ifdef _WIN64
            #define UACPI_POINTER_SIZE 8
        #else
            #define UACPI_POINTER_SIZE 4
        #endif
    #elif defined(__GNUC__)
        #define UACPI_POINTER_SIZE __SIZEOF_POINTER__
    #else
        #error Failed to detect pointer size
    #endif
#endif

#endif
