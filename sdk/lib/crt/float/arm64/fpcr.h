/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 FPCR and FPSR access for the floating point control routines
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <float.h>
#include <intrin.h>

/* Cumulative exception flags in FPSR, the trap enables sit 8 bits higher in FPCR */
#define ARM64_FP_INVALID        0x01
#define ARM64_FP_ZERODIVIDE     0x02
#define ARM64_FP_OVERFLOW       0x04
#define ARM64_FP_UNDERFLOW      0x08
#define ARM64_FP_INEXACT        0x10
#define ARM64_FP_DENORMAL       0x80
#define ARM64_FP_ALL            0x9F
#define ARM64_FPCR_ENABLE_SHIFT 8

/* FPCR rounding mode and flush to zero */
#define ARM64_FPCR_RMODE_MASK   0x00C00000
#define ARM64_FPCR_RMODE_UP     0x00400000
#define ARM64_FPCR_RMODE_DOWN   0x00800000
#define ARM64_FPCR_RMODE_CHOP   0x00C00000
#define ARM64_FPCR_FZ           0x01000000

/* The control word bits ARM64 has a meaning for */
#define ARM64_CW_MASK           (_MCW_EM | _MCW_RC | _MCW_DN)

static __inline
unsigned int
Arm64ReadFpcr(void)
{
    return (unsigned int)_ReadStatusReg(ARM64_FPCR);
}

static __inline
void
Arm64WriteFpcr(unsigned int Value)
{
    _WriteStatusReg(ARM64_FPCR, Value);
}

static __inline
unsigned int
Arm64ReadFpsr(void)
{
    return (unsigned int)_ReadStatusReg(ARM64_FPSR);
}

static __inline
void
Arm64WriteFpsr(unsigned int Value)
{
    _WriteStatusReg(ARM64_FPSR, Value);
}

/* Converts FPSR exception flags into _SW_ status bits */
static __inline
unsigned int
Arm64FlagsToStatus(unsigned int Flags)
{
    unsigned int Status = 0;

    if (Flags & ARM64_FP_INVALID) Status |= _SW_INVALID;
    if (Flags & ARM64_FP_ZERODIVIDE) Status |= _SW_ZERODIVIDE;
    if (Flags & ARM64_FP_OVERFLOW) Status |= _SW_OVERFLOW;
    if (Flags & ARM64_FP_UNDERFLOW) Status |= _SW_UNDERFLOW;
    if (Flags & ARM64_FP_INEXACT) Status |= _SW_INEXACT;
    if (Flags & ARM64_FP_DENORMAL) Status |= _SW_DENORMAL;
    return Status;
}
