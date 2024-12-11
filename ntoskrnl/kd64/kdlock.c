/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdlock.c
 * PURPOSE:         KD64 Port Lock and Break-in Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KdpPollBreakInWithPortLock(VOID)
{
    BOOLEAN DoBreak = FALSE;

    /* If KD is not enabled, no break to do */
    if (!KdDebuggerEnabled)
        return FALSE;

    /* Check if a CTRL-C is in the queue */
    if (KdpContext.KdpControlCPending)
    {
        /* Set it and prepare for break */
        DoBreak = TRUE;
        KdpContext.KdpControlCPending = FALSE;
    }
    else
    {
        /* Now get a packet */
        if (KdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN,
                            NULL,
                            NULL,
                            NULL,
                            NULL) == KdPacketReceived)
        {
            /* Successful break-in */
            DoBreak = TRUE;
        }
    }

    /* Tell the caller to do a break */
    return DoBreak;
}

/* PUBLIC FUNCTIONS **********************************************************/

static BOOLEAN KdpDebuggerLockIntsEnabled;

/**
 * @brief
 * Acquires the kernel debugger global lock at DISPATCH_LEVEL.
 *
 * @warning
 * ReactOS-only: Also disables the interrupts.
 *
 * @param[out]  OldIrql
 * Pointer to a KIRQL variable that is set to the current IRQL when
 * the call occurs, that is to be restored when the lock is released.
 *
 * @note    Exported in NT 6.3.
 **/
_Requires_lock_not_held_(KdpDebuggerLock)
_Acquires_lock_(KdpDebuggerLock)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_raises_(DISPATCH_LEVEL)
VOID
NTAPI
KdAcquireDebuggerLock(
    _Out_ _IRQL_saves_ PKIRQL OldIrql)
{
#ifdef __REACTOS__
    /* Disable interrupts */
    BOOLEAN Enable = KeDisableInterrupts();
#endif
    /* Raise to dispatch and acquire the lock */
    KeRaiseIrql(DISPATCH_LEVEL, OldIrql);
    KiAcquireSpinLock(&KdpDebuggerLock);
#ifdef __REACTOS__
    /* Save the interrupts state now */
    KdpDebuggerLockIntsEnabled = Enable;
#endif
}

/**
 * @brief
 * Releases the kernel debugger global lock.
 *
 * @warning
 * ReactOS-only: Also re-enables the interrupts.
 *
 * @param[in]   OldIrql
 * Specifies the KIRQL value saved from the preceding call to KdAcquireDebuggerLock.
 *
 * @note    Exported in NT 6.3.
 **/
_Requires_lock_held_(KdpDebuggerLock)
_Releases_lock_(KdpDebuggerLock)
_IRQL_requires_(DISPATCH_LEVEL)
VOID
NTAPI
KdReleaseDebuggerLock(
    _In_ _IRQL_restores_ KIRQL OldIrql)
{
#ifdef __REACTOS__
    /* Get the interrupts state */
    BOOLEAN WereEnabled = KdpDebuggerLockIntsEnabled;
#endif
    /* Release the lock and lower IRQL back */
    KiReleaseSpinLock(&KdpDebuggerLock);
    KeLowerIrql(OldIrql);
#ifdef __REACTOS__
    /* Re-enable interrupts */
    KeRestoreInterrupts(WereEnabled);
#endif
}

extern void KdDbgPortPrintf(PCSTR Format, ...);

/*
 * @implemented
 *
 * @note
 * Since this function is called in KeUpdateSystemTime() to regularly poll
 * the debugger for any incoming break-in, it can disable/re-enable interrupts,
 * but **MUST NOT** change the current IRQL!
 */
BOOLEAN
NTAPI
KdPollBreakIn(VOID)
{
    BOOLEAN DoBreak = FALSE;
    BOOLEAN Enable;
    KIRQL OldIrql;

    /* If KD is not enabled, no break to do */
    if (KdPitchDebugger || !KdDebuggerEnabled)
        return FALSE;

    /* Disable interrupts */
    Enable = KeDisableInterrupts();

    /* Elevate IRQL to HIGH_LEVEL, to ensure the lock can be correctly
     * acquired if needed -- the two cases where KdPollBreakIn() is NOT
     * already invoked at >= DISPATCH_LEVEL is during system startup,
     * when KiSystemStartup() invokes KdInitSystem(0, KeLoaderBlock);
     * and when it also invokes KdPollBreakIn() itself. */
    // See commit 835c3023
    // ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check if a CTRL-C is in the queue */
    if (KdpContext.KdpControlCPending)
    {
        /* Set it and prepare for break */
        KdpControlCPressed = TRUE;
        DoBreak = TRUE;
        KdpContext.KdpControlCPending = FALSE;
    }
    else
    {
        // See above...

        /* Try to acquire the lock */
        if (((KSPIN_LOCK)KeGetCurrentThread() | 1) == KdpDebuggerLock)
        //if (!KeTestSpinLock(&KdpDebuggerLock))
        {
            KdDbgPortPrintf("%s(%p) already owned\n", __FUNCTION__, &KdpDebuggerLock);
            KeRosDumpStackFrames(NULL, 0);
        }
        else
        if (KeTryToAcquireSpinLockAtDpcLevel(&KdpDebuggerLock))
        {
            /* Now get a packet */
            if (KdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN,
                                NULL,
                                NULL,
                                NULL,
                                NULL) == KdPacketReceived)
            {
                /* Successful break-in */
                DoBreak = TRUE;
                KdpControlCPressed = TRUE;
            }

            /* Let go of the port */
            KeReleaseSpinLockFromDpcLevel(&KdpDebuggerLock);
        }
    }

    KeLowerIrql(OldIrql);

    /* Re-enable interrupts */
    KeRestoreInterrupts(Enable);

    /* Tell the caller to do a break */
    return DoBreak;
}
