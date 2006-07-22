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
// Recalculates the due time
//
PLARGE_INTEGER
FORCEINLINE
KiRecalculateDueTime(IN PLARGE_INTEGER OriginalDueTime,
                     IN PLARGE_INTEGER DueTime,
                     IN OUT PLARGE_INTEGER NewDueTime)
{
    /* Don't do anything for absolute waits */
    if (OriginalDueTime->QuadPart >= 0) return OriginalDueTime;

    /* Otherwise, query the interrupt time and recalculate */
    NewDueTime->QuadPart = KeQueryInterruptTime();
    NewDueTime->QuadPart -= DueTime->QuadPart;
    return NewDueTime;
}

//
// Determines wether a thread should be added to the wait list
//
#define KiCheckThreadStackSwap(WaitMode, Thread, Swappable)                 \
{                                                                           \
    /* Check the required conditions */                                     \
    if ((WaitMode != KernelMode) &&                                         \
        (Thread->EnableStackSwap) &&                                        \
        (Thread->Priority >= (LOW_REALTIME_PRIORITY + 9)))                  \
    {                                                                       \
        /* We are go for swap */                                            \
        Swappable = TRUE;                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* Don't swap the thread */                                         \
        Swappable = FALSE;                                                  \
    }                                                                       \
}

//
// Adds a thread to the wait list
//
#define KiAddThreadToWaitList(Thread, Swappable)                            \
{                                                                           \
    /* Make sure it's swappable */                                          \
    if (Swappable)                                                          \
    {                                                                       \
        /* Insert it into the PRCB's List */                                \
        InsertTailList(&KeGetCurrentPrcb()->WaitListHead,                   \
                       &Thread->WaitListEntry);                             \
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
