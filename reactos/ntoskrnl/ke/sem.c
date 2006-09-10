/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements kernel semaphores
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
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
                                 sizeof(KSEMAPHORE) / sizeof(ULONG),
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
    return Semaphore->Header.SignalState;
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
    LONG InitialState, State;
    KIRQL OldIrql;
    PKTHREAD CurrentThread;

    DPRINT("KeReleaseSemaphore(Semaphore %x, Increment %d, Adjustment %d, Wait %d)\n",
            Semaphore,
            Increment,
            Adjustment,
            Wait);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Old State and get new one */
    InitialState = Semaphore->Header.SignalState;
    State = InitialState + Adjustment;

    /* Check if the Limit was exceeded */
    if ((Semaphore->Limit < State) || (InitialState > State))
    {
        /* Raise an error if it was exceeded */
        KiReleaseDispatcherLock(OldIrql);
        ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
    }

    /* Now set the new state */
    Semaphore->Header.SignalState = State;

    /* Check if we should wake it */
    if (!(InitialState) && !(IsListEmpty(&Semaphore->Header.WaitListHead)))
    {
        /* Wake the Semaphore */
        KiWaitTest(&Semaphore->Header, Increment);
    }

    /* If the Wait is true, then return with a Wait and don't unlock the Dispatcher Database */
    if (Wait == FALSE)
    {
        /* Release the Lock */
        KiReleaseDispatcherLock(OldIrql);
    }
    else
    {
        /* Set a wait */
        CurrentThread = KeGetCurrentThread();
        CurrentThread->WaitNext = TRUE;
        CurrentThread->WaitIrql = OldIrql;
    }

    /* Return the previous state */
    return InitialState;
}

/* EOF */
