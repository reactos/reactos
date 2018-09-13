/*++

Module Name:

    clock.c

Abstract:

    This module implements the platform specific clock interrupt processing
    routines in for the kernel.

Author:

    Edward G. Chron (echron) 10-Apr-1996 

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include <ia64.h>
#include <ntia64.h>
#include <ntexapi.h>

VOID
KiProcessProfileList (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source,
    IN PLIST_ENTRY     ListHead
    );


BOOLEAN
KiChkTimerExpireSysDpc (
    IN ULONGLONG     TickCount
    )

/*++

Routine Description:

    This routine determines if it should attempt to place a timer expiration DPC 
    in the system DPC list and to drive the DPC by initiating a dispatch level interrupt 
    on the current processor.

    N.B. If DPC is already inserted on the DPC list, we're done.

Arguments:

    TickCount - The lower tick count, timer table hand value

Return Value:

    BOOLEAN - Set to true if Queued DPC or if DPC already Queued.

--*/

{
    BOOLEAN     ret = FALSE;    // No DPC queued, default return value.

    PLIST_ENTRY ListHead = &KiTimerTableListHead[(ULONG)TickCount%TIMER_TABLE_SIZE];
    PLIST_ENTRY NextEntry = ListHead->Flink;

    //
    // Check to see if the list is empty.
    //
    if (NextEntry != ListHead) {
        PKTIMER   Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
        ULONGLONG TimeValue = Timer->DueTime.QuadPart;

        //
        // See if timer expired.
        //

        if (TimeValue <= SharedUserData->InterruptTime) {
    
            PKPRCB Prcb = KeGetCurrentPrcb();
            PKDPC  Dpc  = &KiTimerExpireDpc;

            _disable();
#if !defined(NT_UP)    
            KiAcquireSpinLock(&Prcb->DpcLock);
#endif 
            //
            // Insert DPC only if not already inserted.
            //
            if (Dpc->Lock == NULL) {

                //
                // Put timer expiration DPC in the system DPC list and initiate
                // a dispatch interrupt on the current processor.
                //

                Prcb->DpcCount += 1;
                Prcb->DpcQueueDepth += 1;
                Dpc->Lock = &Prcb->DpcLock;
                Dpc->SystemArgument1 = (PVOID)TickCount;
                Dpc->SystemArgument2 = 0;
                InsertTailList(&Prcb->DpcListHead, &Dpc->DpcListEntry);
                KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
            }

            ret = TRUE;

#if !defined(NT_UP)    
            KiReleaseSpinLock(&Prcb->DpcLock);
#endif 

            _enable();
        }
    }

    return(ret);
}


VOID
KeUpdateSystemTime (
    IN PKTRAP_FRAME TrFrame,
    IN ULONG        Increment
    )

/*++

Routine Description:

    This routine is executed on a single processor in the processor complex.
    It's function is to update the system time and to check to determine if a
    timer has expired.

    N.B. This routine is executed on a single processor in a multiprocess system.
    The remainder of the processors in the complex execute the quantum end and 
    runtime update code.

Arguments:

    TrFrame   - Supplies a pointer to a trap frame.

    Increment - The time increment to be used to adjust the time slice for the next
                tick. The value is supplied in 100ns units.

Return Value:

    None.

--*/

{
    long       SaveTickOffset;
    LARGE_INTEGER SystemTime;

    SharedUserData->InterruptTime += Increment;
    SharedUserData->InterruptHigh2Time = (LONG)(SharedUserData->InterruptTime >> 32);
    KiTickOffset                  -= Increment;

    SaveTickOffset = KiTickOffset;

    if ((LONG)KiTickOffset > 0)
    {
        //
        // Tick has not completed (100ns time units remain).
        //
        // Determine if a timer has expired at the current hand value.
        //

        KiChkTimerExpireSysDpc(KeTickCount); 

    } else {

        //
        // Tick has completed, tick count set to maximum increase plus any 
        // residue and system time is updated.
        //
        // Compute next tick offset.
        //

        KiTickOffset += KeMaximumIncrement;
        SystemTime.HighPart = SharedUserData->SystemHigh1Time;
        SystemTime.LowPart  = SharedUserData->SystemLowTime;
        SystemTime.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemHigh1Time = SystemTime.HighPart;
        SharedUserData->SystemLowTime = SystemTime.LowPart;
        SharedUserData->SystemHigh2Time = SystemTime.HighPart;
        ++KeTickCount;
        SharedUserData->TickCountLow = (ULONG)KeTickCount;

        //
        // Determine if a timer has expired at either the current hand value or 
        // the next hand value.
        //

        if (!KiChkTimerExpireSysDpc(KeTickCount - 1))
            KiChkTimerExpireSysDpc(KeTickCount);

    }

    if (SaveTickOffset <= 0) {
        KeUpdateRunTime(TrFrame);
    }
}


VOID
KeUpdateRunTime (
    IN PKTRAP_FRAME TrFrame
    )

/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    It's function is to update the run time of the current thread, udpate the run
    time for the thread's process, and decrement the current thread's quantum.

Arguments:

    TrFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    KSPIN_LOCK Lock;
    PKPRCB    Prcb    = KeGetCurrentPrcb();
    PKTHREAD  Thread  = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;

    //
    // If thread was executing in user mode:
    //    increment the thread user time.
    //    atomically increment the process user time.
    // else If the old IRQL is greater than the DPC level:
    //         increment the time executing interrupt service routines.
    //      else If the old IRQL is less than the DPC level or If a DPC is not active:
    //              increment the thread kernel time.
    //              atomically increment the process kernel time.
    //      else
    //              increment time executing DPC routines.
    //

    if (TrFrame->PreviousMode != KernelMode) {
        ++Thread->UserTime;

        // Atomic Update of Process User Time required.
        ExInterlockedIncrementLong(&Process->UserTime, &Lock);

        // Update the time spent in user mode for the current processor.
        ++Prcb->UserTime;           
    } else {

        if (TrFrame->OldIrql > DISPATCH_LEVEL) {
            ++Prcb->InterruptTime;
        } else if ((TrFrame->OldIrql < DISPATCH_LEVEL) || 
                   (Prcb->DpcRoutineActive == 0)) {
            ++Thread->KernelTime;
            ExInterlockedIncrementLong(&Process->KernelTime, &Lock);
        } else {
            ++Prcb->DpcTime;
        }

        //
        // Update the time spent in kernel mode for the current processor.
        //

        ++Prcb->KernelTime;         
    }

    // 
    // Update the DPC request rate which is computed as the average between the
    // previous rate and the current rate. 
    // Update the DPC last count with the current DPC count.
    //
    Prcb->DpcRequestRate = ((Prcb->DpcCount - Prcb->DpcLastCount) + Prcb->DpcRequestRate) >> 1;
    Prcb->DpcLastCount = Prcb->DpcCount;

    //
    // If the DPC queue depth is not zero and a DPC routine is not active.
    //      Request a dispatch interrupt.
    //      Decrement the maximum DPC queue depth.
    //      Reset the threshold counter if appropriate.
    //
    if (Prcb->DpcQueueDepth != 0 && Prcb->DpcRoutineActive == 0) {

        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        // Need to request a DPC interrupt. 
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);

        if (Prcb->DpcRequestRate < KiIdealDpcRate && Prcb->MaximumDpcQueueDepth > 1)
            --Prcb->MaximumDpcQueueDepth;
    } else {
        //
        // The DPC queue is empty or a DPC routine is active or a DPC interrupt
        // has been requested. Count down the adjustment threshold and if the count
        // reaches zero, then increment the maximum DPC queue depth but not above
        // the initial value. Also, reset the adjustment threshold value.
        //
        --Prcb->AdjustDpcThreshold;
        if (Prcb->AdjustDpcThreshold == 0) {
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
                ++Prcb->MaximumDpcQueueDepth;
        }    
    }

    //
    // Decrement current thread quantum and determine if quantum end has occurred.
    //
    Thread->Quantum -= CLOCK_QUANTUM_DECREMENT;

    // Set quantum end if time expired, for any thread except idle thread. 
    if (Thread->Quantum == 0 && Thread != Prcb->IdleThread)  {
        
        Prcb->QuantumEnd = 1;

        // Need to request a DPC interrupt. 
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }

}


VOID
KiDecrementQuantum (
    )

/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    Decrement current thread quantum and check to determine if a quantum end
    has occurred.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PKTHREAD  Thread  = KeGetCurrentThread();
    PKPRCB    Prcb    = KeGetCurrentPrcb();

    Thread->Quantum -= CLOCK_QUANTUM_DECREMENT;

    // Set quantum end if time expired, for any thread except idle thread. 
    if (Thread->Quantum == 0 && Thread != Prcb->IdleThread) 
        Prcb->QuantumEnd = 1;
}


VOID
KeProfileInterrupt (
    IN PKTRAP_FRAME TrFrame
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterrupt is an alternate entry for backwards compatability that
         sets the source to zero (ProfileTime).

Arguments:

    TrFrame   - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    KPROFILE_SOURCE Source = 0;

    KeProfileInterruptWithSource(TrFrame, Source);

    return;
}


VOID
KeProfileInterruptWithSource (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterruptWithSource is not currently fully implemented by any of
         the architectures.

Arguments:

    TrFrame - Supplies a pointer to a trap frame.

    Source  - Supplies the source of the profile interrupt.

Return Value:

    None.

--*/

{
    PKTHREAD  Thread  = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;

#if !defined(NT_UP)    
    KiAcquireSpinLock(&KiProfileLock);
#endif 

    KiProcessProfileList(TrFrame, Source, &Process->ProfileListHead);

    KiProcessProfileList(TrFrame, Source, &KiProfileListHead);        

#if !defined(NT_UP)    
    KiAcquireSpinLock(&KiProfileLock);
#endif 
    return;
}


VOID
KiProcessProfileList (
    IN PKTRAP_FRAME    TrFrame,
    IN KPROFILE_SOURCE Source,
    IN PLIST_ENTRY     ListHead
    )
/*++

Routine Description:

    This routine is executed on all processors in the processor complex.
    The routine is entered as the result of an interrupt generated by the profile
    timer. Its function is to update the profile information for the currently
    active profile objects.

    N.B. KeProfileInterruptWithSource is not currently fully implemented by any of
         the architectures.

Arguments:

    TrFrame  - Supplies a pointer to a trap frame.

    Source   - Supplies the source of the profile interrupt.

    ListHead - Supplies a pointer to a profile list.

Return Value:

    None.

--*/

{
    PLIST_ENTRY NextEntry = ListHead->Flink;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Scan profile list and increment profile buckets as appropriate.
    //
    for (; NextEntry != ListHead; NextEntry = NextEntry->Flink) {

        PCHAR  BucketPter;
        PULONG BucketValue;
        
        PKPROFILE Profile = CONTAINING_RECORD(NextEntry, KPROFILE, ProfileListEntry);

        if (Profile->Source != Source || ((Profile->Affinity & Prcb->SetMember) == 0))
            continue;
        
        if ((PVOID)TrFrame->StIIP < Profile->RangeBase || (PVOID)TrFrame->StIIP > Profile->RangeLimit)
            continue;

        BucketPter = (PCHAR)Profile->Buffer +
                     ((((PCHAR)TrFrame->StIIP - (PCHAR)Profile->RangeBase)
                     >> Profile->BucketShift) & 0xFFFFFFFC);

        BucketValue = (PULONG) BucketPter;
        ++BucketValue;
    }   

    return;
}
