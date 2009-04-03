/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/time.c
 * PURPOSE:         Implements timebase functionality on ARM processors
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LONG KiTickOffset;
ULONG KeTimeAdjustment;

/* FUNCTIONS ******************************************************************/

ULONG
KiComputeTimerTableIndex(IN LONGLONG DueTime)
{
    ULONG Hand;
    
    //
    // Compute the timer table index
    //
    Hand = (DueTime / KeMaximumIncrement);
    Hand %= TIMER_TABLE_SIZE;
    return Hand;
}

VOID
NTAPI
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN KIRQL Irql,
                   IN ULONG Increment)
{
    PKPRCB Prcb = KeGetPcr()->Prcb;
    ULARGE_INTEGER SystemTime, InterruptTime;
    ULONG Hand;
    
    //
    // Do nothing if this tick is being skipped
    //
    if (Prcb->SkipTick)
    {
        //
        // Handle it next time
        //
        Prcb->SkipTick = FALSE;
        return;
    }
    
    //
    // Add the increment time to the shared data
    //
    InterruptTime.HighPart = SharedUserData->InterruptTime.High1Time;
    InterruptTime.LowPart = SharedUserData->InterruptTime.LowPart;
    InterruptTime.QuadPart += Increment;
    SharedUserData->InterruptTime.High1Time = InterruptTime.HighPart;
    SharedUserData->InterruptTime.LowPart = InterruptTime.LowPart;
    SharedUserData->InterruptTime.High2Time = InterruptTime.HighPart;
    
    //
    // Update tick count
    //
    KiTickOffset -= Increment;
    
    //
    // Check for incomplete tick
    //
    if (KiTickOffset <= 0)
    {
        //
        // Update the system time
        //
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
        SystemTime.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemTime.High1Time = SystemTime.HighPart;
        SharedUserData->SystemTime.LowPart = SystemTime.LowPart;
        SharedUserData->SystemTime.High2Time = SystemTime.HighPart;
        
        //
        // Update the tick count
        //
        SystemTime.HighPart = KeTickCount.High1Time;
        SystemTime.LowPart = KeTickCount.LowPart;
        SystemTime.QuadPart += 1;
        KeTickCount.High1Time = SystemTime.HighPart;
        KeTickCount.LowPart = SystemTime.LowPart;
        KeTickCount.High2Time = SystemTime.HighPart;
        
        //
        // Update it in the shared user data
        //
        SharedUserData->TickCount.High1Time = SystemTime.HighPart;
        SharedUserData->TickCount.LowPart = SystemTime.LowPart;
        SharedUserData->TickCount.High2Time = SystemTime.HighPart;
    }
    
    //
    // Check for timer expiration
    //
    Hand = KeTickCount.LowPart % TIMER_TABLE_SIZE;
    if (KiTimerTableListHead[Hand].Time.QuadPart <= InterruptTime.QuadPart)
    {
        //
        // Check if we are already doing expiration
        //
        if (!Prcb->TimerRequest)
        {                        
            //
            // Request a DPC to handle this
            //
            Prcb->TimerRequest = TrapFrame->SvcSp;
            Prcb->TimerHand = Hand;
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
    }
    
    //
    // Check if we should request a debugger break
    //
    if (KdDebuggerEnabled)
    {
        //
        // Check if we should break
        //
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }
    
    //
    // Check if this was a full tick
    //
    if (KiTickOffset <= 0)
    {
        //
        // Updare the tick offset
        //
        KiTickOffset += KeMaximumIncrement;
        
        //
        // Update system runtime
        //
        KeUpdateRunTime(TrapFrame, CLOCK2_LEVEL);
    }
    else
    {
        //
        // Increase interrupt count and exit
        //
        Prcb->InterruptCount++;
    }
}

VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL Irql)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Do nothing if this tick is being skipped
    //
    if (Prcb->SkipTick)
    {
        //
        // Handle it next time
        //
        Prcb->SkipTick = FALSE;
        return;
    }

    //
    // Increase interrupt count
    //
    Prcb->InterruptCount++;
    
    //
    // Check if we came from user mode
    //
    if (TrapFrame->PreviousMode != KernelMode)
    {
        //
        // Increase process user time
        //
        InterlockedIncrement((PLONG)&Process->UserTime);
        Prcb->UserTime++;
        Thread->UserTime++;
    }
    else
    {
        //
        // See if we were in an ISR
        //
        if (TrapFrame->OldIrql > DISPATCH_LEVEL)
        {
            //
            // Handle that
            //
            Prcb->InterruptTime++;
        }
        else if ((TrapFrame->OldIrql < DISPATCH_LEVEL) ||
                 !(Prcb->DpcRoutineActive))
        {
            //
            // Handle being in kernel mode
            //
            Thread->KernelTime++;
            InterlockedIncrement((PLONG)&Process->KernelTime);
        }
        else
        {
            //
            // Handle being in a DPC
            //
            Prcb->DpcTime++;
            
            //
            // Update Debug DPC time 
            //
            Prcb->DebugDpcTime++;
            
            //
            // Check if we've timed out
            //
            if (Prcb->DebugDpcTime >= KiDPCTimeout)
            {
                //
                // Print a message
                //
                DbgPrint("\n*** DPC routine > 1 sec --- This is not a break in "
                         "KeUpdateSystemTime\n");
                
                //
                // Break if a debugger is attached
                //
                if (KdDebuggerEnabled) DbgBreakPoint();
                
                //
                // Restore the debug DPC time
                //
                Prcb->DebugDpcTime = 0;
            }
        }
    }
    
    //
    // Update DPC rates
    //
    Prcb->DpcRequestRate = ((Prcb->DpcData[0].DpcCount - Prcb->DpcLastCount) +
                            Prcb->DpcRequestRate) >> 1;
    Prcb->DpcLastCount = Prcb->DpcData[0].DpcCount;
    
    //
    // Check if the queue is large enough
    //
    if ((Prcb->DpcData[0].DpcQueueDepth) && !(Prcb->DpcRoutineActive))
    {
        //
        // Request a DPC
        //
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        
        //
        // Fix the maximum queue depth
        //
        if ((Prcb->DpcRequestRate < KiIdealDpcRate) &&
            (Prcb->MaximumDpcQueueDepth > 1))
        {
            //
            // Make it smaller
            //
            Prcb->MaximumDpcQueueDepth--;
        }
    }
    else
    {
        //
        // Check if we've reached the adjustment limit
        //
        if (!(--Prcb->AdjustDpcThreshold))
        {
            //
            // Reset it, and check the queue maximum
            //
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
            {
                //
                // Increase it
                //
                Prcb->MaximumDpcQueueDepth++;
            }
        }
    }
    
    //
    // Decrement the thread quantum
    //
    Thread->Quantum -= CLOCK_QUANTUM_DECREMENT;
    
    //
    // Check if the time expired
    //
    if ((Thread->Quantum <= 0) && (Thread != Prcb->IdleThread))
    {
        //
        // Schedule a quantum end
        //
        Prcb->QuantumEnd = 1;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }
}
