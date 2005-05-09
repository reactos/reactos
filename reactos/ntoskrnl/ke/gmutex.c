/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/gmutex.c
 * PURPOSE:         Implements Guarded Mutex
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) and
 *                  Filip Navara (xnavara@volny.cz)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

UCHAR
FASTCALL
InterlockedClearBit(PLONG Destination,
                    LONG Bit);

typedef enum _KGUARDED_MUTEX_BITS
{
    GM_LOCK_BIT = 1,
    GM_LOCK_WAITER_WOKEN = 2,
    GM_LOCK_WAITER_INC = 4
} KGUARDED_MUTEX_BITS;

/* FUNCTIONS *****************************************************************/

/**
 * @name KeEnterGuardedRegion
 *
 * Enters a guarded region. This causes all (incl. special kernel) APCs
 * to be disabled.
 */
VOID
STDCALL
KeEnterGuardedRegion(VOID)
{
    /* Disable Special APCs */
    KeGetCurrentThread()->SpecialApcDisable--;
}

/**
 * @name KeLeaveGuardedRegion
 *
 * Leaves a guarded region and delivers pending APCs if possible.
 */
VOID
STDCALL
KeLeaveGuardedRegion(VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Boost the enable count and check if Special APCs are enabled */
    if (++Thread->SpecialApcDisable == 0)
    {
        /* Check if there are Kernel APCs on the list */
        if (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* Check for APC Delivery */
            KiKernelApcDeliveryCheck();
        }
    }
}

VOID
FASTCALL
KeInitializeGuardedMutex(PKGUARDED_MUTEX GuardedMutex)
{
    /* Setup the Initial Data */
    GuardedMutex->Count = GM_LOCK_BIT;
    GuardedMutex->Owner = NULL;
    GuardedMutex->Contention = 0;

    /* Initialize the Wait Gate */
    KeInitializeGate(&GuardedMutex->Gate);
}

VOID
FASTCALL
KiAcquireGuardedMutexContented(PKGUARDED_MUTEX GuardedMutex)
{
    ULONG BitsToRemove;
    ULONG BitsToAdd;
    LONG OldValue;

    /* Increase the contention count */
    InterlockedIncrement((PLONG)&GuardedMutex->Contention);

    /* Start by unlocking the Guarded Mutex */
    BitsToRemove = GM_LOCK_BIT;
    BitsToAdd = GM_LOCK_WAITER_INC;

    while (1)
    {
        /* Get the Count Bits */
        OldValue = (volatile LONG)GuardedMutex->Count;

        /* Check if the Guarded Mutex is locked */
        if (OldValue & GM_LOCK_BIT)
        {
            /* Unlock it by removing the Lock Bit */
            if (InterlockedCompareExchange(&GuardedMutex->Count,
                                           OldValue &~ BitsToRemove,
                                           OldValue) == OldValue)
            {
                /* The Guarded Mutex is now unlocked */
                break;
            }
        }
        else
        {
            /* The Guarded Mutex isn't locked, so simply set the bits */
            if (InterlockedCompareExchange(&GuardedMutex->Count,
                                           OldValue | BitsToAdd,
                                           OldValue) != OldValue)
            {
                /* The Guarded Mutex value changed behind our back, start over */
                continue;
            }

            /* Now we have to wait for it */
            KeWaitForGate(&GuardedMutex->Gate, WrGuardedMutex, KernelMode);

            /* Ok, the wait is done, so set the new bits */
            BitsToRemove = GM_LOCK_BIT | GM_LOCK_WAITER_WOKEN;
            BitsToAdd = GM_LOCK_WAITER_WOKEN;
       }
    }
}

VOID
FASTCALL
KeAcquireGuardedMutex(PKGUARDED_MUTEX GuardedMutex)
{
    /* Disable Special APCs */
    KeEnterGuardedRegion();

    /* Do the Unsafe Acquire */
    KeAcquireGuardedMutexUnsafe(GuardedMutex);
}

VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(PKGUARDED_MUTEX GuardedMutex)
{
    /* Remove the lock */
    if (!InterlockedClearBit(&GuardedMutex->Count, 0))
    {
        /* The Guarded Mutex was already locked, enter contented case */
        KiAcquireGuardedMutexContented(GuardedMutex);
    }

    /* Set the Owner */
    GuardedMutex->Owner = KeGetCurrentThread();
}

VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(PKGUARDED_MUTEX GuardedMutex)
{
    LONG OldValue;

    /* Destroy the Owner */
    GuardedMutex->Owner = NULL;

    /* Add the Lock Bit */
    OldValue = InterlockedExchangeAdd(&GuardedMutex->Count, 1);

    /* Check if it was already locked, but not woken */
    if (OldValue && !(OldValue & GM_LOCK_WAITER_WOKEN))
    {
        /* Update the Oldvalue to what it should be now */
        OldValue |= GM_LOCK_BIT;

        /* Remove the Woken bit */
        if (InterlockedCompareExchange(&GuardedMutex->Count,
                                       OldValue &~ GM_LOCK_WAITER_WOKEN,
                                       OldValue) == OldValue)
        {
            /* Signal the Gate */
            KeSignalGateBoostPriority(&GuardedMutex->Gate);
        }
    }
}

VOID
FASTCALL
KeReleaseGuardedMutex(PKGUARDED_MUTEX GuardedMutex)
{
    /* Do the actual release */
    KeReleaseGuardedMutexUnsafe(GuardedMutex);

    /* Re-enable APCs */
    KeLeaveGuardedRegion();
}

BOOL
FASTCALL
KeTryToAcquireGuardedMutex(PKGUARDED_MUTEX GuardedMutex)
{
    /* Block APCs */
    KeEnterGuardedRegion();

    /* Remove the lock */
    if (InterlockedClearBit(&GuardedMutex->Count, 0))
    {
        /* Re-enable APCs */
        KeLeaveGuardedRegion();

        /* Return failure */
        return FALSE;
    }

    /* Set the Owner */
    GuardedMutex->Owner = KeGetCurrentThread();
    return TRUE;
}

/* EOF */
