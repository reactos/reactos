/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 intriniscs
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _tag_ARM64INTR_BARRIER_TYPE
{
    _ARM64_BARRIER_SY     = 0xF,
    _ARM64_BARRIER_ST     = 0xE,
    _ARM64_BARRIER_LD     = 0xD,
    _ARM64_BARRIER_ISH    = 0xB,
    _ARM64_BARRIER_ISHST  = 0xA,
    _ARM64_BARRIER_ISHLD  = 0x9,
    _ARM64_BARRIER_NSH    = 0x7,
    _ARM64_BARRIER_NSHST  = 0x6,
    _ARM64_BARRIER_NSHLD  = 0x5,
    _ARM64_BARRIER_OSH    = 0x3,
    _ARM64_BARRIER_OSHST  = 0x2,
    _ARM64_BARRIER_OSHLD  = 0x1
} _ARM64INTR_BARRIER_TYPE;

void __dmb(unsigned int _Type);
void __dsb(unsigned int _Type);
void __isb(unsigned int _Type);
void __wfe(void);
void __wfi(void);
void __sev(void);
void __sevl(void);
void __yield(void);
void __cdecl __prefetch(const void *_Address);
#if !defined(__clang__)
void __cdecl __prefetch2(const void *_Address, unsigned char _PrfOp);
#endif

#pragma intrinsic(__dmb)
#pragma intrinsic(__dsb)
#pragma intrinsic(__isb)
#pragma intrinsic(__wfe)
#pragma intrinsic(__wfi)
#pragma intrinsic(__sev)
#pragma intrinsic(__sevl)
#pragma intrinsic(__yield)
#pragma intrinsic(__prefetch)
#if !defined(__clang__)
#pragma intrinsic(__prefetch2)
#endif

#if defined(__clang__)
/* clang has no _disable or _enable builtin for ARM64, the later prototypes in intrin.h keep this linkage */
static __inline void _disable(void) { __asm__ __volatile__("msr daifset, #2" : : : "memory"); }
static __inline void _enable(void) { __asm__ __volatile__("msr daifclr, #2" : : : "memory"); }
/* nor __prefetch2, the prfm operation is only a hint so a read prefetch stands in */
#define __prefetch2(_Address, _PrfOp) __builtin_prefetch((_Address), 0, 3)
#endif

#if defined(_MSC_VER)

/* System register number as encoded in mrs and msr, op0 is always 2 or 3 */
#define ARM64_SYSREG(op0, op1, crn, crm, op2) \
    ((((op0) & 1) << 14) | (((op1) & 7) << 11) | (((crn) & 15) << 7) | (((crm) & 15) << 3) | ((op2) & 7))

#define ARM64_FPCR ARM64_SYSREG(3, 3, 4, 4, 0)
#define ARM64_FPSR ARM64_SYSREG(3, 3, 4, 4, 1)
#define ARM64_CNTVCT ARM64_SYSREG(3, 3, 14, 0, 2)

__int64 _ReadStatusReg(int _Register);
void _WriteStatusReg(int _Register, __int64 _Value);

#pragma intrinsic(_ReadStatusReg)
#pragma intrinsic(_WriteStatusReg)

#endif /* _MSC_VER */

#if defined(__cplusplus)
} // extern "C"
#endif
