/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ke_x.h
* PURPOSE:         Internal Inlined Functions for the Kernel
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Enters a Critical Region
//
#define KeEnterGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT((Thread->SpecialApcDisable <= 0) &&      \
           (Thread->SpecialApcDisable != -32768));  \
                                                    \
    /* Disable Special APCs */                      \
    Thread->SpecialApcDisable--;                    \
}

//
// Leaves a Critical Region
//
#define KeLeaveGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT(Thread->SpecialApcDisable < 0);          \
                                                    \
    /* Leave region and check if APCs are OK now */ \
    if (!(++Thread->SpecialApcDisable))             \
    {                                               \
        /* Check for Kernel APCs on the list */     \
        if (!IsListEmpty(&Thread->ApcState.         \
                         ApcListHead[KernelMode]))  \
        {                                           \
            /* Check for APC Delivery */            \
            KiCheckForKernelApcDelivery();          \
        }                                           \
    }                                               \
}

//
// TODO: Guarded Mutex Routines
//

//
// TODO: Critical Region Routines
//

//
// Satisfies the wait of any dispatcher object
//
#define KiSatisfyObjectWait(Object, Thread)                                 \
{                                                                           \
    /* Special case for Mutants */                                          \
    if ((Object)->Header.Type == MutantObject)                              \
    {                                                                       \
        /* Decrease the Signal State */                                     \
        (Object)->Header.SignalState--;                                     \
                                                                            \
        /* Check if it's now non-signaled */                                \
        if (!(Object)->Header.SignalState)                                  \
        {                                                                   \
            /* Set the Owner Thread */                                      \
            (Object)->OwnerThread = Thread;                                 \
                                                                            \
            /* Disable APCs if needed */                                    \
            Thread->KernelApcDisable -= (Object)->ApcDisable;               \
                                                                            \
            /* Check if it's abandoned */                                   \
            if ((Object)->Abandoned)                                        \
            {                                                               \
                /* Unabandon it */                                          \
                (Object)->Abandoned = FALSE;                                \
                                                                            \
                /* Return Status */                                         \
                Thread->WaitStatus = STATUS_ABANDONED;                      \
            }                                                               \
                                                                            \
            /* Insert it into the Mutant List */                            \
            InsertHeadList(Thread->MutantListHead.Blink,                    \
                           &(Object)->MutantListEntry);                     \
        }                                                                   \
    }                                                                       \
    else if (((Object)->Header.Type & TIMER_OR_EVENT_TYPE) ==               \
             EventSynchronizationObject)                                    \
    {                                                                       \
        /* Synchronization Timers and Events just get un-signaled */        \
        (Object)->Header.SignalState = 0;                                   \
    }                                                                       \
    else if ((Object)->Header.Type == SemaphoreObject)                      \
    {                                                                       \
        /* These ones can have multiple states, so we only decrease it */   \
        (Object)->Header.SignalState--;                                     \
    }                                                                       \
}

//
// Satisfies the wait of a mutant dispatcher object
//
#define KiSatisfyMutantWait(Object, Thread)                                 \
{                                                                           \
    /* Decrease the Signal State */                                         \
    (Object)->Header.SignalState--;                                         \
                                                                            \
    /* Check if it's now non-signaled */                                    \
    if (!(Object)->Header.SignalState)                                      \
    {                                                                       \
        /* Set the Owner Thread */                                          \
        (Object)->OwnerThread = Thread;                                     \
                                                                            \
        /* Disable APCs if needed */                                        \
        Thread->KernelApcDisable -= (Object)->ApcDisable;                   \
                                                                            \
        /* Check if it's abandoned */                                       \
        if ((Object)->Abandoned)                                            \
        {                                                                   \
            /* Unabandon it */                                              \
            (Object)->Abandoned = FALSE;                                    \
                                                                            \
            /* Return Status */                                             \
            Thread->WaitStatus = STATUS_ABANDONED;                          \
        }                                                                   \
                                                                            \
        /* Insert it into the Mutant List */                                \
        InsertHeadList(Thread->MutantListHead.Blink,                        \
                       &(Object)->MutantListEntry);                         \
    }                                                                       \
}

//
// Satisfies the wait of any nonmutant dispatcher object
//
#define KiSatisfyNonMutantWait(Object, Thread)                              \
{                                                                           \
    if (((Object)->Header.Type & TIMER_OR_EVENT_TYPE) ==                    \
             EventSynchronizationObject)                                    \
    {                                                                       \
        /* Synchronization Timers and Events just get un-signaled */        \
        (Object)->Header.SignalState = 0;                                   \
    }                                                                       \
    else if ((Object)->Header.Type == SemaphoreObject)                      \
    {                                                                       \
        /* These ones can have multiple states, so we only decrease it */   \
        (Object)->Header.SignalState--;                                     \
    }                                                                       \
}

//
// Rules for checking alertability:
//  - For Alertable waits ONLY:
//      * We don't wait and return STATUS_ALERTED if the thread is alerted
//        in EITHER the specified wait mode OR in Kernel Mode.
//  - For BOTH Alertable AND Non-Alertable waits:
//      * We don't want and return STATUS_USER_APC if the User Mode APC list
//        is not empty AND the wait mode is User Mode.
//
#define KiCheckAlertability()                                               \
{                                                                           \
    if (Alertable)                                                          \
    {                                                                       \
        if (CurrentThread->Alerted[(int)WaitMode])                          \
        {                                                                   \
            CurrentThread->Alerted[(int)WaitMode] = FALSE;                  \
            WaitStatus = STATUS_ALERTED;                                    \
            break;                                                          \
        }                                                                   \
        else if ((WaitMode != KernelMode) &&                                \
                (!IsListEmpty(&CurrentThread->                              \
                              ApcState.ApcListHead[UserMode])))             \
        {                                                                   \
            CurrentThread->ApcState.UserApcPending = TRUE;                  \
            WaitStatus = STATUS_USER_APC;                                   \
            break;                                                          \
        }                                                                   \
        else if (CurrentThread->Alerted[KernelMode])                        \
        {                                                                   \
            CurrentThread->Alerted[KernelMode] = FALSE;                     \
            WaitStatus = STATUS_ALERTED;                                    \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    else if ((WaitMode != KernelMode) &&                                    \
             (CurrentThread->ApcState.UserApcPending))                      \
    {                                                                       \
        WaitStatus = STATUS_USER_APC;                                       \
        break;                                                              \
    }                                                                       \
}
