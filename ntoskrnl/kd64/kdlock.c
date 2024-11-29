/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdlock.c
 * PURPOSE:         KD64 Port Lock and Breakin Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KdpPortLock(VOID)
{
    /* Acquire the lock */
    KiAcquireSpinLock(&KdpDebuggerLock);
}

VOID
NTAPI
KdpPortUnlock(VOID)
{
    /* Release the lock */
    KiReleaseSpinLock(&KdpDebuggerLock);
}

BOOLEAN
NTAPI
KdpPollBreakInWithPortLock(VOID)
{
    BOOLEAN DoBreak = FALSE;

    /* First make sure that KD is enabled */
    if (KdDebuggerEnabled)
    {
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
                /* Successful breakin */
                DoBreak = TRUE;
            }
        }
    }

    /* Tell the caller to do a break */
    return DoBreak;
}

/* PUBLIC FUNCTIONS **********************************************************/

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

    /* First make sure that KD is enabled */
    if (KdDebuggerEnabled)
    {
        /* Disable interrupts */
        BOOLEAN Enable = KeDisableInterrupts();
        KIRQL OldIrql;

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
                    /* Successful breakin */
                    DoBreak = TRUE;
                    KdpControlCPressed = TRUE;
                }

                /* Let go of the port */
                KdpPortUnlock();
            }
        }

        KeLowerIrql(OldIrql);

        /* Re-enable interrupts */
        KeRestoreInterrupts(Enable);
    }

    /* Tell the caller to do a break */
    return DoBreak;
}
