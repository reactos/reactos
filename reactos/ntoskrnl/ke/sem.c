/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements kernel semaphores
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeSemaphore(PKSEMAPHORE Semaphore,
                      LONG Count,
                      LONG Limit)
{

    DPRINT("KeInitializeSemaphore Sem: %x\n", Semaphore);
    
    /* Simply Initialize the Header */
    KeInitializeDispatcherHeader(&Semaphore->Header,
                                 SemaphoreObject,
                                 sizeof(KSEMAPHORE)/sizeof(ULONG),
                                 Count);

    /* Set the Limit */
    Semaphore->Limit = Limit;  
}

/*
 * @implemented
 */
LONG 
STDCALL
KeReadStateSemaphore(PKSEMAPHORE Semaphore)
{
    /* Just return the Signal State */
    return(Semaphore->Header.SignalState);
}

/*
 * @implemented
 *
 * FUNCTION: KeReleaseSemaphore releases a given semaphore object. This
 * routine supplies a runtime priority boost for waiting threads. If this
 * call sets the semaphore to the Signaled state, the semaphore count is
 * augmented by the given value. The caller can also specify whether it
 * will call one of the KeWaitXXX routines as soon as KeReleaseSemaphore
 * returns control.
 * ARGUMENTS:
 *       Semaphore = Points to an initialized semaphore object for which the
 *                   caller provides the storage.
 *       Increment = Specifies the priority increment to be applied if
 *                   releasing the semaphore causes a wait to be 
 *                   satisfied.
 *       Adjustment = Specifies a value to be added to the current semaphore
 *                    count. This value must be positive
 *       Wait = Specifies whether the call to KeReleaseSemaphore is to be
 *              followed immediately by a call to one of the KeWaitXXX.
 * RETURNS: If the return value is zero, the previous state of the semaphore
 *          object is Not-Signaled.
 */
LONG 
STDCALL
KeReleaseSemaphore(PKSEMAPHORE Semaphore,
                   KPRIORITY Increment,
                   LONG Adjustment,
                   BOOLEAN Wait)

{
    ULONG InitialState;
    KIRQL OldIrql;
    PKTHREAD CurrentThread;

    DPRINT("KeReleaseSemaphore(Semaphore %x, Increment %d, Adjustment %d, Wait %d)\n", 
            Semaphore, 
            Increment, 
            Adjustment, 
            Wait);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old State */
    InitialState = Semaphore->Header.SignalState;
    
    /* Check if the Limit was exceeded */
    if (Semaphore->Limit < (LONG) InitialState + Adjustment || 
        InitialState > InitialState + Adjustment) {
        
        /* Raise an error if it was exceeded */
        KeReleaseDispatcherDatabaseLock(OldIrql);
        ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
    }

    /* Now set the new state */
    Semaphore->Header.SignalState += Adjustment;
    
    /* Check if we should wake it */
    if (InitialState == 0 && !IsListEmpty(&Semaphore->Header.WaitListHead)) {
        
        /* Wake the Semaphore */
        KiDispatcherObjectWake(&Semaphore->Header, SEMAPHORE_INCREMENT);
    }

    /* If the Wait is true, then return with a Wait and don't unlock the Dispatcher Database */
    if (Wait == FALSE) {
        
        /* Release the Lock */
        KeReleaseDispatcherDatabaseLock(OldIrql);
    
    } else {
        
        /* Set a wait */
        CurrentThread = KeGetCurrentThread();
        CurrentThread->WaitNext = TRUE;
        CurrentThread->WaitIrql = OldIrql;
    }

    /* Return the previous state */
    return InitialState;
}

/* EOF */
