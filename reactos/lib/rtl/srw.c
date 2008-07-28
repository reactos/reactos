/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Slim Reader/Writer (SRW) Routines
 * PROGRAMMER:        Thomas Weidenmueller <w3seek@reactos.com>
 *
 * NOTES:             The algorithms used in this implementation
 *                    may be different from Vista's implementation.
 *                    Since applications should treat the RTL_SRWLOCK
 *                    structure as opaque data, it should not matter.
 *                    The algorithms are probably not as optimized.
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#ifdef _WIN64
#define InterlockedBitTestAndSetPointer(ptr,val) InterlockedBitTestAndSet64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedAddPointer(ptr,val) InterlockedAdd64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedAndPointer(ptr,val) InterlockedAnd64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedOrPointer(ptr,val) InterlockedOr64((PLONGLONG)ptr,(LONGLONG)val)
#else
#define InterlockedBitTestAndSetPointer(ptr,val) InterlockedBitTestAndSet((PLONG)ptr,(LONG)val)
#define InterlockedAddPointer(ptr,val) InterlockedAdd((PLONG)ptr,(LONG)val)
#define InterlockedAndPointer(ptr,val) InterlockedAnd((PLONG)ptr,(LONG)val)
#define InterlockedOrPointer(ptr,val) InterlockedOr((PLONG)ptr,(LONG)val)
#endif

#define RTL_SRWLOCK_OWNED_BIT   0
#define RTL_SRWLOCK_CONTENDED_BIT   1
#define RTL_SRWLOCK_SHARED_BIT  2
#define RTL_SRWLOCK_CONTENTION_LOCK_BIT 3
#define RTL_SRWLOCK_OWNED   (1 << RTL_SRWLOCK_OWNED_BIT)
#define RTL_SRWLOCK_CONTENDED   (1 << RTL_SRWLOCK_CONTENDED_BIT)
#define RTL_SRWLOCK_SHARED  (1 << RTL_SRWLOCK_SHARED_BIT)
#define RTL_SRWLOCK_CONTENTION_LOCK (1 << RTL_SRWLOCK_CONTENTION_LOCK_BIT)
#define RTL_SRWLOCK_MASK    (RTL_SRWLOCK_OWNED | RTL_SRWLOCK_CONTENDED | \
                             RTL_SRWLOCK_SHARED | RTL_SRWLOCK_CONTENTION_LOCK)
#define RTL_SRWLOCK_BITS    4

#if defined(__GNUC__) && !defined(_M_AMD64)
/* This macro will cause the code to assert if compiled with a buggy
   version of GCC that doesn't align the wait blocks properly on the stack! */
#define ASSERT_SRW_WAITBLOCK(ptr) \
    ASSERT(((ULONG_PTR)ptr & ((1 << RTL_SRWLOCK_BITS) - 1)) == 0)
#else
#define ASSERT_SRW_WAITBLOCK(ptr)
#endif

typedef struct _RTLP_SRWLOCK_SHARED_WAKE
{
    LONG Wake;
    volatile struct _RTLP_SRWLOCK_SHARED_WAKE *Next;
} volatile RTLP_SRWLOCK_SHARED_WAKE, *PRTLP_SRWLOCK_SHARED_WAKE;

typedef struct _RTLP_SRWLOCK_WAITBLOCK
{
    /* SharedCount is the number of shared acquirers. */
    LONG SharedCount;

    /* Last points to the last wait block in the chain. The value
       is only valid when read from the first wait block. */
    volatile struct _RTLP_SRWLOCK_WAITBLOCK *Last;

    /* Next points to the next wait block in the chain. */
    volatile struct _RTLP_SRWLOCK_WAITBLOCK *Next;

    union
    {
        /* Wake is only valid for exclusive wait blocks */
        LONG Wake;
        /* The wake chain is only valid for shared wait blocks */
        struct
        {
            PRTLP_SRWLOCK_SHARED_WAKE SharedWakeChain;
            PRTLP_SRWLOCK_SHARED_WAKE LastSharedWake;
        };
    };

    BOOLEAN Exclusive;
} volatile RTLP_SRWLOCK_WAITBLOCK, *PRTLP_SRWLOCK_WAITBLOCK;


static VOID
NTAPI
RtlpReleaseWaitBlockLockExclusive(IN OUT PRTL_SRWLOCK SRWLock,
                                  IN PRTLP_SRWLOCK_WAITBLOCK FirstWaitBlock)
{
    PRTLP_SRWLOCK_WAITBLOCK Next;
    LONG_PTR NewValue;

    /* NOTE: We're currently in an exclusive lock in contended mode. */

    Next = FirstWaitBlock->Next;
    if (Next != NULL)
    {
        /* There's more blocks chained, we need to update the pointers
           in the next wait block and update the wait block pointer. */
        NewValue = (LONG_PTR)Next | RTL_SRWLOCK_OWNED | RTL_SRWLOCK_CONTENDED;
        if (!FirstWaitBlock->Exclusive)
        {
            /* The next wait block has to be an exclusive lock! */
            ASSERT(Next->Exclusive);

            /* Save the shared count */
            Next->SharedCount = FirstWaitBlock->SharedCount;

            NewValue |= RTL_SRWLOCK_SHARED;
        }

        Next->Last = FirstWaitBlock->Last;
    }
    else
    {
        /* Convert the lock to a simple lock. */
        if (FirstWaitBlock->Exclusive)
            NewValue = RTL_SRWLOCK_OWNED;
        else
        {
            ASSERT(FirstWaitBlock->SharedCount > 0);

            NewValue = ((LONG_PTR)FirstWaitBlock->SharedCount << RTL_SRWLOCK_BITS) |
                       RTL_SRWLOCK_SHARED | RTL_SRWLOCK_OWNED;
        }
    }

    (void)_InterlockedExchange((PLONG)&SRWLock->Ptr, NewValue);

    if (FirstWaitBlock->Exclusive)
    {
        (void)InterlockedOr(&FirstWaitBlock->Wake,
                            TRUE);
    }
    else
    {
        PRTLP_SRWLOCK_SHARED_WAKE WakeChain, Next;

        /* If we were the first one to acquire the shared
           lock, we now need to wake all others... */
        WakeChain = FirstWaitBlock->SharedWakeChain;
        do
        {
            Next = WakeChain->Next;

            (void)InterlockedOr((PLONG)&WakeChain->Wake,
                                TRUE);

            WakeChain = Next;
        } while (WakeChain != NULL);
    }
}


static VOID
NTAPI
RtlpReleaseWaitBlockLockLastShared(IN OUT PRTL_SRWLOCK SRWLock,
                                   IN PRTLP_SRWLOCK_WAITBLOCK FirstWaitBlock)
{
    PRTLP_SRWLOCK_WAITBLOCK Next;
    LONG_PTR NewValue;

    /* NOTE: We're currently in a shared lock in contended mode. */

    /* The next acquirer to be unwaited *must* be an exclusive lock! */
    ASSERT(FirstWaitBlock->Exclusive);

    Next = FirstWaitBlock->Next;
    if (Next != NULL)
    {
        /* There's more blocks chained, we need to update the pointers
           in the next wait block and update the wait block pointer. */
        NewValue = (LONG_PTR)Next | RTL_SRWLOCK_OWNED | RTL_SRWLOCK_CONTENDED;

        Next->Last = FirstWaitBlock->Last;
    }
    else
    {
        /* Convert the lock to a simple exclusive lock. */
        NewValue = RTL_SRWLOCK_OWNED;
    }

    (void)_InterlockedExchange((PLONG)&SRWLock->Ptr, NewValue);

    (void)InterlockedOr(&FirstWaitBlock->Wake,
                        TRUE);
}


static VOID
NTAPI
RtlpReleaseWaitBlockLock(IN OUT PRTL_SRWLOCK SRWLock)
{
    InterlockedAndPointer(&SRWLock->Ptr,
                          ~RTL_SRWLOCK_CONTENTION_LOCK);
}


static PRTLP_SRWLOCK_WAITBLOCK
NTAPI
RtlpAcquireWaitBlockLock(IN OUT PRTL_SRWLOCK SRWLock)
{
    LONG_PTR PrevValue;
    PRTLP_SRWLOCK_WAITBLOCK WaitBlock;

    while (1)
    {
        PrevValue = InterlockedOrPointer(&SRWLock->Ptr,
                                         RTL_SRWLOCK_CONTENTION_LOCK);

        if (!(PrevValue & RTL_SRWLOCK_CONTENTION_LOCK))
            break;

        YieldProcessor();
    }

    if (!(PrevValue & RTL_SRWLOCK_CONTENDED) ||
        (PrevValue & ~RTL_SRWLOCK_MASK) == 0)
    {
        /* Too bad, looks like the wait block was removed in the
           meanwhile, unlock again */
        RtlpReleaseWaitBlockLock(SRWLock);
        return NULL;
    }

    WaitBlock = (PRTLP_SRWLOCK_WAITBLOCK)(PrevValue & ~RTL_SRWLOCK_MASK);

    ASSERT_SRW_WAITBLOCK(WaitBlock);
    return WaitBlock;
}


static VOID
NTAPI
RtlpAcquireSRWLockExclusiveWait(IN OUT PRTL_SRWLOCK SRWLock,
                                IN PRTLP_SRWLOCK_WAITBLOCK WaitBlock)
{
    LONG_PTR CurrentValue;

    while (1)
    {
        CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;
        if (!(CurrentValue & RTL_SRWLOCK_SHARED))
        {
            if (CurrentValue & RTL_SRWLOCK_CONTENDED)
            {
                if (WaitBlock->Wake != 0)
                {
                    /* Our wait block became the first one
                       in the chain, we own the lock now! */
                    break;
                }
            }
            else
            {
                /* The last wait block was removed and/or we're
                   finally a simple exclusive lock. This means we
                   don't need to wait anymore, we acquired the lock! */
                break;
            }
        }

        YieldProcessor();
    }
}


static VOID
NTAPI
RtlpAcquireSRWLockSharedWait(IN OUT PRTL_SRWLOCK SRWLock,
                             IN OUT PRTLP_SRWLOCK_WAITBLOCK FirstWait  OPTIONAL,
                             IN OUT PRTLP_SRWLOCK_SHARED_WAKE WakeChain)
{
    if (FirstWait != NULL)
    {
        while (WakeChain->Wake == 0)
        {
            YieldProcessor();
        }
    }
    else
    {
        LONG_PTR CurrentValue;

        while (1)
        {
            CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;
            if (CurrentValue & RTL_SRWLOCK_SHARED)
            {
                /* The RTL_SRWLOCK_OWNED bit always needs to be set when
                   RTL_SRWLOCK_SHARED is set! */
                ASSERT(CurrentValue & RTL_SRWLOCK_OWNED);

                if (CurrentValue & RTL_SRWLOCK_CONTENDED)
                {
                    if (WakeChain->Wake != 0)
                    {
                        /* Our wait block became the first one
                           in the chain, we own the lock now! */
                        break;
                    }
                }
                else
                {
                    /* The last wait block was removed and/or we're
                       finally a simple shared lock. This means we
                       don't need to wait anymore, we acquired the lock! */
                    break;
                }
            }

            YieldProcessor();
        }
    }
}


VOID
NTAPI
RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock)
{
    SRWLock->Ptr = NULL;
}


VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock)
{
    __ALIGNED(16) RTLP_SRWLOCK_WAITBLOCK StackWaitBlock;
    RTLP_SRWLOCK_SHARED_WAKE SharedWake;
    LONG_PTR CurrentValue, NewValue;
    PRTLP_SRWLOCK_WAITBLOCK First, Shared, FirstWait;

    while (1)
    {
        CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;

        if (CurrentValue & RTL_SRWLOCK_SHARED)
        {
            /* NOTE: It is possible that the RTL_SRWLOCK_OWNED bit is set! */

            if (CurrentValue & RTL_SRWLOCK_CONTENDED)
            {
                /* There's other waiters already, lock the wait blocks and
                   increment the shared count */
                First = RtlpAcquireWaitBlockLock(SRWLock);
                if (First != NULL)
                {
                    FirstWait = NULL;

                    if (First->Exclusive)
                    {
                        /* We need to setup a new wait block! Although
                           we're currently in a shared lock and we're acquiring
                           a shared lock, there are exclusive locks queued. We need
                           to wait until those are released. */
                        Shared = First->Last;

                        if (Shared->Exclusive)
                        {
                            StackWaitBlock.Exclusive = FALSE;
                            StackWaitBlock.SharedCount = 1;
                            StackWaitBlock.Next = NULL;
                            StackWaitBlock.Last = &StackWaitBlock;
                            StackWaitBlock.SharedWakeChain = &SharedWake;

                            Shared->Next = &StackWaitBlock;
                            First->Last = &StackWaitBlock;

                            Shared = &StackWaitBlock;
                            FirstWait = &StackWaitBlock;
                        }
                        else
                        {
                            Shared->LastSharedWake->Next = &SharedWake;
                            Shared->SharedCount++;
                        }
                    }
                    else
                    {
                        Shared = First;
                        Shared->LastSharedWake->Next = &SharedWake;
                        Shared->SharedCount++;
                    }

                    SharedWake.Next = NULL;
                    SharedWake.Wake = 0;

                    Shared->LastSharedWake = &SharedWake;

                    ASSERT_SRW_WAITBLOCK(Shared);

                    RtlpReleaseWaitBlockLock(SRWLock);

                    RtlpAcquireSRWLockSharedWait(SRWLock,
                                                 FirstWait,
                                                 &SharedWake);

                    /* Successfully incremented the shared count, we acquired the lock */
                    break;
                }
            }
            else
            {
                /* This is a fastest path, just increment the number of
                   current shared locks */

                /* Since the RTL_SRWLOCK_SHARED bit is set, the RTL_SRWLOCK_OWNED bit also has
                   to be set! */

                ASSERT(CurrentValue & RTL_SRWLOCK_OWNED);

                NewValue = (CurrentValue >> RTL_SRWLOCK_BITS) + 1;
                NewValue = (NewValue << RTL_SRWLOCK_BITS) | (CurrentValue & RTL_SRWLOCK_MASK);

                if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                NewValue,
                                                CurrentValue) == CurrentValue)
                {
                    /* Successfully incremented the shared count, we acquired the lock */
                    break;
                }
            }
        }
        else
        {
            if (CurrentValue & RTL_SRWLOCK_OWNED)
            {
                /* The resource is currently acquired exclusively */
                if (CurrentValue & RTL_SRWLOCK_CONTENDED)
                {
                    SharedWake.Next = NULL;
                    SharedWake.Wake = 0;

                    /* There's other waiters already, lock the wait blocks and
                       increment the shared count. If the last block in the chain
                       is an exclusive lock, add another block. */

                    StackWaitBlock.Exclusive = FALSE;
                    StackWaitBlock.SharedCount = 0;
                    StackWaitBlock.Next = NULL;
                    StackWaitBlock.Last = &StackWaitBlock;
                    StackWaitBlock.SharedWakeChain = &SharedWake;

                    First = RtlpAcquireWaitBlockLock(SRWLock);
                    if (First != NULL)
                    {
                        Shared = First->Last;
                        if (Shared->Exclusive)
                        {
                            Shared->Next = &StackWaitBlock;
                            First->Last = &StackWaitBlock;

                            Shared = &StackWaitBlock;
                            FirstWait = &StackWaitBlock;
                        }
                        else
                        {
                            FirstWait = NULL;
                            Shared->LastSharedWake->Next = &SharedWake;
                        }

                        ASSERT_SRW_WAITBLOCK(Shared);

                        Shared->SharedCount++;
                        Shared->LastSharedWake = &SharedWake;

                        RtlpReleaseWaitBlockLock(SRWLock);

                        RtlpAcquireSRWLockSharedWait(SRWLock,
                                                     FirstWait,
                                                     &SharedWake);

                        /* Successfully incremented the shared count, we acquired the lock */
                        break;
                    }
                }
                else
                {
                    SharedWake.Next = NULL;
                    SharedWake.Wake = 0;

                    /* We need to setup the first wait block. Currently an exclusive lock is
                       held, change the lock to contended mode. */
                    StackWaitBlock.Exclusive = FALSE;
                    StackWaitBlock.SharedCount = 1;
                    StackWaitBlock.Next = NULL;
                    StackWaitBlock.Last = &StackWaitBlock;
                    StackWaitBlock.SharedWakeChain = &SharedWake;
                    StackWaitBlock.LastSharedWake = &SharedWake;

                    ASSERT_SRW_WAITBLOCK(&StackWaitBlock);

                    NewValue = (ULONG_PTR)&StackWaitBlock | RTL_SRWLOCK_OWNED | RTL_SRWLOCK_CONTENDED;
                    if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                    NewValue,
                                                    CurrentValue) == CurrentValue)
                    {
                        RtlpAcquireSRWLockSharedWait(SRWLock,
                                                     &StackWaitBlock,
                                                     &SharedWake);

                        /* Successfully set the shared count, we acquired the lock */
                        break;
                    }
                }
            }
            else
            {
                /* This is a fast path, we can simply try to set the shared count to 1 */
                NewValue = (1 << RTL_SRWLOCK_BITS) | RTL_SRWLOCK_SHARED | RTL_SRWLOCK_OWNED;

                /* The RTL_SRWLOCK_CONTENDED bit should never be set if neither the
                   RTL_SRWLOCK_SHARED nor the RTL_SRWLOCK_OWNED bit is set */
                ASSERT(!(CurrentValue & RTL_SRWLOCK_CONTENDED));

                if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                NewValue,
                                                CurrentValue) == CurrentValue)
                {
                    /* Successfully set the shared count, we acquired the lock */
                    break;
                }
            }
        }

        YieldProcessor();
    }
}


VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock)
{
    LONG_PTR CurrentValue, NewValue;
    PRTLP_SRWLOCK_WAITBLOCK WaitBlock;
    BOOLEAN LastShared;

    while (1)
    {
        CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;

        if (CurrentValue & RTL_SRWLOCK_SHARED)
        {
            if (CurrentValue & RTL_SRWLOCK_CONTENDED)
            {
                /* There's a wait block, we need to wake a pending
                   exclusive acquirer if this is the last shared release */
                WaitBlock = RtlpAcquireWaitBlockLock(SRWLock);
                if (WaitBlock != NULL)
                {
                    LastShared = (--WaitBlock->SharedCount == 0);

                    if (LastShared)
                        RtlpReleaseWaitBlockLockLastShared(SRWLock,
                                                           WaitBlock);
                    else
                        RtlpReleaseWaitBlockLock(SRWLock);

                    /* We released the lock */
                    break;
                }
            }
            else
            {
                /* This is a fast path, we can simply decrement the shared
                   count and store the pointer */
                NewValue = CurrentValue >> RTL_SRWLOCK_BITS;

                if (--NewValue != 0)
                {
                    NewValue = (NewValue << RTL_SRWLOCK_BITS) | RTL_SRWLOCK_SHARED | RTL_SRWLOCK_OWNED;
                }

                if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                NewValue,
                                                CurrentValue) == CurrentValue)
                {
                    /* Successfully released the lock */
                    break;
                }
            }
        }
        else
        {
            /* The RTL_SRWLOCK_SHARED bit has to be present now,
               even in the contended case! */
            RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
        }

        YieldProcessor();
    }
}


VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock)
{
    __ALIGNED(16) RTLP_SRWLOCK_WAITBLOCK StackWaitBlock;
    PRTLP_SRWLOCK_WAITBLOCK First, Last;

    if (InterlockedBitTestAndSetPointer(&SRWLock->Ptr,
                                        RTL_SRWLOCK_OWNED_BIT))
    {
        LONG_PTR CurrentValue, NewValue;

        while (1)
        {
            CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;

            if (CurrentValue & RTL_SRWLOCK_SHARED)
            {
                /* A shared lock is being held right now. We need to add a wait block! */

                if (CurrentValue & RTL_SRWLOCK_CONTENDED)
                {
                    goto AddWaitBlock;
                }
                else
                {
                    /* There are no wait blocks so far, we need to add ourselves as the first
                       wait block. We need to keep the shared count! */
                    StackWaitBlock.Exclusive = TRUE;
                    StackWaitBlock.SharedCount = CurrentValue >> RTL_SRWLOCK_BITS;
                    StackWaitBlock.Next = NULL;
                    StackWaitBlock.Last = &StackWaitBlock;
                    StackWaitBlock.Wake = 0;

                    ASSERT_SRW_WAITBLOCK(&StackWaitBlock);

                    NewValue = (ULONG_PTR)&StackWaitBlock | RTL_SRWLOCK_SHARED | RTL_SRWLOCK_CONTENDED | RTL_SRWLOCK_OWNED;

                    if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                    NewValue,
                                                    CurrentValue) == CurrentValue)
                    {
                        RtlpAcquireSRWLockExclusiveWait(SRWLock,
                                                        &StackWaitBlock);

                        /* Successfully acquired the exclusive lock */
                        break;
                    }
                }
            }
            else
            {
                if (CurrentValue & RTL_SRWLOCK_OWNED)
                {
                    /* An exclusive lock is being held right now. We need to add a wait block! */

                    if (CurrentValue & RTL_SRWLOCK_CONTENDED)
                    {
AddWaitBlock:
                        StackWaitBlock.Exclusive = TRUE;
                        StackWaitBlock.SharedCount = 0;
                        StackWaitBlock.Next = NULL;
                        StackWaitBlock.Last = &StackWaitBlock;
                        StackWaitBlock.Wake = 0;

                        ASSERT_SRW_WAITBLOCK(&StackWaitBlock);

                        First = RtlpAcquireWaitBlockLock(SRWLock);
                        if (First != NULL)
                        {
                            Last = First->Last;
                            Last->Next = &StackWaitBlock;
                            First->Last = &StackWaitBlock;

                            RtlpReleaseWaitBlockLock(SRWLock);

                            RtlpAcquireSRWLockExclusiveWait(SRWLock,
                                                            &StackWaitBlock);

                            /* Successfully acquired the exclusive lock */
                            break;
                        }
                    }
                    else
                    {
                        /* There are no wait blocks so far, we need to add ourselves as the first
                           wait block. We need to keep the shared count! */
                        StackWaitBlock.Exclusive = TRUE;
                        StackWaitBlock.SharedCount = 0;
                        StackWaitBlock.Next = NULL;
                        StackWaitBlock.Last = &StackWaitBlock;
                        StackWaitBlock.Wake = 0;

                        ASSERT_SRW_WAITBLOCK(&StackWaitBlock);

                        NewValue = (ULONG_PTR)&StackWaitBlock | RTL_SRWLOCK_OWNED | RTL_SRWLOCK_CONTENDED;
                        if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                        NewValue,
                                                        CurrentValue) == CurrentValue)
                        {
                            RtlpAcquireSRWLockExclusiveWait(SRWLock,
                                                            &StackWaitBlock);

                            /* Successfully acquired the exclusive lock */
                            break;
                        }
                    }
                }
                else
                {
                    if (!InterlockedBitTestAndSetPointer(&SRWLock->Ptr,
                                                         RTL_SRWLOCK_OWNED_BIT))
                    {
                        /* We managed to get hold of a simple exclusive lock! */
                        break;
                    }
                }
            }

            YieldProcessor();
        }
    }
}


VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock)
{
    LONG_PTR CurrentValue, NewValue;
    PRTLP_SRWLOCK_WAITBLOCK WaitBlock;

    while (1)
    {
        CurrentValue = *(volatile LONG_PTR *)&SRWLock->Ptr;

        if (!(CurrentValue & RTL_SRWLOCK_OWNED))
        {
            RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
        }

        if (!(CurrentValue & RTL_SRWLOCK_SHARED))
        {
            if (CurrentValue & RTL_SRWLOCK_CONTENDED)
            {
                /* There's a wait block, we need to wake the next pending
                   acquirer (exclusive or shared) */
                WaitBlock = RtlpAcquireWaitBlockLock(SRWLock);
                if (WaitBlock != NULL)
                {
                    RtlpReleaseWaitBlockLockExclusive(SRWLock,
                                                      WaitBlock);

                    /* We released the lock */
                    break;
                }
            }
            else
            {
                /* This is a fast path, we can simply clear the RTL_SRWLOCK_OWNED
                   bit. All other bits should be 0 now because this is a simple
                   exclusive lock and no one is waiting. */

                ASSERT(!(CurrentValue & ~RTL_SRWLOCK_OWNED));

                NewValue = 0;
                if (_InterlockedCompareExchange((PLONG)&SRWLock->Ptr,
                                                NewValue,
                                                CurrentValue) == CurrentValue)
                {
                    /* We released the lock */
                    break;
                }
            }
        }
        else
        {
            /* The RTL_SRWLOCK_SHARED bit must not be present now,
               not even in the contended case! */
            RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
        }

        YieldProcessor();
    }
}
