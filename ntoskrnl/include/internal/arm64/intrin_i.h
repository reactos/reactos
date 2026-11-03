/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 system register access
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define ARM64_MIDR_EL1          ARM64_SYSREG(3, 0, 0, 0, 0)
#define ARM64_MPIDR_EL1         ARM64_SYSREG(3, 0, 0, 0, 5)
#define ARM64_ID_AA64PFR0_EL1   ARM64_SYSREG(3, 0, 0, 4, 0)
#define ARM64_ID_AA64DFR0_EL1   ARM64_SYSREG(3, 0, 0, 5, 0)
#define ARM64_ID_AA64ISAR0_EL1  ARM64_SYSREG(3, 0, 0, 6, 0)
#define ARM64_ID_AA64MMFR0_EL1  ARM64_SYSREG(3, 0, 0, 7, 0)
#define ARM64_CTR_EL0           ARM64_SYSREG(3, 3, 0, 0, 1)
#define ARM64_SCTLR_EL1         ARM64_SYSREG(3, 0, 1, 0, 0)
#define ARM64_CPACR_EL1         ARM64_SYSREG(3, 0, 1, 0, 2)
#define ARM64_TTBR0_EL1         ARM64_SYSREG(3, 0, 2, 0, 0)
#define ARM64_TTBR1_EL1         ARM64_SYSREG(3, 0, 2, 0, 1)
#define ARM64_TCR_EL1           ARM64_SYSREG(3, 0, 2, 0, 2)
#define ARM64_DAIF              ARM64_SYSREG(3, 3, 4, 2, 1)
#define ARM64_ESR_EL1           ARM64_SYSREG(3, 0, 5, 2, 0)
#define ARM64_FAR_EL1           ARM64_SYSREG(3, 0, 6, 0, 0)
#define ARM64_PAR_EL1           ARM64_SYSREG(3, 0, 7, 4, 0)
#define ARM64_MAIR_EL1          ARM64_SYSREG(3, 0, 10, 2, 0)
#define ARM64_VBAR_EL1          ARM64_SYSREG(3, 0, 12, 0, 0)
#define ARM64_CONTEXTIDR_EL1    ARM64_SYSREG(3, 0, 13, 0, 1)
#define ARM64_TPIDR_EL1         ARM64_SYSREG(3, 0, 13, 0, 4)
#define ARM64_TPIDRRO_EL0_REG   ARM64_SYSREG(3, 3, 13, 0, 3)
#define ARM64_CNTFRQ_EL0        ARM64_SYSREG(3, 3, 14, 0, 0)
#define ARM64_CNTV_TVAL_EL0     ARM64_SYSREG(3, 3, 14, 3, 0)
#define ARM64_CNTV_CTL_EL0      ARM64_SYSREG(3, 3, 14, 3, 1)
#define ARM64_CNTV_CVAL_EL0     ARM64_SYSREG(3, 3, 14, 3, 2)
#define ARM64_CNTKCTL_EL1       ARM64_SYSREG(3, 0, 14, 1, 0)

/* DAIF and SPSR mask bits */
#define ARM64_PSR_F             0x40
#define ARM64_PSR_I             0x80
#define ARM64_PSR_A             0x100
#define ARM64_PSR_D             0x200
#define ARM64_PSR_MODE_MASK     0xF
#define ARM64_PSR_MODE_EL0T     0x0
#define ARM64_PSR_MODE_EL1H     0x5

#define KeArm64ReadRegister(Register)         ((ULONG64)_ReadStatusReg(Register))
#define KeArm64WriteRegister(Register, Value) _WriteStatusReg((Register), (__int64)(Value))

FORCEINLINE
VOID
KeSetCurrentIrql(
    _In_ KIRQL Irql)
{
    KeGetPcr()->CurrentIrql = Irql;
}

/* Implemented in ke/arm64/cpu.s */
VOID
NTAPI
KeArm64FlushTbEntry(
    _In_ PVOID Address);

VOID
NTAPI
KeArm64FlushTbAll(VOID);

VOID
NTAPI
KeArm64SweepIcacheRange(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Length);

VOID
NTAPI
KeArm64SweepDcacheRange(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Length);

VOID
NTAPI
KeArm64WaitForInterrupt(VOID);

VOID
NTAPI
KeArm64SetTranslationBase(
    _In_ ULONG64 RootTable);
