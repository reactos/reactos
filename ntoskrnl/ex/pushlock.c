/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/pushlock.c
 * PURPOSE:         Pushlock and Cache-Aware Pushlock Implementation
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

ULONG ExPushLockSpinCount = 0;

#undef EX_PUSH_LOCK
#undef PEX_PUSH_LOCK

/* PRIVATE FUNCTIONS *********************************************************/

#ifdef _WIN64
#define InterlockedAndPointer(ptr,val) InterlockedAnd64((PLONGLONG)ptr,(LONGLONG)val)
#else
#define InterlockedAndPointer(ptr,val) InterlockedAnd((PLONG)ptr,(LONG)val)
#endif

/*++
 * @name ExpInitializePushLocks
 *
 *     The ExpInitializePushLocks routine initialized Pushlock support.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks The ExpInitializePushLocks routine sets up the spin on SMP machines.
 *
 *--*/
CODE_SEG("INIT")
VOID
NTAPI
ExpInitializePushLocks(VOID)
{
#ifdef CONFIG_SMP
    /* Initialize an internal 1024-iteration spin for MP CPUs */
    if (KeNumberProcessors > 1)
        ExPushLockSpinCount = 1024;
#endif
}

/*++
 * @name ExfWakePushLock
 *
 *     The ExfWakePushLock routine wakes a Pushlock that is in the waiting
 *     state.
 *
 * @param PushLock
 *        Pointer to a pushlock that is waiting.
 *
 * @param OldValue
 *        Last known value of the pushlock before this routine was called.
 *
 * @return None.
 *
 * @remarks This is an internal routine; do not call it manually. Only the system
 *          can properly know if the pushlock is ready to be awakened or not.
 *          External callers should use ExfTrytoWakePushLock.
 *
 *--*/
VOID
FASTCALL
ExfWakePushLock(PEX_PUSH_LOCK PushLock,
                EX_PUSH_LOCK OldValue)
{
    EX_PUSH_LOCK NewValue;
    PEX_PUSH_LOCK_WAIT_BLOCK PreviousWaitBlock, FirstWaitBlock, LastWaitBlock;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock;
    KIRQL OldIrql;

    /* Start main wake loop */
    for (;;)
    {
        /* Sanity checks */
        ASSERT(!OldValue.MultipleShared);

        /* Check if it's locked */
        while (OldValue.Locked)
        {
            /* It's not waking anymore */
            NewValue.Value = OldValue.Value &~ EX_PUSH_LOCK_WAKING;

            /* Sanity checks */
            ASSERT(!NewValue.Waking);
            ASSERT(NewValue.Locked);
            ASSERT(NewValue.Waiting);

            /* Write the New Value */
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value == OldValue.Value) return;

            /* Someone changed the value behind our back, update it*/
            OldValue = NewValue;
        }

        /* Save the First Block */
        FirstWaitBlock = (PEX_PUSH_LOCK_WAIT_BLOCK)(OldValue.Value &
                          ~EX_PUSH_LOCK_PTR_BITS);
        WaitBlock = FirstWaitBlock;

        /* Try to find the last block */
        while (TRUE)
        {
            /* Get the last wait block */
            LastWaitBlock = WaitBlock->Last;

            /* Check if we found it */
            if (LastWaitBlock)
            {
                /* Use it */
                WaitBlock = LastWaitBlock;
                break;
            }

            /* Save the previous block */
            PreviousWaitBlock = WaitBlock;

            /* Move to next block */
            WaitBlock = WaitBlock->Next;

            /* Save the previous block */
            WaitBlock->Previous = PreviousWaitBlock;
        }

        /* Check if the last Wait Block is not Exclusive or if it's the only one */
        PreviousWaitBlock = WaitBlock->Previous;
        if (!(WaitBlock->Flags & EX_PUSH_LOCK_FLAGS_EXCLUSIVE) ||
            !(PreviousWaitBlock))
        {
            /* Destroy the pushlock */
            NewValue.Value = 0;
            ASSERT(!NewValue.Waking);

            /* Write the New Value */
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value == OldValue.Value) break;

            /* Someone changed the value behind our back, update it*/
            OldValue = NewValue;
        }
        else
        {
            /* Link the wait blocks */
            FirstWaitBlock->Last = PreviousWaitBlock;
            WaitBlock->Previous = NULL;

            /* Sanity checks */
            ASSERT(FirstWaitBlock != WaitBlock);
            ASSERT(PushLock->Waiting);

            /* Remove waking bit from pushlock */
            InterlockedAndPointer(&PushLock->Value, ~EX_PUSH_LOCK_WAKING);

            /* Leave the loop */
            break;
        }
    }

    /* Check if there's a previous block */
    OldIrql = DISPATCH_LEVEL;
    if (WaitBlock->Previous)
    {
        /* Raise to Dispatch */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }

    /* Signaling loop */
    for (;;)
    {
        /* Get the previous Wait block */
        PreviousWaitBlock = WaitBlock->Previous;

        /* Sanity check */
        ASSERT(!WaitBlock->Signaled);

#if DBG
        /* We are about to get signaled */
        WaitBlock->Signaled = TRUE;
#endif

        /* Set the Wait Bit in the Wait Block */
        if (!InterlockedBitTestAndReset(&WaitBlock->Flags, 1))
        {
            /* Nobody signaled us, so do it */
            KeSignalGateBoostPriority(&WaitBlock->WakeGate);
        }

        /* Set the wait block and check if there still is one to loop*/
        WaitBlock = PreviousWaitBlock;
        if (!WaitBlock) break;
    }

    /* Check if we have to lower back the IRQL */
    if (OldIrql != DISPATCH_LEVEL) KeLowerIrql(OldIrql);
}

/*++
 * @name ExpOptimizePushLockList
 *
 *     The ExpOptimizePushLockList routine optimizes the list of waiters
 *     associated to a pushlock's wait block.
 *
 * @param PushLock
 *        Pointer to a pushlock whose waiter list needs to be optimized.
 *
 * @param OldValue
 *        Last known value of the pushlock before this routine was called.
 *
 * @return None.
 *
 * @remarks At the end of the optimization, the pushlock will also be wakened.
 *
 *--*/
VOID
FASTCALL
ExpOptimizePushLockList(PEX_PUSH_LOCK PushLock,
                        EX_PUSH_LOCK OldValue)
{
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock, LastWaitBlock, PreviousWaitBlock, FirstWaitBlock;
    EX_PUSH_LOCK NewValue;

    /* Start main loop */
    for (;;)
    {
        /* Check if we've been unlocked */
        if (!OldValue.Locked)
        {
            /* Wake us up and leave */
            ExfWakePushLock(PushLock, OldValue);
            break;
        }

        /* Get the wait block */
        WaitBlock = (PEX_PUSH_LOCK_WAIT_BLOCK)(OldValue.Value &
                                               ~EX_PUSH_LOCK_PTR_BITS);

        /* Loop the blocks */
        FirstWaitBlock = WaitBlock;
        while (TRUE)
        {
            /* Get the last wait block */
            LastWaitBlock = WaitBlock->Last;
            if (LastWaitBlock)
            {
                /* Set this as the new last block, we're done */
                FirstWaitBlock->Last = LastWaitBlock;
                break;
            }

            /* Save the block */
            PreviousWaitBlock = WaitBlock;

            /* Get the next block */
            WaitBlock = WaitBlock->Next;

            /* Save the previous */
            WaitBlock->Previous = PreviousWaitBlock;
        }

        /* Remove the wake bit */
        NewValue.Value = OldValue.Value &~ EX_PUSH_LOCK_WAKING;

        /* Sanity checks */
        ASSERT(NewValue.Locked);
        ASSERT(!NewValue.Waking);

        /* Update the value */
        NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                         NewValue.Ptr,
                                                         OldValue.Ptr);

        /* If we updated correctly, leave */
        if (NewValue.Value == OldValue.Value) break;

        /* Update value */
        OldValue = NewValue;
    }
}

/*++
 * @name ExTimedWaitForUnblockPushLock
 *
 *     The ExTimedWaitForUnblockPushLock routine waits for a pushlock
 *     to be unblocked, for a specified internal.
 *
 * @param PushLock
 *        Pointer to a pushlock whose waiter list needs to be optimized.
 *
 * @param WaitBlock
 *        Pointer to the pushlock's wait block.
 *
 * @param Timeout
 *        Amount of time to wait for this pushlock to be unblocked.
 *
 * @return STATUS_SUCCESS is the pushlock is now unblocked, otherwise the error
 *         code returned by KeWaitForSingleObject.
 *
 * @remarks If the wait fails, then a manual unblock is attempted.
 *
 *--*/
NTSTATUS
FASTCALL
ExTimedWaitForUnblockPushLock(IN PEX_PUSH_LOCK PushLock,
                              IN PVOID WaitBlock,
                              IN PLARGE_INTEGER Timeout)
{
    NTSTATUS Status;

    /* Initialize the wait event */
    KeInitializeEvent(&((PEX_PUSH_LOCK_WAIT_BLOCK)WaitBlock)->WakeEvent,
                      SynchronizationEvent,
                      FALSE);

#ifdef CONFIG_SMP
    /* Spin on the push lock if necessary */
    if (ExPushLockSpinCount)
    {
        ULONG i = ExPushLockSpinCount;

        do
        {
            /* Check if we got lucky and can leave early */
            if (!(*(volatile LONG *)&((PEX_PUSH_LOCK_WAIT_BLOCK)WaitBlock)->Flags & EX_PUSH_LOCK_WAITING))
                return STATUS_SUCCESS;

            YieldProcessor();
        } while (--i);
    }
#endif

    /* Now try to remove the wait bit */
    if (InterlockedBitTestAndReset(&((PEX_PUSH_LOCK_WAIT_BLOCK)WaitBlock)->Flags,
                                   EX_PUSH_LOCK_FLAGS_WAIT_V))
    {
        /* Nobody removed it already, let's do a full wait */
        Status = KeWaitForSingleObject(&((PEX_PUSH_LOCK_WAIT_BLOCK)WaitBlock)->
                                       WakeEvent,
                                       WrPushLock,
                                       KernelMode,
                                       FALSE,
                                       Timeout);
        /* Check if the wait was satisfied */
        if (Status != STATUS_SUCCESS)
        {
            /* Try unblocking the pushlock if it was not */
            ExfUnblockPushLock(PushLock, WaitBlock);
        }
    }
    else
    {
        /* Someone beat us to it, no need to wait */
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

/*++
 * @name ExWaitForUnblockPushLock
 *
 *     The ExWaitForUnblockPushLock routine waits for a pushlock
 *     to be unblocked, for a specified internal.
 *
 * @param PushLock
 *        Pointer to a pushlock whose waiter list needs to be optimized.
 *
 * @param WaitBlock
 *        Pointer to the pushlock's wait block.
 *
 * @return STATUS_SUCCESS is the pushlock is now unblocked, otherwise the error
 *         code returned by KeWaitForSingleObject.
 *
 * @remarks If the wait fails, then a manual unblock is attempted.
 *
 *--*/
VOID
FASTCALL
ExWaitForUnblockPushLock(IN PEX_PUSH_LOCK PushLock,
                         IN PVOID WaitBlock)
{
    /* Call the timed function with no timeout */
    ExTimedWaitForUnblockPushLock(PushLock, WaitBlock, NULL);
}

/*++
 * @name ExBlockPushLock
 *
 *     The ExBlockPushLock routine blocks a pushlock.
 *
 * @param PushLock
 *        Pointer to a pushlock whose waiter list needs to be optimized.
 *
 * @param WaitBlock
 *        Pointer to the pushlock's wait block.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
FASTCALL
ExBlockPushLock(PEX_PUSH_LOCK PushLock,
                PVOID pWaitBlock)
{
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock = pWaitBlock;
    EX_PUSH_LOCK NewValue, OldValue;

    /* Detect invalid wait block alignment */
    ASSERT(((ULONG_PTR)pWaitBlock & 0xF) == 0);

    /* Set the waiting bit */
    WaitBlock->Flags = EX_PUSH_LOCK_FLAGS_WAIT;

    /* Get the old value */
    OldValue = *PushLock;

    /* Start block loop */
    for (;;)
    {
        /* Link the wait blocks */
        WaitBlock->Next = OldValue.Ptr;

        /* Set the new wait block value */
        NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                         WaitBlock,
                                                         OldValue.Ptr);
        if (OldValue.Ptr == NewValue.Ptr) break;

        /* Try again with the new value */
        OldValue = NewValue;
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name ExAcquirePushLockExclusive
 * @implemented NT5.1
 *
 *     The ExAcquirePushLockExclusive macro exclusively acquires a PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be acquired.
 *
 * @return None.
 *
 * @remarks Callers of ExAcquirePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeAcquireCriticalRegion.
 *
 *--*/
VOID
FASTCALL
ExfAcquirePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock, NewValue, TempValue;
    BOOLEAN NeedWake;
    EX_PUSH_LOCK_WAIT_BLOCK Block;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock = &Block;

    /* Start main loop */
    for (;;)
    {
        /* Check if it's unlocked */
        if (!OldValue.Locked)
        {
            /* Lock it */
            NewValue.Value = OldValue.Value | EX_PUSH_LOCK_LOCK;
            ASSERT(NewValue.Locked);

            /* Set the new value */
            if (InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                  NewValue.Ptr,
                                                  OldValue.Ptr) != OldValue.Ptr)
            {
                /* Retry */
                OldValue = *PushLock;
                continue;
            }

            /* Break out of the loop */
            break;
        }
        else
        {
            /* We'll have to create a Waitblock */
            WaitBlock->Flags = EX_PUSH_LOCK_FLAGS_EXCLUSIVE |
                               EX_PUSH_LOCK_FLAGS_WAIT;
            WaitBlock->Previous = NULL;
            NeedWake = FALSE;

            /* Check if there is already a waiter */
            if (OldValue.Waiting)
            {
                /* Nobody is the last waiter yet */
                WaitBlock->Last = NULL;

                /* We are an exclusive waiter */
                WaitBlock->ShareCount = 0;

                /* Set the current Wait Block pointer */
                WaitBlock->Next = (PEX_PUSH_LOCK_WAIT_BLOCK)(
                                   OldValue.Value &~ EX_PUSH_LOCK_PTR_BITS);

                /* Point to ours */
                NewValue.Value = (OldValue.Value & EX_PUSH_LOCK_MULTIPLE_SHARED) |
                                  EX_PUSH_LOCK_LOCK |
                                  EX_PUSH_LOCK_WAKING |
                                  EX_PUSH_LOCK_WAITING |
                                  (ULONG_PTR)WaitBlock;

                /* Check if the pushlock was already waking */
                if (!OldValue.Waking) NeedWake = TRUE;
            }
            else
            {
                /* We are the first waiter, so loop the wait block */
                WaitBlock->Last = WaitBlock;

                /* Set the share count */
                WaitBlock->ShareCount = (LONG)OldValue.Shared;

                /* Check if someone is sharing this pushlock */
                if (OldValue.Shared > 1)
                {
                    /* Point to our wait block */
                    NewValue.Value = EX_PUSH_LOCK_MULTIPLE_SHARED |
                                     EX_PUSH_LOCK_LOCK |
                                     EX_PUSH_LOCK_WAITING |
                                     (ULONG_PTR)WaitBlock;
                }
                else
                {
                    /* No shared count */
                    WaitBlock->ShareCount = 0;

                    /* Point to our wait block */
                    NewValue.Value = EX_PUSH_LOCK_LOCK |
                                     EX_PUSH_LOCK_WAITING |
                                     (ULONG_PTR)WaitBlock;
                }
            }

#if DBG
            /* Setup the Debug Wait Block */
            WaitBlock->Signaled = 0;
            WaitBlock->OldValue = OldValue;
            WaitBlock->NewValue = NewValue;
            WaitBlock->PushLock = PushLock;
#endif

            /* Sanity check */
            ASSERT(NewValue.Waiting);
            ASSERT(NewValue.Locked);

            /* Write the new value */
            TempValue = NewValue;
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value != OldValue.Value)
            {
                /* Retry */
                OldValue = *PushLock;
                continue;
            }

            /* Check if the pushlock needed waking */
            if (NeedWake)
            {
                /* Scan the Waiters and Wake PushLocks */
                ExpOptimizePushLockList(PushLock, TempValue);
            }

            /* Set up the Wait Gate */
            KeInitializeGate(&WaitBlock->WakeGate);

#ifdef CONFIG_SMP
            /* Now spin on the push lock if necessary */
            if (ExPushLockSpinCount)
            {
                ULONG i = ExPushLockSpinCount;

                do
                {
                    if (!(*(volatile LONG *)&WaitBlock->Flags & EX_PUSH_LOCK_WAITING))
                        break;

                    YieldProcessor();
                } while (--i);
            }
#endif

            /* Now try to remove the wait bit */
            if (InterlockedBitTestAndReset(&WaitBlock->Flags, 1))
            {
                /* Nobody removed it already, let's do a full wait */
                KeWaitForGate(&WaitBlock->WakeGate, WrPushLock, KernelMode);
                ASSERT(WaitBlock->Signaled);
            }

            /* We shouldn't be shared anymore */
            ASSERT((WaitBlock->ShareCount == 0));

            /* Loop again */
            OldValue = NewValue;
        }
    }
}

/*++
 * @name ExAcquirePushLockShared
 * @implemented NT5.1
 *
 *     The ExAcquirePushLockShared routine acquires a shared PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be acquired.
 *
 * @return None.
 *
 * @remarks Callers of ExAcquirePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeAcquireCriticalRegion.
 *
 *--*/
VOID
FASTCALL
ExfAcquirePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock, NewValue;
    BOOLEAN NeedWake;
    EX_PUSH_LOCK_WAIT_BLOCK Block;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock = &Block;

    /* Start main loop */
    for (;;)
    {
        /* Check if it's unlocked or if it's waiting without any sharers */
        if (!(OldValue.Locked) || (!(OldValue.Waiting) && (OldValue.Shared > 0)))
        {
            /* Check if anyone is waiting on it */
            if (!OldValue.Waiting)
            {
                /* Increase the share count and lock it */
                NewValue.Value = OldValue.Value | EX_PUSH_LOCK_LOCK;
                NewValue.Shared++;
            }
            else
            {
                /* Simply set the lock bit */
                NewValue.Value = OldValue.Value | EX_PUSH_LOCK_LOCK;
            }

            /* Sanity check */
            ASSERT(NewValue.Locked);

            /* Set the new value */
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value != OldValue.Value)
            {
                /* Retry */
                OldValue = *PushLock;
                continue;
            }

            /* Break out of the loop */
            break;
        }
        else
        {
            /* We'll have to create a Waitblock */
            WaitBlock->Flags = EX_PUSH_LOCK_FLAGS_WAIT;
            WaitBlock->ShareCount = 0;
            NeedWake = FALSE;
            WaitBlock->Previous = NULL;

            /* Check if there is already a waiter */
            if (OldValue.Waiting)
            {
                /* Set the current Wait Block pointer */
                WaitBlock->Next = (PEX_PUSH_LOCK_WAIT_BLOCK)(
                                   OldValue.Value &~ EX_PUSH_LOCK_PTR_BITS);

                /* Nobody is the last waiter yet */
                WaitBlock->Last = NULL;

                /* Point to ours */
                NewValue.Value = (OldValue.Value & (EX_PUSH_LOCK_MULTIPLE_SHARED |
                                                    EX_PUSH_LOCK_LOCK)) |
                                  EX_PUSH_LOCK_WAKING |
                                  EX_PUSH_LOCK_WAITING |
                                  (ULONG_PTR)WaitBlock;

                /* Check if the pushlock was already waking */
                if (!OldValue.Waking) NeedWake = TRUE;
            }
            else
            {
                /* We are the first waiter, so loop the wait block */
                WaitBlock->Last = WaitBlock;

                /* Point to our wait block */
                NewValue.Value = (OldValue.Value & EX_PUSH_LOCK_PTR_BITS) |
                                  EX_PUSH_LOCK_WAITING |
                                  (ULONG_PTR)WaitBlock;
            }

            /* Sanity check */
            ASSERT(NewValue.Waiting);

#if DBG
            /* Setup the Debug Wait Block */
            WaitBlock->Signaled = 0;
            WaitBlock->OldValue = OldValue;
            WaitBlock->NewValue = NewValue;
            WaitBlock->PushLock = PushLock;
#endif

            /* Write the new value */
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Ptr != OldValue.Ptr)
            {
                /* Retry */
                OldValue = *PushLock;
                continue;
            }

            /* Update the value now */
            OldValue = NewValue;

            /* Check if the pushlock needed waking */
            if (NeedWake)
            {
                /* Scan the Waiters and Wake PushLocks */
                ExpOptimizePushLockList(PushLock, OldValue);
            }

            /* Set up the Wait Gate */
            KeInitializeGate(&WaitBlock->WakeGate);

#ifdef CONFIG_SMP
            /* Now spin on the push lock if necessary */
            if (ExPushLockSpinCount)
            {
                ULONG i = ExPushLockSpinCount;

                do
                {
                    if (!(*(volatile LONG *)&WaitBlock->Flags & EX_PUSH_LOCK_WAITING))
                        break;

                    YieldProcessor();
                } while (--i);
            }
#endif

            /* Now try to remove the wait bit */
            if (InterlockedBitTestAndReset(&WaitBlock->Flags, 1))
            {
                /* Fast-path did not work, we need to do a full wait */
                KeWaitForGate(&WaitBlock->WakeGate, WrPushLock, KernelMode);
                ASSERT(WaitBlock->Signaled);
            }

            /* We shouldn't be shared anymore */
            ASSERT((WaitBlock->ShareCount == 0));
        }
    }
}

/*++
 * @name ExfReleasePushLock
 * @implemented NT5.1
 *
 *     The ExReleasePushLock routine releases a previously acquired PushLock.
 *
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks Callers of ExfReleasePushLock must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FASTCALL
ExfReleasePushLock(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock, NewValue, WakeValue;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock, LastWaitBlock;

    /* The caller must hold the push lock */
    ASSERT(OldValue.Locked);

    while (TRUE)
    {
        /*
         * Without waiters, the upper bits contain the shared acquisition count.
         * Drop one shared acquisition, or clear the lock word when releasing the
         * final owner.
         *
         * A waiter may be inserted before the CMPXCHG completes. If that happens,
         * continue using the value returned by the failed operation.
         */
        if (!OldValue.Waiting)
        {
            if (OldValue.Shared > 1)
            {
                NewValue = OldValue;
                NewValue.Shared--;
            }
            else
            {
                NewValue.Value = 0;
            }

            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value == OldValue.Value) return;

            OldValue = NewValue;
        }
        else
        {
            /*
             * Once waiters are queued, the upper bits hold a wait block pointer.
             * If multiple shared owners existed when the first waiter was queued,
             * their remaining count is stored in the oldest exclusive wait block,
             * and MultipleShared marks that representation.
             *
             * If this call releases one of those shared owners, only the final
             * shared release continues to unlock the push lock.
             */
            if (OldValue.MultipleShared)
            {
                WaitBlock = (PEX_PUSH_LOCK_WAIT_BLOCK)(OldValue.Value &
                                                       ~EX_PUSH_LOCK_PTR_BITS);

                /*
                 * The push lock points to the newest wait block. Last contains
                 * the oldest block once the list has been optimized; otherwise
                 * follow Next until a wait block with Last set is found.
                 */
                while (TRUE)
                {
                    LastWaitBlock = WaitBlock->Last;
                    if (LastWaitBlock)
                    {
                        WaitBlock = LastWaitBlock;
                        break;
                    }

                    WaitBlock = WaitBlock->Next;
                }

                /*
                 * A generic release may reach this path after the saved shared
                 * acquisition count has already been exhausted.
                 */
                if (WaitBlock->ShareCount > 0)
                {
                    /* The outstanding shared count is stored in the oldest exclusive waiter */
                    ASSERT(WaitBlock->Flags & EX_PUSH_LOCK_FLAGS_EXCLUSIVE);

                    if (InterlockedDecrement(&WaitBlock->ShareCount) > 0) return;
                }
            }

            /*
             * The final owner must clear Locked and MultipleShared. If no wakeup is
             * in progress, it must also set Waking and process the wait list. A new
             * waiter may be inserted while this update is being attempted, so both
             * cases use CMPXCHG.
             */
            for (;;)
            {
                if (OldValue.Waking)
                {
                    /*
                     * Another thread owns wakeup. Leave Waking set and
                     * clear only the ownership state.
                     */
                    NewValue.Value = OldValue.Value;
                    NewValue.MultipleShared = FALSE;
                    NewValue.Locked = FALSE;

                    ASSERT(NewValue.Waking && !NewValue.Locked && !NewValue.MultipleShared);

                    NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                                     NewValue.Ptr,
                                                                     OldValue.Ptr);
                    if (NewValue.Value == OldValue.Value) return;
                }
                else
                {
                    /*
                     * No thread is responsible for wakeup yet. Clear the ownership state
                     * and set Waking in the same atomic update so this thread becomes
                     * responsible for processing the wait list.
                     */
                    NewValue.Value = OldValue.Value;
                    NewValue.MultipleShared = FALSE;
                    NewValue.Locked = FALSE;

                    NewValue.Waking = TRUE;

                    ASSERT(NewValue.Waking && !NewValue.Locked && !NewValue.MultipleShared);

                    WakeValue = NewValue;
                    NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                                     NewValue.Ptr,
                                                                     OldValue.Ptr);
                    if (NewValue.Value == OldValue.Value)
                    {
                        /* Wake the waiters after the state update succeeds */
                        ExfWakePushLock(PushLock, WakeValue);
                        return;
                    }
                }

                /*
                 * The failed CMPXCHG returned the current lock value, use it for
                 * the next attempt.
                 */
                OldValue = NewValue;
            }
        }
    }
}

/*++
 * @name ExfReleasePushLockShared
 * @implemented NT5.2
 *
 *     The ExfReleasePushLockShared macro releases a previously acquired PushLock.
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks Callers of ExReleasePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FASTCALL
ExfReleasePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock, NewValue, WakeValue;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock, LastWaitBlock;

    /* The caller must hold the push lock */
    ASSERT(OldValue.Locked);

    /*
     * Without waiters, the upper bits contain the shared acquisition count.
     * Drop one shared acquisition, or clear the lock word when releasing the
     * final owner.
     *
     * A waiter may be inserted before the CMPXCHG completes. If that happens,
     * continue using the value returned by the failed operation.
     */
    while (!OldValue.Waiting)
    {
        if (OldValue.Shared > 1)
        {
            NewValue = OldValue;
            NewValue.Shared--;
        }
        else
        {
            NewValue.Value = 0;
        }

        NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                         NewValue.Ptr,
                                                         OldValue.Ptr);
        if (NewValue.Value == OldValue.Value) return;

        OldValue = NewValue;
    }

    /*
     * Once waiters are queued, the upper bits hold a wait block pointer.
     * If multiple shared owners existed when the first waiter was queued,
     * their remaining count is stored in the oldest exclusive wait block,
     * and MultipleShared marks that representation.
     *
     * Only the final shared release continues to unlock the push lock.
     */
    if (OldValue.MultipleShared)
    {
        WaitBlock = (PEX_PUSH_LOCK_WAIT_BLOCK)(OldValue.Value &
                                               ~EX_PUSH_LOCK_PTR_BITS);

        /*
         * The push lock points to the newest wait block. Last contains the
         * oldest block once the list has been optimized; otherwise follow
         * Next until a wait block with Last set is found.
         */
        while (TRUE)
        {
            LastWaitBlock = WaitBlock->Last;
            if (LastWaitBlock)
            {
                WaitBlock = LastWaitBlock;
                break;
            }

            WaitBlock = WaitBlock->Next;
        }

        /* The oldest exclusive waiter stores the remaining shared acquisitions */
        ASSERT(WaitBlock->ShareCount > 0);
        ASSERT(WaitBlock->Flags & EX_PUSH_LOCK_FLAGS_EXCLUSIVE);

        if (InterlockedDecrement(&WaitBlock->ShareCount) > 0) return;
    }

    /*
     * The final owner must clear Locked and MultipleShared. If no wakeup is
     * in progress, it must also set Waking and process the wait list. A new
     * waiter may be inserted while this update is being attempted, so both
     * cases use CMPXCHG.
     */
    for (;;)
    {
        if (OldValue.Waking)
        {
            /*
             * Another thread owns wakeup. Leave Waking set and
             * clear only the ownership state.
             */
            NewValue.Value = OldValue.Value;
            NewValue.MultipleShared = FALSE;
            NewValue.Locked = FALSE;

            ASSERT(NewValue.Waking && !NewValue.Locked && !NewValue.MultipleShared);

            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value == OldValue.Value) return;
        }
        else
        {
            /*
             * No thread is responsible for wakeup yet. Clear the ownership state
             * and set Waking in the same atomic update so this thread becomes 
             * responsible for processing the wait list.
             */
            NewValue.Value = OldValue.Value;
            NewValue.MultipleShared = FALSE;
            NewValue.Locked = FALSE;

            NewValue.Waking = TRUE;

            ASSERT(NewValue.Waking && !NewValue.Locked && !NewValue.MultipleShared);

            WakeValue = NewValue;
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);
            if (NewValue.Value == OldValue.Value)
            {
                /* Wake the waiters after the state update succeeds */
                ExfWakePushLock(PushLock, WakeValue);
                return;
            }
        }

        /*
         * The failed CMPXCHG returned the current lock value, use it for
         * the next attempt.
         */
        OldValue = NewValue;
    }
}

/*++
 * ExfReleasePushLockExclusive
 * @implemented NT5.2
 *
 *     The ExfReleasePushLockExclusive routine releases a previously
 *     exclusively acquired PushLock.
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks Callers of ExReleasePushLockExclusive must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FASTCALL
ExfReleasePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK NewValue, WakeValue;
    EX_PUSH_LOCK OldValue = *PushLock;

    /* Loop until we can change */
    for (;;)
    {
        /* Sanity checks */
        ASSERT(OldValue.Locked);
        ASSERT(OldValue.Waiting || OldValue.Shared == 0);

        /* Check if it's waiting and not yet waking */
        if ((OldValue.Waiting) && !(OldValue.Waking))
        {
            /* Remove the lock bit, and add the wake bit */
            NewValue.Value = (OldValue.Value &~ EX_PUSH_LOCK_LOCK) |
                              EX_PUSH_LOCK_WAKING;

            /* Sanity check */
            ASSERT(NewValue.Waking && !NewValue.Locked);

            /* Write the New Value. Save our original value for waking */
            WakeValue = NewValue;
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);

            /* Check if the value changed behind our back */
            if (NewValue.Value == OldValue.Value)
            {
                /* Wake the Pushlock */
                ExfWakePushLock(PushLock, WakeValue);
                break;
            }
        }
        else
        {
            /* A simple unlock */
            NewValue.Value = OldValue.Value &~ EX_PUSH_LOCK_LOCK;

            /* Sanity check */
            ASSERT(NewValue.Waking || !NewValue.Waiting);

            /* Write the New Value */
            NewValue.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr,
                                                             NewValue.Ptr,
                                                             OldValue.Ptr);

            /* Check if the value changed behind our back */
            if (NewValue.Value == OldValue.Value) break;
        }

        /* Loop again */
        OldValue = NewValue;
    }
}

/*++
 * @name ExfTryToWakePushLock
 * @implemented NT5.2
 *
 *     The ExfTryToWakePushLock attempts to wake a waiting pushlock.
 *
 * @param PushLock
 *        Pointer to a PushLock which is in the wait state.
 *
 * @return None.
 *
 * @remarks The pushlock must be in a wait state and must not be already waking.
 *
 *--*/
VOID
FASTCALL
ExfTryToWakePushLock(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock, NewValue;

    /*
     * If the Pushlock is not waiting on anything, or if it's already waking up
     * and locked, don't do anything
     */
    if ((OldValue.Waking) || (OldValue.Locked) || !(OldValue.Waiting)) return;

    /* Make it Waking */
    NewValue = OldValue;
    NewValue.Waking = TRUE;

    /* Write the New Value */
    if (InterlockedCompareExchangePointer(&PushLock->Ptr,
                                          NewValue.Ptr,
                                          OldValue.Ptr) == OldValue.Ptr)
    {
        /* Wake the Pushlock */
        ExfWakePushLock(PushLock, NewValue);
    }
}

/*++
 * @name ExfUnblockPushLock
 * @implemented NT5.1
 *
 *     The ExfUnblockPushLock routine unblocks a previously blocked PushLock.
 *
 * @param PushLock
 *        Pointer to a previously blocked PushLock.
 *
 * @return None.
 *
 * @remarks Callers of ExfUnblockPushLock can be running at any IRQL.
 *
 *--*/
VOID
FASTCALL
ExfUnblockPushLock(PEX_PUSH_LOCK PushLock,
                   PVOID CurrentWaitBlock)
{
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock, NextWaitBlock;
    KIRQL OldIrql = DISPATCH_LEVEL;

    /* Get the wait block and erase the previous one */
    WaitBlock = InterlockedExchangePointer(&PushLock->Ptr, NULL);
    if (WaitBlock)
    {
        /* Check if there is a linked pushlock and raise IRQL appropriately */
        if (WaitBlock->Next) KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        /* Start block loop */
        while (WaitBlock)
        {
            /* Get the next block */
            NextWaitBlock = WaitBlock->Next;

            /* Remove the wait flag from the Wait block */
            if (!InterlockedBitTestAndReset(&WaitBlock->Flags, EX_PUSH_LOCK_FLAGS_WAIT_V))
            {
                /* Nobody removed the flag before us, so signal the event */
                KeSetEventBoostPriority(&WaitBlock->WakeEvent, NULL);
            }

            /* Try the next one */
            WaitBlock = NextWaitBlock;
        }

        /* Lower IRQL if needed */
        if (OldIrql != DISPATCH_LEVEL) KeLowerIrql(OldIrql);
    }

    /* Check if we got a wait block that's pending */
    if ((CurrentWaitBlock) &&
        (((PEX_PUSH_LOCK_WAIT_BLOCK)CurrentWaitBlock)->Flags &
           EX_PUSH_LOCK_FLAGS_WAIT))
    {
        /* Wait for the pushlock to be unblocked */
        ExWaitForUnblockPushLock(PushLock, CurrentWaitBlock);
    }
}
