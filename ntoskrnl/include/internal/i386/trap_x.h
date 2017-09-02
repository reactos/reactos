/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/internal/i386/trap_x.h
 * PURPOSE:         Internal Inlined Functions for the Trap Handling Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#if defined(_MSC_VER)
#define UNREACHABLE   __assume(0)
#define               __builtin_expect(a,b) (a)
#elif defined(__GNUC__)
#define UNREACHABLE   __builtin_unreachable()
#else
#error
#endif

//
// Helper Code
//
FORCEINLINE
BOOLEAN
KiUserTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Anything else but Ring 0 is Ring 3 */
    return !!(TrapFrame->SegCs & MODE_MASK);
}

//
// Debug Macros
//
FORCEINLINE
VOID
KiDumpTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Dump the whole thing */
    DbgPrint("DbgEbp: %x\n", TrapFrame->DbgEbp);
    DbgPrint("DbgEip: %x\n", TrapFrame->DbgEip);
    DbgPrint("DbgArgMark: %x\n", TrapFrame->DbgArgMark);
    DbgPrint("DbgArgPointer: %x\n", TrapFrame->DbgArgPointer);
    DbgPrint("TempSegCs: %x\n", TrapFrame->TempSegCs);
    DbgPrint("TempEsp: %x\n", TrapFrame->TempEsp);
    DbgPrint("Dr0: %x\n", TrapFrame->Dr0);
    DbgPrint("Dr1: %x\n", TrapFrame->Dr1);
    DbgPrint("Dr2: %x\n", TrapFrame->Dr2);
    DbgPrint("Dr3: %x\n", TrapFrame->Dr3);
    DbgPrint("Dr6: %x\n", TrapFrame->Dr6);
    DbgPrint("Dr7: %x\n", TrapFrame->Dr7);
    DbgPrint("SegGs: %x\n", TrapFrame->SegGs);
    DbgPrint("SegEs: %x\n", TrapFrame->SegEs);
    DbgPrint("SegDs: %x\n", TrapFrame->SegDs);
    DbgPrint("Edx: %x\n", TrapFrame->Edx);
    DbgPrint("Ecx: %x\n", TrapFrame->Ecx);
    DbgPrint("Eax: %x\n", TrapFrame->Eax);
    DbgPrint("PreviousPreviousMode: %x\n", TrapFrame->PreviousPreviousMode);
    DbgPrint("ExceptionList: %p\n", TrapFrame->ExceptionList);
    DbgPrint("SegFs: %x\n", TrapFrame->SegFs);
    DbgPrint("Edi: %x\n", TrapFrame->Edi);
    DbgPrint("Esi: %x\n", TrapFrame->Esi);
    DbgPrint("Ebx: %x\n", TrapFrame->Ebx);
    DbgPrint("Ebp: %x\n", TrapFrame->Ebp);
    DbgPrint("ErrCode: %x\n", TrapFrame->ErrCode);
    DbgPrint("Eip: %x\n", TrapFrame->Eip);
    DbgPrint("SegCs: %x\n", TrapFrame->SegCs);
    DbgPrint("EFlags: %x\n", TrapFrame->EFlags);
    DbgPrint("HardwareEsp: %x\n", TrapFrame->HardwareEsp);
    DbgPrint("HardwareSegSs: %x\n", TrapFrame->HardwareSegSs);
    DbgPrint("V86Es: %x\n", TrapFrame->V86Es);
    DbgPrint("V86Ds: %x\n", TrapFrame->V86Ds);
    DbgPrint("V86Fs: %x\n", TrapFrame->V86Fs);
    DbgPrint("V86Gs: %x\n", TrapFrame->V86Gs);
}

#if DBG
FORCEINLINE
VOID
KiFillTrapFrameDebug(IN PKTRAP_FRAME TrapFrame)
{
    /* Set the debug information */
    TrapFrame->DbgArgPointer = TrapFrame->Edx;
    TrapFrame->DbgArgMark = 0xBADB0D00;
    TrapFrame->DbgEip = TrapFrame->Eip;
    TrapFrame->DbgEbp = TrapFrame->Ebp;
    TrapFrame->PreviousPreviousMode = (ULONG)-1;
}

#define DR7_RESERVED_READ_AS_1 0x400

#define CheckDr(DrNumner, ExpectedValue) \
    { \
        ULONG DrValue = __readdr(DrNumner); \
        if (DrValue != (ExpectedValue)) \
        { \
            DbgPrint("Dr%ld: expected %.8lx, got %.8lx\n", \
                    DrNumner, ExpectedValue, DrValue); \
            __debugbreak(); \
        } \
    }

extern BOOLEAN StopChecking;

FORCEINLINE
VOID
KiExitTrapDebugChecks(IN PKTRAP_FRAME TrapFrame,
                      IN BOOLEAN SkipPreviousMode)
{
    /* Don't check recursively */
    if (StopChecking) return;
    StopChecking = TRUE;

    /* Make sure interrupts are disabled */
    if (__readeflags() & EFLAGS_INTERRUPT_MASK)
    {
        DbgPrint("Exiting with interrupts enabled: %lx\n", __readeflags());
        __debugbreak();
    }

    /* Make sure this is a real trap frame */
    if (TrapFrame->DbgArgMark != 0xBADB0D00)
    {
        DbgPrint("Exiting with an invalid trap frame? (No MAGIC in trap frame)\n");
        KiDumpTrapFrame(TrapFrame);
        __debugbreak();
    }

    /* Make sure we're not in user-mode or something */
    if (Ke386GetFs() != KGDT_R0_PCR)
    {
        DbgPrint("Exiting with an invalid FS: %lx\n", Ke386GetFs());
        __debugbreak();
    }

    /* Make sure we have a valid SEH chain */
    if (KeGetPcr()->NtTib.ExceptionList == 0)
    {
        DbgPrint("Exiting with NULL exception chain: %p\n", KeGetPcr()->NtTib.ExceptionList);
        __debugbreak();
    }

    /* Make sure we're restoring a valid SEH chain */
    if (TrapFrame->ExceptionList == 0)
    {
        DbgPrint("Entered a trap with a NULL exception chain: %p\n", TrapFrame->ExceptionList);
        __debugbreak();
    }

    /* If we're ignoring previous mode, make sure caller doesn't actually want it */
    if (SkipPreviousMode && (TrapFrame->PreviousPreviousMode != -1))
    {
        DbgPrint("Exiting a trap witout restoring previous mode, yet previous mode seems valid: %lx\n", TrapFrame->PreviousPreviousMode);
        __debugbreak();
    }

    /* FIXME: KDBG messes around with these improperly */
#if !defined(KDBG)
    /* Check DR values */
    if (KiUserTrap(TrapFrame))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive)
        {
            if ((TrapFrame->Dr7 & ~DR7_RESERVED_MASK) == 0) __debugbreak();

            CheckDr(0, TrapFrame->Dr0);
            CheckDr(1, TrapFrame->Dr1);
            CheckDr(2, TrapFrame->Dr2);
            CheckDr(3, TrapFrame->Dr3);
            CheckDr(7, TrapFrame->Dr7 | DR7_RESERVED_READ_AS_1);
        }
    }
    else
    {
        PKPRCB Prcb = KeGetCurrentPrcb();
        CheckDr(0, Prcb->ProcessorState.SpecialRegisters.KernelDr0);
        CheckDr(1, Prcb->ProcessorState.SpecialRegisters.KernelDr1);
        CheckDr(2, Prcb->ProcessorState.SpecialRegisters.KernelDr2);
        CheckDr(3, Prcb->ProcessorState.SpecialRegisters.KernelDr3);
        // CheckDr(7, Prcb->ProcessorState.SpecialRegisters.KernelDr7); // Disabled, see CORE-10165 for more details.
    }
#endif

    StopChecking = FALSE;
}

#else
#define KiExitTrapDebugChecks(x, y)
#define KiFillTrapFrameDebug(x)
#endif

FORCEINLINE
VOID
KiExitSystemCallDebugChecks(IN ULONG SystemCall,
                            IN PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;

    /* Check if this was a user call */
    if (KiUserTrap(TrapFrame))
    {
        /* Make sure we are not returning with elevated IRQL */
        OldIrql = KeGetCurrentIrql();
        if (OldIrql != PASSIVE_LEVEL)
        {
            /* Forcibly put us in a sane state */
            KeGetPcr()->Irql = PASSIVE_LEVEL;
            _disable();

            /* Fail */
            KeBugCheckEx(IRQL_GT_ZERO_AT_SYSTEM_SERVICE,
                         SystemCall,
                         OldIrql,
                         0,
                         0);
        }

        /* Make sure we're not attached and that APCs are not disabled */
        if ((KeGetCurrentThread()->ApcStateIndex != OriginalApcEnvironment) ||
            (KeGetCurrentThread()->CombinedApcDisable != 0))
        {
            /* Fail */
            KeBugCheckEx(APC_INDEX_MISMATCH,
                         SystemCall,
                         KeGetCurrentThread()->ApcStateIndex,
                         KeGetCurrentThread()->CombinedApcDisable,
                         0);
        }
    }
}

//
// Generic Exit Routine
//
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallSysExitReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiSystemCallTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiEditedTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturn(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturnNoSegments(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL KiTrapReturnNoSegmentsRet8(IN PKTRAP_FRAME TrapFrame);

typedef
VOID
(FASTCALL *PFAST_SYSTEM_CALL_EXIT)(
    IN PKTRAP_FRAME TrapFrame
);

extern PFAST_SYSTEM_CALL_EXIT KiFastCallExitHandler;

//
// Save user mode debug registers and restore kernel values
//
FORCEINLINE
VOID
KiHandleDebugRegistersOnTrapEntry(
    IN PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Save all debug registers in the trap frame */
    TrapFrame->Dr0 = __readdr(0);
    TrapFrame->Dr1 = __readdr(1);
    TrapFrame->Dr2 = __readdr(2);
    TrapFrame->Dr3 = __readdr(3);
    TrapFrame->Dr6 = __readdr(6);
    TrapFrame->Dr7 = __readdr(7);

    /* Disable all active debugging */
    __writedr(7, 0);

    /* Restore kernel values */
    __writedr(0, Prcb->ProcessorState.SpecialRegisters.KernelDr0);
    __writedr(1, Prcb->ProcessorState.SpecialRegisters.KernelDr1);
    __writedr(2, Prcb->ProcessorState.SpecialRegisters.KernelDr2);
    __writedr(3, Prcb->ProcessorState.SpecialRegisters.KernelDr3);
    __writedr(6, Prcb->ProcessorState.SpecialRegisters.KernelDr6);
    __writedr(7, Prcb->ProcessorState.SpecialRegisters.KernelDr7);
}

FORCEINLINE
VOID
KiHandleDebugRegistersOnTrapExit(
    PKTRAP_FRAME TrapFrame)
{
    /* Disable all active debugging */
    __writedr(7, 0);

    /* Load all debug registers from the trap frame */
    __writedr(0, TrapFrame->Dr0);
    __writedr(1, TrapFrame->Dr1);
    __writedr(2, TrapFrame->Dr2);
    __writedr(3, TrapFrame->Dr3);
    __writedr(6, TrapFrame->Dr6);
    __writedr(7, TrapFrame->Dr7);
}

//
// Virtual 8086 Mode Optimized Trap Exit
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiExitV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KIRQL OldIrql;

    /* Get the thread */
    Thread = KeGetCurrentThread();
    while (TRUE)
    {
        /* Return if this isn't V86 mode anymore */
        if (!(TrapFrame->EFlags & EFLAGS_V86_MASK)) KiEoiHelper(TrapFrame);

        /* Turn off the alerted state for kernel mode */
        Thread->Alerted[KernelMode] = FALSE;

        /* Are there pending user APCs? */
        if (__builtin_expect(!Thread->ApcState.UserApcPending, 1)) break;

        /* Raise to APC level and enable interrupts */
        OldIrql = KfRaiseIrql(APC_LEVEL);
        _enable();

        /* Deliver APCs */
        KiDeliverApc(UserMode, NULL, TrapFrame);

        /* Restore IRQL and disable interrupts once again */
        KfLowerIrql(OldIrql);
        _disable();
    }

    /* If we got here, we're still in a valid V8086 context, so quit it */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Restore debug registers from the trap frame */
        KiHandleDebugRegistersOnTrapExit(TrapFrame);
    }

    /* Return from interrupt */
    KiTrapReturnNoSegments(TrapFrame);
}

//
// Virtual 8086 Mode Optimized Trap Entry
//
FORCEINLINE
VOID
KiEnterV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list */
    TrapFrame->ExceptionList = KeGetPcr()->NtTib.ExceptionList;

    /* Save DR7 and check for debugging */
    TrapFrame->Dr7 = __readdr(7);
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Handle debug registers */
        KiHandleDebugRegistersOnTrapEntry(TrapFrame);
    }
}

//
// Interrupt Trap Entry
//
FORCEINLINE
VOID
KiEnterInterruptTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list and terminate it */
    TrapFrame->ExceptionList = KeGetPcr()->NtTib.ExceptionList;
    KeGetPcr()->NtTib.ExceptionList = EXCEPTION_CHAIN_END;

    /* Default to debugging disabled */
    TrapFrame->Dr7 = 0;

    /* Check if the frame was from user mode or v86 mode */
    if (KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive & 0xFF)
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapEntry(TrapFrame);
        }
    }

    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
}

//
// Generic Trap Entry
//
FORCEINLINE
VOID
KiEnterTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list */
    TrapFrame->ExceptionList = KeGetPcr()->NtTib.ExceptionList;

    /* Default to debugging disabled */
    TrapFrame->Dr7 = 0;

    /* Check if the frame was from user mode or v86 mode */
    if (KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Check for active debugging */
        if (KeGetCurrentThread()->Header.DebugActive & 0xFF)
        {
            /* Handle debug registers */
            KiHandleDebugRegistersOnTrapEntry(TrapFrame);
        }
    }

    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
}
