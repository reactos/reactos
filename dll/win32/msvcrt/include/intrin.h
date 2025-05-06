/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _INC_INTRIN
#define _INC_INTRIN

#if defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__))
# include <x86intrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#if defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__))

#if __has_builtin(__cpuidex) || (defined(_MSC_VER) && !defined(__clang__))
void __cpuidex(int info[4], int ax, int cx);
#pragma intrinsic(__cpuidex)
#else
static inline void __cpuidex(int info[4], int ax, int cx)
{
  __asm__ ("cpuid" : "=a"(info[0]), "=b" (info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(ax), "c"(cx));
}
#endif

#if __has_builtin(__cpuid) || (defined(_MSC_VER) && !defined(__clang__))
void __cpuid(int info[4], int ax);
#pragma intrinsic(__cpuid)
#else
static inline void __cpuid(int info[4], int ax)
{
    return __cpuidex(info, ax, 0);
}
#endif

#endif

#if defined(__aarch64__) || defined(__arm64ec__)
typedef enum _tag_ARM64INTR_BARRIER_TYPE
{
    _ARM64_BARRIER_OSHLD  = 0x1,
    _ARM64_BARRIER_OSHST  = 0x2,
    _ARM64_BARRIER_OSH    = 0x3,
    _ARM64_BARRIER_NSHLD  = 0x5,
    _ARM64_BARRIER_NSHST  = 0x6,
    _ARM64_BARRIER_NSH    = 0x7,
    _ARM64_BARRIER_ISHLD  = 0x9,
    _ARM64_BARRIER_ISHST  = 0xa,
    _ARM64_BARRIER_ISH    = 0xb,
    _ARM64_BARRIER_LD     = 0xd,
    _ARM64_BARRIER_ST     = 0xe,
    _ARM64_BARRIER_SY     = 0xf
} _ARM64INTR_BARRIER_TYPE;
#endif

#ifdef __arm__
typedef enum _tag_ARMINTR_BARRIER_TYPE
{
    _ARM_BARRIER_OSHST  = 0x2,
    _ARM_BARRIER_OSH    = 0x3,
    _ARM_BARRIER_NSHST  = 0x6,
    _ARM_BARRIER_NSH    = 0x7,
    _ARM_BARRIER_ISHST  = 0xa,
    _ARM_BARRIER_ISH    = 0xb,
    _ARM_BARRIER_ST     = 0xe,
    _ARM_BARRIER_SY     = 0xf
} _ARMINTR_BARRIER_TYPE;
#endif

#if defined(_MSC_VER) && (defined(__arm__) || defined(__aarch64__) || defined(__arm64ec__))

void __dmb(unsigned int);

#pragma intrinsic(__dmb)

#endif

#if defined(_MSC_VER) && (defined(__aarch64__) || defined(__arm64ec__))

unsigned __int64 __getReg(int);
#pragma intrinsic(__getReg)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _INC_INTRIN */
