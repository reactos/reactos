/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/mutex.c
 * PURPOSE:         Implements Mutexes and Mutants (that silly davec...)
 *
 * PROGRAMMERS:
 *                  Alex Ionescu (alex@relsoft.net) - Reorganized/commented some of the code.
 *                                                    Simplified some functions, fixed some return values and
 *                                                    corrected some minor bugs, added debug output.
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
KeInitializeMutant(IN PKMUTANT Mutant,
                   IN BOOLEAN InitialOwner)
{
    PKTHREAD CurrentThread;
    KIRQL OldIrql;
    DPRINT("KeInitializeMutant: %x\n", Mutant);

    /* Check if we have an initial owner */
    if (InitialOwner == TRUE)
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
        DPRINT("Mutant with Initial Owner\n");
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
STDCALL
KeInitializeMutex(IN PKMUTEX Mutex,
                  IN ULONG Level)
{
    DPRINT("KeInitializeMutex: %x\n", Mutex);

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
STDCALL
KeReadStateMutant(IN PKMUTANT Mutant)
{
    /* Return the Signal State */
    return(Mutant->Header.SignalState);
}

/*
 * @implemented
 */
LONG
STDCALL
KeReleaseMutant(IN PKMUTANT Mutant,
                IN KPRIORITY Increment,
                IN BOOLEAN Abandon,
                IN BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    DPRINT("KeReleaseMutant: %x\n", Mutant);

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
            DPRINT1("Trying to touch a Mutant that the caller doesn't own!\n");

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
        DPRINT("Abandonning the Mutant\n");
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
            DPRINT("Removing Mutant %p\n", Mutant);
            RemoveEntryList(&Mutant->MutantListEntry);

            /* Reenable APCs */
            DPRINT("Re-enabling APCs\n");
            CurrentThread->KernelApcDisable += Mutant->ApcDisable;

            /* Check if the thread has APCs enabled */
            if (!CurrentThread->KernelApcDisable)
            {
                /* Check if any are pending */
                if (!IsListEmpty(&CurrentThread->ApcState.ApcListHead[KernelMode]))
                {
                    /* Set Kernel APC Pending */
                    CurrentThread->ApcState.KernelApcPending = TRUE;

                    /* Request the Interrupt */
                    DPRINT("Requesting APC Interupt\n");
                    HalRequestSoftwareInterrupt(APC_LEVEL);
                }
            }
        }

        /* Remove the Owning Thread and wake it */
        Mutant->OwnerThread = NULL;

        /* Check if the Wait List isn't empty */
        DPRINT("Checking whether to wake the Mutant\n");
        if (!IsListEmpty(&Mutant->Header.WaitListHead))
        {
            /* Wake the Mutant */
            DPRINT("Waking the Mutant\n");
            KiWaitTest(&Mutant->Header, Increment);
        }
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
        CurrentThread->WaitNext = TRUE;
        CurrentThread->WaitIrql = OldIrql;
    }

    /* Return the previous state */
    return PreviousState;
}

/*
 * @implemented
 */
LONG
STDCALL
KeReleaseMutex(IN PKMUTEX Mutex,
               IN BOOLEAN Wait)
{
    /* There's no difference at this level between the two */
    return KeReleaseMutant(Mutex, 1, FALSE, Wait);
}

/* EOF */
