/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdbreak.c
 * PURPOSE:         KD64 Breakpoint Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

ULONG
NTAPI
KdpAddBreakpoint(IN PVOID Address)
{
    UCHAR Content;
    ULONG i;

    /* Loop current breakpoints */
    for (i = 0; i < 20; i++)
    {
        /* Check if the breakpoint is valid */
        if ((KdpBreakpointTable[i].Flags & KdpBreakpointActive) &&
            (KdpBreakpointTable[i].Address == Address))
        {
            /* Check if it's pending */
            if ((KdpBreakpointTable[i].Flags & KdpBreakpointPending))
            {
                /* It's not pending anymore now */
                KdpBreakpointTable[i].Flags &= ~KdpBreakpointPending;
                return i + 1;
            }
            else
            {
                /* Fail */
                return 0;
            }
        }
    }

    /* Find a free entry */
    for (i = 0; i < 20; i++) if (!(KdpBreakpointTable[i].Flags)) break;

    /* Fail if no free entry was found */
    if (i == 20) return 0;

    /* Save the old instruction */
    RtlCopyMemory(&Content, Address, sizeof(UCHAR));

    /* Write the entry */
    KdpBreakpointTable[i].Address = Address;
    KdpBreakpointTable[i].Content = Content;
    KdpBreakpointTable[i].Flags = KdpBreakpointActive;

    /* Write the INT3 and return the handle */
    RtlCopyMemory(Address, &KdpBreakpointInstruction, sizeof(UCHAR));
    return i + 1;
}

BOOLEAN
NTAPI
KdpLowWriteContent(IN ULONG BpIndex)
{
    /* Make sure that the breakpoint is actually active */
    if (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointPending)
    {
        /* So we have a valid breakpoint, but it hasn't been used yet... */
        KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointPending;
        return TRUE;
    }

    /* Is the original instruction an INT3 anyway? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Then leave it that way... */
        return TRUE;
    }

    /* We have an active breakpoint with an instruction to bring back. Do it. */
    RtlCopyMemory(KdpBreakpointTable[BpIndex].Address,
                  &KdpBreakpointTable[BpIndex].Content,
                  sizeof(UCHAR));

    /* Everything went fine, return */
    return TRUE;
}

BOOLEAN
NTAPI
KdpLowRestoreBreakpoint(IN ULONG BpIndex)
{
    /* Were we not able to remove it earlier? */
    if (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointExpired)
    {
        /* Well then, we'll just re-use it and return success! */
        KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointExpired;
        return TRUE;
    }

    /* Are we merely writing an INT3 on top of another INT3? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Nothing to do then... */
        return TRUE;
    }

    /* Ok, we actually have to overwrite the instruction now */
    RtlCopyMemory(KdpBreakpointTable[BpIndex].Address,
                  &KdpBreakpointInstruction,
                  sizeof(UCHAR));

    /* Clear any possible previous pending flag and return success */
    KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointPending;
    return TRUE;
}

BOOLEAN
NTAPI
KdpDeleteBreakpoint(IN ULONG BpEntry)
{
    ULONG BpIndex = BpEntry - 1;

    /* Check for invalid breakpoint entry */
    if (!(BpEntry) || (BpEntry > 20)) return FALSE;

    /* If the specified breakpoint table entry is not valid, then return FALSE. */
    if (!KdpBreakpointTable[BpIndex].Flags) return FALSE;

    /* Check if the breakpoint is suspended */
    if (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointSuspended)
    {
        /* Check if breakpoint is not ...? */
        if (!(KdpBreakpointTable[BpIndex].Flags & KdpBreakpointExpired))
        {
            /* Invalidate it and return success */
            KdpBreakpointTable[BpIndex].Flags = 0;
            return TRUE;
        }
    }

    /* Restore original data, then invalidate it and return success */
    if (KdpLowWriteContent(BpIndex)) KdpBreakpointTable[BpIndex].Flags = 0;
    return TRUE;
}

BOOLEAN
NTAPI
KdpDeleteBreakpointRange(IN PVOID Base,
                         IN PVOID Limit)
{
    ULONG BpIndex;
    BOOLEAN Return = FALSE;

    /* Loop the breakpoint table */
    for (BpIndex = 0; BpIndex < 20; BpIndex++)
    {
        /* Make sure that the breakpoint is active and matches the range. */
        if ((KdpBreakpointTable[BpIndex].Flags & KdpBreakpointActive) &&
            ((KdpBreakpointTable[BpIndex].Address >= Base) &&
             (KdpBreakpointTable[BpIndex].Address <= Limit)))
        {
            /* Delete it */
            Return = Return || KdpDeleteBreakpoint(BpIndex + 1);
        }
    }

    /* Return to caller */
    return Return;
}

VOID
NTAPI
KdpRestoreAllBreakpoints(VOID)
{
    ULONG BpIndex;

    /* No more suspended Breakpoints */
    BreakpointsSuspended = FALSE;

    /* Loop the breakpoints */
    for (BpIndex = 0; BpIndex < 20; BpIndex++ )
    {
        /* Check if they are valid, suspended breakpoints */
        if ((KdpBreakpointTable[BpIndex].Flags & KdpBreakpointActive) &&
            (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointSuspended))
        {
            /* Unsuspend them */
            KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointSuspended;
            KdpLowRestoreBreakpoint(BpIndex);
        }
    }
}
