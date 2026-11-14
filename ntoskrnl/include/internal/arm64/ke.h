/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 kernel definitions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASM__

#include "intrin_i.h"

//
// brk #0xF000
//
#define KD_BREAKPOINT_TYPE        ULONG
#define KD_BREAKPOINT_SIZE        sizeof(ULONG)
#define KD_BREAKPOINT_VALUE       0xD43E0000

/* Exception classes from ESR_EL1 */
#define ESR_EC_SHIFT              26
#define ESR_EC_UNKNOWN            0x00
#define ESR_EC_WFX                0x01
#define ESR_EC_FP_ACCESS          0x07
#define ESR_EC_ILLEGAL_STATE      0x0E
#define ESR_EC_SVC64              0x15
#define ESR_EC_SYS64              0x18
#define ESR_EC_SVE                0x19
#define ESR_EC_IABT_LOWER         0x20
#define ESR_EC_IABT_CURRENT       0x21
#define ESR_EC_PC_ALIGN           0x22
#define ESR_EC_DABT_LOWER         0x24
#define ESR_EC_DABT_CURRENT       0x25
#define ESR_EC_SP_ALIGN           0x26
#define ESR_EC_FP_EXCEPTION       0x2C
#define ESR_EC_SERROR             0x2F
#define ESR_EC_BREAKPOINT_LOWER   0x30
#define ESR_EC_BREAKPOINT_CURRENT 0x31
#define ESR_EC_STEP_LOWER         0x32
#define ESR_EC_STEP_CURRENT       0x33
#define ESR_EC_WATCHPOINT_LOWER   0x34
#define ESR_EC_WATCHPOINT_CURRENT 0x35
#define ESR_EC_BRK64              0x3C

/* Data and instruction abort syndrome */
#define ESR_ISS_WNR               0x40
#define ESR_ISS_FSC_MASK          0x3F

/* Trap types passed from the vectors to KiArm64TrapDispatch */
#define KI_TRAP_SYNCHRONOUS       0
#define KI_TRAP_IRQ               1
#define KI_TRAP_FIQ               2
#define KI_TRAP_SERROR            3

//
// One-liners for getting and setting special purpose registers in portable code
//
FORCEINLINE
ULONG_PTR
KeGetContextPc(
    _In_ PCONTEXT Context)
{
    return Context->Pc;
}

FORCEINLINE
VOID
KeSetContextPc(
    _Inout_ PCONTEXT Context,
    _In_ ULONG_PTR ProgramCounter)
{
    Context->Pc = ProgramCounter;
}

FORCEINLINE
ULONG_PTR
KeGetContextReturnRegister(
    _In_ PCONTEXT Context)
{
    return Context->X0;
}

FORCEINLINE
VOID
KeSetContextReturnRegister(
    _Inout_ PCONTEXT Context,
    _In_ ULONG_PTR ReturnValue)
{
    Context->X0 = ReturnValue;
}

FORCEINLINE
ULONG_PTR
KeGetContextStackRegister(
    _In_ PCONTEXT Context)
{
    return Context->Sp;
}

FORCEINLINE
ULONG_PTR
KeGetContextFrameRegister(
    _In_ PCONTEXT Context)
{
    return Context->Fp;
}

FORCEINLINE
VOID
KeSetContextFrameRegister(
    _Inout_ PCONTEXT Context,
    _In_ ULONG_PTR Frame)
{
    Context->Fp = Frame;
}

FORCEINLINE
ULONG_PTR
KeGetTrapFramePc(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return TrapFrame->Pc;
}

FORCEINLINE
PKTRAP_FRAME
KiGetLinkedTrapFrame(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return (PKTRAP_FRAME)TrapFrame->TrapFrame;
}

FORCEINLINE
ULONG_PTR
KeGetTrapFrameStackRegister(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return TrapFrame->Sp;
}

FORCEINLINE
ULONG_PTR
KeGetTrapFrameFrameRegister(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return TrapFrame->Fp;
}

FORCEINLINE
ULONG64
KxQueryProcessorCycleTime(VOID)
{
    return (ULONG64)_ReadStatusReg(ARM64_CNTVCT);
}

//
// Macro to get trap and exception frame from a thread stack
//
#define KeGetTrapFrame(Thread) \
    ((PKTRAP_FRAME)((ULONG_PTR)((Thread)->InitialStack) - \
                   sizeof(KTRAP_FRAME)))

#define KeGetExceptionFrame(Thread) \
    (PKEXCEPTION_FRAME)((ULONG_PTR)KeGetTrapFrame(Thread) - \
                        sizeof(KEXCEPTION_FRAME))

//
// Macro to get context switches from the PRCB
// All architectures but x86 have it in the PRCB's KeContextSwitches
//
#define KeGetContextSwitches(Prcb)  \
    ((Prcb)->KeContextSwitches)

//
// Macro to get the second level cache size field name which differs between
// CISC and RISC architectures, as the former has unified I/D cache
//
#define KiGetSecondLevelDCacheSize() ((PKIPCR)KeGetPcr())->SecondLevelCacheSize

//
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
#define KeGetTrapFrameInterruptState(TrapFrame) \
        (((TrapFrame)->Spsr & ARM64_PSR_I) == 0)

/* Disable interrupts and return whether they were enabled before */
FORCEINLINE
BOOLEAN
KeDisableInterrupts(VOID)
{
    ULONG64 Daif;

    Daif = KeArm64ReadRegister(ARM64_DAIF);
    _disable();
    return (Daif & ARM64_PSR_I) ? FALSE : TRUE;
}

/* Restore previous interrupt state */
FORCEINLINE
VOID
KeRestoreInterrupts(
    _In_ BOOLEAN WereEnabled)
{
    if (WereEnabled)
        _enable();
}

FORCEINLINE
VOID
KeInvalidateTlbEntry(
    _In_ PVOID Address)
{
    KeArm64FlushTbEntry(Address);
}

FORCEINLINE
VOID
KeFlushProcessTb(VOID)
{
    KeArm64FlushTbAll();
}

FORCEINLINE
VOID
KeSweepICache(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T FlushSize)
{
    KeArm64SweepIcacheRange(BaseAddress, FlushSize);
}

FORCEINLINE
VOID
KiRundownThread(
    _In_ PKTHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
}

FORCEINLINE
BOOLEAN
KiUserTrap(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return ((TrapFrame->Spsr & ARM64_PSR_MODE_MASK) == ARM64_PSR_MODE_EL0T);
}

#define KiGetPreviousMode(TrapFrame) \
    (KiUserTrap(TrapFrame) ? UserMode : KernelMode)

FORCEINLINE
VOID
KiEndInterrupt(
    _In_ KIRQL Irql,
    _In_ PKTRAP_FRAME TrapFrame)
{
    UNREFERENCED_PARAMETER(Irql);
    UNREFERENCED_PARAMETER(TrapFrame);
}

FORCEINLINE
VOID
Ki386PerfEnd(VOID)
{
}

FORCEINLINE
PULONG_PTR
KiGetUserModeStackAddress(VOID)
{
    return &PsGetCurrentThread()->Tcb.TrapFrame->Sp;
}

DECLSPEC_NORETURN
VOID
KiSwitchToBootStack(
    _In_ ULONG_PTR InitialStack);

VOID
KiArm64InitializeTraps(VOID);

VOID
KiSetProcessorType(VOID);

ULONG64
KiGetFeatureBits(VOID);

VOID
KeArm64InitThreadWithContext(
    _In_ PKTHREAD Thread,
    _In_ PKSYSTEM_ROUTINE SystemRoutine,
    _In_ PKSTART_ROUTINE StartRoutine,
    _In_ PVOID StartContext,
    _In_opt_ PCONTEXT Context);
#define KeArchInitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context) \
    KeArm64InitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context)

#ifdef _NTOSKRNL_
VOID
KiThreadStartup(VOID);
#endif

VOID
KiGetTrapContext(
    _In_ PKTRAP_FRAME TrapFrame,
    _Out_ PCONTEXT Context);

VOID
KiSetTrapContext(
    _Out_ PKTRAP_FRAME TrapFrame,
    _In_ PCONTEXT Context,
    _In_ KPROCESSOR_MODE RequestorMode);

DECLSPEC_NORETURN
VOID
KiUserCallbackExit(
    _In_ PKTRAP_FRAME TrapFrame);

DECLSPEC_NORETURN
VOID
KiExceptionExit(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame);

#endif /* __ASM__ */

// HACK
extern NTKERNELAPI volatile KSYSTEM_TIME KeTickCount;

// win64 uses DMA macros, this one is not defined
NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PWAIT_CONTEXT_BLOCK Wcb,
    _In_ ULONG NumberOfMapRegisters,
    _In_ PDRIVER_CONTROL ExecutionRoutine);

#ifdef __cplusplus
} // extern "C"
#endif
