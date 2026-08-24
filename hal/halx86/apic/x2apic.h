/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header file for x2APIC
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#pragma once

#include "apicp.h"

#define X2APIC_MSR_BASE 0x00000800
#define CPUID_X2APIC_FEATURE_BIT 21
#define X2APIC_MSR_ICR 0x00000830
#define X2APIC_MSR_SELF_IPI 0x0000083F

FORCEINLINE
APIC_REGISTER
X2ApicMsrFromRegister(APIC_REGISTER Register)
{
    return (APIC_REGISTER)(X2APIC_MSR_BASE + (Register / 0x10));
}

FORCEINLINE
ULONG
X2ApicRead(_In_ APIC_REGISTER Register)
{
    return (ULONG)__readmsr(X2ApicMsrFromRegister(Register));
}

FORCEINLINE
VOID
X2ApicWrite(_In_ APIC_REGISTER Register, _In_ ULONG Value)
{
    __writemsr(X2ApicMsrFromRegister(Register), Value);
}

FORCEINLINE
VOID
X2ApicWriteIcr(_In_ APIC_INTERRUPT_COMMAND_REGISTER IcrValue)
{
    __writemsr(X2APIC_MSR_ICR, IcrValue.LongLong);
}

BOOLEAN
NTAPI
X2ApicIsSupported(VOID);

VOID
NTAPI
X2ApicEnable(VOID);