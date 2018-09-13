/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements kernel debugger synchronization routines.

Author:

    Ken Reneris (kenr) 30-Aug-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#define IDBG    1


#define FrozenState(a)  (a & 0xF)

// state
#define RUNNING                 0x00
#define TARGET_FROZEN           0x02
#define TARGET_THAW             0x03
#define FREEZE_OWNER            0x04

// flags (bits)
#define FREEZE_ACTIVE           0x20


//
// Define local storage to save the old IRQL.
//

KIRQL KiOldIrql;

#ifndef NT_UP
PKPRCB KiFreezeOwner;
#endif



BOOLEAN
KeFreezeExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function freezes the execution of all other processors in the host
    configuration and then returns to the caller.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to an exception frame that
        describes the trap.

Return Value:

    Previous interrupt enable.

--*/

{

    BOOLEAN Enable;

#if !defined(NT_UP)

    BOOLEAN Flag;
    PKPRCB Prcb;
    ULONG TargetSet;
    ULONG BitNumber;
    KIRQL OldIrql;

#if IDBG

    ULONG Count = 30000;

#endif
#endif

    //
    // Disable interrupts.
    //

    Enable = KiDisableInterrupts();
    KiFreezeFlag = FREEZE_FROZEN;

#if !defined(NT_UP)
    //
    // Raise IRQL to HIGH_LEVEL.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    if (FrozenState(KeGetCurrentPrcb()->IpiFrozen) == FREEZE_OWNER) {
        //
        // This processor already owns the freeze lock.
        // Return without trying to re-acquire lock or without
        // trying to IPI the other processors again
        //

        return Enable;
    }


    //
    // Try to acquire the KiFreezeExecutionLock before sending the request.
    // To prevent deadlock from occurring, we need to accept and process
    // incoming FreexeExecution requests while we are waiting to acquire
    // the FreezeExecutionFlag.
    //

    while (KiTryToAcquireSpinLock (&KiFreezeExecutionLock) == FALSE) {

        //
        // FreezeExecutionLock is busy.  Another processor may be trying
        // to IPI us - go service any IPI.
        //

        KiRestoreInterrupts(Enable);
        Flag = KiIpiServiceRoutine((PVOID)TrapFrame, (PVOID)ExceptionFrame);
        KiDisableInterrupts();

#if IDBG

        if (Flag != FALSE) {
            Count = 30000;
            continue;
        }

        KeStallExecutionProcessor (100);
        if (!Count--) {
            Count = 30000;
            if (KiTryToAcquireSpinLock (&KiFreezeLockBackup) == TRUE) {
                KiFreezeFlag |= FREEZE_BACKUP;
                break;
            }
        }

#endif

    }

    //
    // After acquiring the lock flag, we send Freeze request to each processor
    // in the system (other than us) and wait for it to become frozen.
    //

    Prcb = KeGetCurrentPrcb();  // Do this after spinlock is acquired.
    TargetSet = KeActiveProcessors & ~(1 << Prcb->Number);
    if (TargetSet) {

#if IDBG
        Count = 400;
#endif

        KiFreezeOwner = Prcb;
        Prcb->IpiFrozen = FREEZE_OWNER | FREEZE_ACTIVE;
        Prcb->SkipTick  = TRUE;
        KiIpiSend((KAFFINITY) TargetSet, IPI_FREEZE);

        while (TargetSet != 0) {
            BitNumber = KeFindFirstSetRightMember(TargetSet);
            ClearMember(BitNumber, TargetSet);
            Prcb = KiProcessorBlock[BitNumber];

#if IDBG

            while (Prcb->IpiFrozen != TARGET_FROZEN) {
                if (Count == 0) {
                    KiFreezeFlag |= FREEZE_SKIPPED_PROCESSOR;
                    break;
                }

                KeStallExecutionProcessor (10000);
                Count--;
            }

#else

            while (Prcb->IpiFrozen != TARGET_FROZEN) {
                KeYieldProcessor();
            }
#endif

        }
    }

    //
    // Save the old IRQL and return whether interrupts were previous enabled.
    //

    KiOldIrql = OldIrql;

#endif      // !defined(NT_UP)

    return Enable;
}

VOID
KiFreezeTargetExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function freezes the execution of the current running processor.
    If a trapframe is supplied to current state is saved into the prcb
    for the debugger.

Arguments:

    TrapFrame - Supplies a pointer to the trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to the exception frame that
        describes the trap.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    KIRQL OldIrql;
    PKPRCB Prcb;
    BOOLEAN Enable;
    KCONTINUE_STATUS Status;
    EXCEPTION_RECORD ExceptionRecord;

    Enable = KiDisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    Prcb = KeGetCurrentPrcb();
    Prcb->IpiFrozen = TARGET_FROZEN;
    Prcb->SkipTick  = TRUE;

    if (TrapFrame != NULL) {
        KiSaveProcessorState(TrapFrame, ExceptionFrame);
    }

    //
    // Sweep the data cache in case this is a system crash and the bug
    // check code is attempting to write a crash dump file.
    //

    KeSweepCurrentDcache();

    //
    //  Wait for person requesting us to freeze to
    //  clear our frozen flag
    //

    while (FrozenState(Prcb->IpiFrozen) == TARGET_FROZEN) {
        if (Prcb->IpiFrozen & FREEZE_ACTIVE) {

            //
            // This processor has been made the active processor
            //
            if (TrapFrame) {
                RtlZeroMemory (&ExceptionRecord, sizeof ExceptionRecord);
                ExceptionRecord.ExceptionCode = STATUS_WAKE_SYSTEM_DEBUGGER;
                ExceptionRecord.ExceptionRecord  = &ExceptionRecord;
                ExceptionRecord.ExceptionAddress =
                    (PVOID)CONTEXT_TO_PROGRAM_COUNTER (&Prcb->ProcessorState.ContextFrame);

                Status = (KiDebugSwitchRoutine) (
                            &ExceptionRecord,
                            &Prcb->ProcessorState.ContextFrame,
                            FALSE
                            );

            } else {
                Status = ContinueError;
            }

            //
            // If status is anything other then, continue with next
            // processor then reselect master
            //

            if (Status != ContinueNextProcessor) {
                Prcb->IpiFrozen &= ~FREEZE_ACTIVE;
                KiFreezeOwner->IpiFrozen |= FREEZE_ACTIVE;
            }
        }
        KeYieldProcessor();
    }

    if (TrapFrame != NULL) {
        KiRestoreProcessorState(TrapFrame, ExceptionFrame);
    }

    Prcb->IpiFrozen = RUNNING;

    KeFlushCurrentTb();
    KeSweepCurrentIcache();

    KeLowerIrql(OldIrql);
    KiRestoreInterrupts(Enable);
#endif      // !define(NT_UP)

    return;
}


KCONTINUE_STATUS
KeSwitchFrozenProcessor (
    IN ULONG ProcessorNumber
    )
{
#if !defined(NT_UP)
    PKPRCB TargetPrcb, CurrentPrcb;

    //
    // If Processor number is out of range, reselect current processor
    //

    if (ProcessorNumber >= (ULONG) KeNumberProcessors) {
        return ContinueProcessorReselected;
    }

    TargetPrcb = KiProcessorBlock[ProcessorNumber];
    CurrentPrcb = KeGetCurrentPrcb();

    //
    // Move active flag to correct processor.
    //

    CurrentPrcb->IpiFrozen &= ~FREEZE_ACTIVE;
    TargetPrcb->IpiFrozen  |= FREEZE_ACTIVE;

    //
    // If this processor is frozen in KiFreezeTargetExecution, return to it
    //

    if (FrozenState(CurrentPrcb->IpiFrozen) == TARGET_FROZEN) {
        return ContinueNextProcessor;
    }

    //
    // This processor must be FREEZE_OWNER, wait to be reselected as the
    // active processor
    //

    if (FrozenState(CurrentPrcb->IpiFrozen) != FREEZE_OWNER) {
        return ContinueError;
    }

    while (!(CurrentPrcb->IpiFrozen & FREEZE_ACTIVE)) {
        KeYieldProcessor();
    }

#endif  // !defined(NT_UP)

    //
    // Reselect this processor
    //

    return ContinueProcessorReselected;
}


VOID
KeThawExecution (
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function thaws the execution of all other processors in the host
    configuration and then returns to the caller. It is intended for use by
    the kernel debugger.

Arguments:

    Enable - Supplies the previous interrupt enable that is to be restored
        after having thawed the execution of all other processors.

Return Value:

    None.

--*/

{
#if !defined(NT_UP)

    KIRQL OldIrql;
    ULONG TargetSet;
    ULONG BitNumber;
    ULONG Flag;
    PKPRCB Prcb;

    //
    // Before releasing FreezeExecutionLock clear any all targets IpiFrozen
    // flag.
    //

    KeGetCurrentPrcb()->IpiFrozen = RUNNING;

    TargetSet = KeActiveProcessors & ~(1 << KeGetCurrentPrcb()->Number);
    while (TargetSet != 0) {
        BitNumber = KeFindFirstSetRightMember(TargetSet);
        ClearMember(BitNumber, TargetSet);
        Prcb = KiProcessorBlock[BitNumber];
#if IDBG
        //
        // If the target processor was not forzen, then don't wait
        // for target to unfreeze.
        //

        if (FrozenState(Prcb->IpiFrozen) != TARGET_FROZEN) {
            Prcb->IpiFrozen = RUNNING;
            continue;
        }
#endif

        Prcb->IpiFrozen = TARGET_THAW;
        while (Prcb->IpiFrozen == TARGET_THAW) {
            KeYieldProcessor();
        }
    }

    //
    // Capture the previous IRQL before releasing the freeze lock.
    //

    OldIrql = KiOldIrql;

#if IDBG

    Flag = KiFreezeFlag;
    KiFreezeFlag = 0;

    if ((Flag & FREEZE_BACKUP) != 0) {
        KiReleaseSpinLock(&KiFreezeLockBackup);
    } else {
        KiReleaseSpinLock(&KiFreezeExecutionLock);
    }

#else

    KiFreezeFlag = 0;
    KiReleaseSpinLock(&KiFreezeExecutionLock);

#endif
#endif  // !defined (NT_UP)


    //
    // Flush the current TB, instruction cache, and data cache.
    //

    KeFlushCurrentTb();
    KeSweepCurrentIcache();
    KeSweepCurrentDcache();

    //
    // Lower IRQL and restore interrupt enable
    //

#if !defined(NT_UP)
    KeLowerIrql(OldIrql);
#endif
    KiRestoreInterrupts(Enable);
    return;
}

VOID
KeReturnToFirmware (
    IN FIRMWARE_REENTRY Routine
    )

/*++

Routine Description:

    This routine will thaw all other processors in an MP environment to cause
    them to return to do a return to firmware with the supplied parameter.

    It will then call HalReturnToFirmware itself.

    N.B. It is assumed that we are in the environment of the kernel debugger
    or a crash dump.


Arguments:

    Routine - What to invoke on return to firmware.

Return Value:

    None.

--*/

{

    //
    // Just get the interface in now.  When intel and kenr come up with the
    // right stuff we can fill this in.
    //

    HalReturnToFirmware(Routine);

}

VOID
KiPollFreezeExecution(
    VOID
    )

/*++

Routine Description:

    This routine is called from code that is spinning with interrupts
    disabled, waiting for something to happen, when there is some
    (possibly extremely small) chance that that thing will not happen
    because a system freeze has been initiated.

    N.B. Interrupts are disabled.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Check to see if a freeze is pending for this processor.
    //

    PKPRCB Prcb = KeGetCurrentPrcb();

    if ((Prcb->RequestSummary & IPI_FREEZE) != 0) {

        //
        // Clear the freeze request and freeze this processor.
        //

        InterlockedExchangeAdd((PLONG)&Prcb->RequestSummary, -(IPI_FREEZE));
        KiFreezeTargetExecution(NULL, NULL);

    } else {

        //
        // No freeze pending, assume this processor is spinning.
        //

        KeYieldProcessor();
    }
}
