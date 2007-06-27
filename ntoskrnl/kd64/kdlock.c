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
            if (!KdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL))
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

/*
 * @implemented
 */
BOOLEAN
NTAPI
KdPollBreakIn(VOID)
{
    BOOLEAN DoBreak = FALSE;
    ULONG Flags = 0;

    /* First make sure that KD is enabled */
    if (KdDebuggerEnabled)
    {
        /* Disable interrupts */
        Ke386SaveFlags(Flags);
        //Flags = __getcallerseflags();
        _disable();

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
            /* Try to acquire the lock */
            if (KeTryToAcquireSpinLockAtDpcLevel(&KdpDebuggerLock))
            {
                /* Now get a packet */
                if (!KdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL))
                {
                    /* Successful breakin */
                    DoBreak = TRUE;
                    KdpControlCPressed = TRUE;
                }

                /* Let go of the port */
                KdpPortUnlock();
            }
        }

        /* Re-enable interrupts if they were disabled */
        if (Flags & EFLAGS_INTERRUPT_MASK) _enable();
    }

    /* Tell the caller to do a break */
    return DoBreak;
}

