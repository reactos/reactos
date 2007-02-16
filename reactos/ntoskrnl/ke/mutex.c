/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/mutex.c
 * PURPOSE:         Implements the Mutant Dispatcher Object
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
KeInitializeMutant(IN PKMUTANT Mutant,
                   IN BOOLEAN InitialOwner)
{
    PKTHREAD CurrentThread;
    KIRQL OldIrql;

    /* Check if we have an initial owner */
    if (InitialOwner)
    {
        /* We also need to associate a thread */
        CurrentThread = KeGetCurrentThread();
        Mutant->OwnerThread = CurrentThread;

        /* We're about to touch the Thread, so lock the Dispatcher */
        OldIrql = KiAcquireDispatcherLock();

        /* And insert it into its list */
        InsertTailList(&CurrentThread->MutantListHead,
                       &Mutant->MutantListEntry);

        /* Release Dispatcher Lock */
        KiReleaseDispatcherLock(OldIrql);
    }
    else
    {
        /* In this case, we don't have an owner yet */
        Mutant->OwnerThread = NULL;
    }

    /* Now we set up the Dispatcher Header */
    KeInitializeDispatcherHeader(&Mutant->Header,
                                 MutantObject,
                                 sizeof(KMUTANT) / sizeof(ULONG),
                                 InitialOwner ? FALSE : TRUE);

    /* Initialize the default data */
    Mutant->Abandoned = FALSE;
    Mutant->ApcDisable = 0;
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeMutex(IN PKMUTEX Mutex,
                  IN ULONG Level)
{
    /* Set up the Dispatcher Header */
    KeInitializeDispatcherHeader(&Mutex->Header,
                                 MutantObject,
                                 sizeof(KMUTEX) / sizeof(ULONG),
                                 TRUE);

    /* Initialize the default data */
    Mutex->OwnerThread = NULL;
    Mutex->Abandoned = FALSE;
    Mutex->ApcDisable = 1;
}

/*
 * @implemented
 */
LONG
NTAPI
KeReadStateMutant(IN PKMUTANT Mutant)
{
    /* Return the Signal State */
    return Mutant->Header.SignalState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeReleaseMutant(IN PKMUTANT Mutant,
                IN KPRIORITY Increment,
                IN BOOLEAN Abandon,
                IN BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    BOOLEAN EnableApc = FALSE;
    ASSERT_MUTANT(Mutant);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Previous State */
    PreviousState = Mutant->Header.SignalState;

    /* Check if it is to be abandonned */
    if (Abandon == FALSE)
    {
        /* Make sure that the Owner Thread is the current Thread */
        if (Mutant->OwnerThread != CurrentThread)
        {
            /* Release the lock */
            KiReleaseDispatcherLock(OldIrql);

            /* Raise an exception */
            ExRaiseStatus(Mutant->Abandoned ? STATUS_ABANDONED :
                                              STATUS_MUTANT_NOT_OWNED);
        }

        /* If the thread owns it, then increase the signal state */
        Mutant->Header.SignalState++;
    }
    else
    {
        /* It's going to be abandonned */
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
    }

    /* Check if the signal state is only single */
    if (Mutant->Header.SignalState == 1)
    {
        /* Check if it's below 0 now */
        if (PreviousState <= 0)
        {
            /* Remove the mutant from the list */
            RemoveEntryList(&Mutant->MutantListEntry);

            /* Save if we need to re-enable APCs */
            EnableApc = Mutant->ApcDisable;
        }

        /* Remove the Owning Thread and wake it */
        Mutant->OwnerThread = NULL;

        /* Check if the Wait List isn't empty */
        if (!IsListEmpty(&Mutant->Header.WaitListHead))
        {
            /* Wake the Mutant */
            KiWaitTest(&Mutant->Header, Increment);
        }
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
        CurrentThread->WaitNext = TRUE;
        CurrentThread->WaitIrql = OldIrql;
    }

    /* Check if we need to re-enable APCs */
    if (EnableApc) KeLeaveCriticalRegion();

    /* Return the previous state */
    return PreviousState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeReleaseMutex(IN PKMUTEX Mutex,
               IN BOOLEAN Wait)
{
    ASSERT_MUTANT(Mutex);

    /* There's no difference at this level between the two */
    return KeReleaseMutant(Mutex, 1, FALSE, Wait);
}

/* EOF */
