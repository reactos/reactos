/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/time.c
 * PURPOSE:         Implements timebase functionality
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

VOID
FASTCALL
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN ULONG Increment,
                   IN KIRQL Irql)                   
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULARGE_INTEGER CurrentTime, InterruptTime;
    ULONG Hand, OldTickCount;
    
    /* Add the increment time to the shared data */
    InterruptTime.HighPart = SharedUserData->InterruptTime.High1Time;
    InterruptTime.LowPart = SharedUserData->InterruptTime.LowPart;
    InterruptTime.QuadPart += Increment;
    SharedUserData->InterruptTime.High1Time = InterruptTime.HighPart;
    SharedUserData->InterruptTime.LowPart = InterruptTime.LowPart;
    SharedUserData->InterruptTime.High2Time = InterruptTime.HighPart;
    
    /* Update tick count */
    InterlockedExchangeAdd(&KiTickOffset, -(LONG)Increment);
    
    /* Check for incomplete tick */
    OldTickCount = KeTickCount.LowPart;
    if (KiTickOffset <= 0)
    {
        /* Update the system time */
        CurrentTime.HighPart = SharedUserData->SystemTime.High1Time;
        CurrentTime.LowPart = SharedUserData->SystemTime.LowPart;
        CurrentTime.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemTime.High2Time = CurrentTime.HighPart;
        SharedUserData->SystemTime.LowPart = CurrentTime.LowPart;
        SharedUserData->SystemTime.High1Time = CurrentTime.HighPart;
        
        /* Update the tick count */
        CurrentTime.HighPart = KeTickCount.High1Time;
        CurrentTime.LowPart = OldTickCount;
        CurrentTime.QuadPart += 1;
        KeTickCount.High2Time = CurrentTime.HighPart;
        KeTickCount.LowPart = CurrentTime.LowPart;
        KeTickCount.High1Time = CurrentTime.HighPart;
        
        /* Update it in the shared user data */
        SharedUserData->TickCount.High2Time = CurrentTime.HighPart;
        SharedUserData->TickCount.LowPart = CurrentTime.LowPart;
        SharedUserData->TickCount.High1Time = CurrentTime.HighPart;
        
        /* Check for timer expiration */
        Hand = OldTickCount & (TIMER_TABLE_SIZE - 1);
        if (KiTimerTableListHead[Hand].Time.QuadPart <= InterruptTime.QuadPart)
        {
            /* Check if we are already doing expiration */
            if (!Prcb->TimerRequest)
            {                        
                /* Request a DPC to handle this */
                Prcb->TimerRequest = (ULONG_PTR)TrapFrame;
                Prcb->TimerHand = Hand;
                HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
            }
        }
        
        /* Check for expiration with the new tick count as well */
        OldTickCount++;
    }
    
    /* Check for timer expiration */
    Hand = OldTickCount & (TIMER_TABLE_SIZE - 1);
    if (KiTimerTableListHead[Hand].Time.QuadPart <= InterruptTime.QuadPart)
    {
        /* Check if we are already doing expiration */
        if (!Prcb->TimerRequest)
        {                        
            /* Request a DPC to handle this */
            Prcb->TimerRequest = (ULONG_PTR)TrapFrame;
            Prcb->TimerHand = Hand;
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
    }
    
    /* Check if this was a full tick */
    if (KiTickOffset <= 0)
    {
        /* Update the tick offset */
        KiTickOffset += KeMaximumIncrement;
        
        /* Update system runtime */
        KeUpdateRunTime(TrapFrame, Irql);
    }
    else
    {
        /* Increase interrupt count and exit */
        Prcb->InterruptCount++;
    }
    
    /* Disable interrupts and end the interrupt */
    KiEndInterrupt(Irql, TrapFrame);
}

VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL Irql)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Increase interrupt count */
    Prcb->InterruptCount++;
    
    /* Check if we came from user mode */
#ifndef _M_ARM
    if ((TrapFrame->SegCs & MODE_MASK) || (TrapFrame->EFlags & EFLAGS_V86_MASK))
#else
    if (TrapFrame->PreviousMode == UserMode)
#endif
    {
        /* Increase thread user time */
        Prcb->UserTime++;
        Thread->UserTime++;
    }
    else
    {
        /* See if we were in an ISR */
        Prcb->KernelTime++;
        if (Irql > DISPATCH_LEVEL)
        {
            /* Handle that */
            Prcb->InterruptTime++;
        }
        else if ((Irql < DISPATCH_LEVEL) || !(Prcb->DpcRoutineActive))
        {
            /* Handle being in kernel mode */
            Thread->KernelTime++;
        }
        else
        {
            /* Handle being in a DPC */
            Prcb->DpcTime++;
        }
    }
    
    /* Update DPC rates */
    Prcb->DpcRequestRate = ((Prcb->DpcData[0].DpcCount - Prcb->DpcLastCount) +
                            Prcb->DpcRequestRate) >> 1;
    Prcb->DpcLastCount = Prcb->DpcData[0].DpcCount;
    
    /* Check if the queue is large enough */
    if ((Prcb->DpcData[0].DpcQueueDepth) && !(Prcb->DpcRoutineActive))
    {
        /* Request a DPC */
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        
        /* Fix the maximum queue depth */
        if ((Prcb->DpcRequestRate < KiIdealDpcRate) &&
            (Prcb->MaximumDpcQueueDepth > 1))
        {
            /* Make it smaller */
            Prcb->MaximumDpcQueueDepth--;
        }
    }
    else
    {
        /* Check if we've reached the adjustment limit */
        if (!(--Prcb->AdjustDpcThreshold))
        {
            /* Reset it, and check the queue maximum */
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
            {
                /* Increase it */
                Prcb->MaximumDpcQueueDepth++;
            }
        }
    }
    
    /* Decrement the thread quantum */
    Thread->Quantum -= CLOCK_QUANTUM_DECREMENT;

    /* Check if the time expired */
    if ((Thread->Quantum <= 0) && (Thread != Prcb->IdleThread))
    {
        /* Schedule a quantum end */
        Prcb->QuantumEnd = 1;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }
}
