/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements the Semaphore Dispatcher Object
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeSemaphore(IN PKSEMAPHORE Semaphore,
                      IN LONG Count,
                      IN LONG Limit)
{
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
NTAPI
KeReadStateSemaphore(IN PKSEMAPHORE Semaphore)
{
    ASSERT_SEMAPHORE(Semaphore);

    /* Just return the Signal State */
    return Semaphore->Header.SignalState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeReleaseSemaphore(IN PKSEMAPHORE Semaphore,
                   IN KPRIORITY Increment,
                   IN LONG Adjustment,
                   IN BOOLEAN Wait)
{
    LONG InitialState, State;
    KIRQL OldIrql;
    PKTHREAD CurrentThread;
    ASSERT_SEMAPHORE(Semaphore);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

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

    /* Check if the caller wants to wait after this release */
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
