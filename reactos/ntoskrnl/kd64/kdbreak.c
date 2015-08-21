/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdbreak.c
 * PURPOSE:         KD64 Breakpoint Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
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
    KD_BREAKPOINT_TYPE Content;
    ULONG i;
    NTSTATUS Status;

    /* Loop current breakpoints */
    for (i = 0; i < KD_BREAKPOINT_MAX; i++)
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
    for (i = 0; i < KD_BREAKPOINT_MAX; i++) if (!(KdpBreakpointTable[i].Flags)) break;

    /* Fail if no free entry was found */
    if (i == KD_BREAKPOINT_MAX) return 0;

    /* Save the old instruction */
    Status = KdpCopyMemoryChunks((ULONG_PTR)Address,
                                 &Content,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE,
                                 NULL);

    if (!NT_SUCCESS(Status))
    {
        /* TODO: Set it as a owed breakpoint */
        KdpDprintf("Failed to set breakpoint at address 0x%p\n", Address);
        return 0;
    }

    /* Write the entry */
    KdpBreakpointTable[i].Address = Address;
    KdpBreakpointTable[i].Content = Content;
    KdpBreakpointTable[i].Flags = KdpBreakpointActive;

    /* Write the breakpoint */
    Status = KdpCopyMemoryChunks((ULONG_PTR)Address,
                                 &KdpBreakpointInstruction,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        /* This should never happen */
        KdpDprintf("Unable to write breakpoint to address 0x%p\n", Address);
    }

    /* Return the breakpoint handle */
    return i + 1;
}

BOOLEAN
NTAPI
KdpLowWriteContent(IN ULONG BpIndex)
{
    NTSTATUS Status;

    /* Make sure that the breakpoint is actually active */
    if (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointPending)
    {
        /* So we have a valid breakpoint, but it hasn't been used yet... */
        KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointPending;
        return TRUE;
    }

    /* Is the original instruction a breakpoint anyway? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Then leave it that way... */
        return TRUE;
    }

    /* We have an active breakpoint with an instruction to bring back. Do it. */
    Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[BpIndex].
                                 Address,
                                 &KdpBreakpointTable[BpIndex].Content,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        /* TODO: Set it as a owed breakpoint */
        KdpDprintf("Failed to delete breakpoint at address 0x%p\n",
                   KdpBreakpointTable[BpIndex].Address);
        return FALSE;
    }

    /* Everything went fine, return */
    return TRUE;
}

BOOLEAN
NTAPI
KdpLowRestoreBreakpoint(IN ULONG BpIndex)
{
    NTSTATUS Status;

    /* Were we not able to remove it earlier? */
    if (KdpBreakpointTable[BpIndex].Flags & KdpBreakpointExpired)
    {
        /* Well then, we'll just re-use it and return success! */
        KdpBreakpointTable[BpIndex].Flags &= ~KdpBreakpointExpired;
        return TRUE;
    }

    /* Are we merely writing a breakpoint on top of another breakpoint? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Nothing to do then... */
        return TRUE;
    }

    /* Ok, we actually have to overwrite the instruction now */
    Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[BpIndex].
                                 Address,
                                 &KdpBreakpointInstruction,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        /* FIXME: Set it as a owed breakpoint */
        KdpDprintf("Failed to restore breakpoint at address 0x%p\n",
                   KdpBreakpointTable[BpIndex].Address);
        return FALSE;
    }

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
    if (!(BpEntry) || (BpEntry > KD_BREAKPOINT_MAX)) return FALSE;

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
    BOOLEAN DeletedBreakpoints;

    /* Assume no breakpoints will be deleted */
    DeletedBreakpoints = FALSE;

    /* Loop the breakpoint table */
    for (BpIndex = 0; BpIndex < KD_BREAKPOINT_MAX; BpIndex++)
    {
        /* Make sure that the breakpoint is active and matches the range. */
        if ((KdpBreakpointTable[BpIndex].Flags & KdpBreakpointActive) &&
            ((KdpBreakpointTable[BpIndex].Address >= Base) &&
             (KdpBreakpointTable[BpIndex].Address <= Limit)))
        {
            /* Delete it, and remember if we succeeded at least once */
            if (KdpDeleteBreakpoint(BpIndex + 1)) DeletedBreakpoints = TRUE;
        }
    }

    /* Return whether we deleted anything */
    return DeletedBreakpoints;
}

VOID
NTAPI
KdpRestoreAllBreakpoints(VOID)
{
    ULONG BpIndex;

    /* No more suspended Breakpoints */
    BreakpointsSuspended = FALSE;

    /* Loop the breakpoints */
    for (BpIndex = 0; BpIndex < KD_BREAKPOINT_MAX; BpIndex++)
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

VOID
NTAPI
KdpSuspendBreakPoint(IN ULONG BpEntry)
{
    ULONG BpIndex = BpEntry - 1;

    /* Check if this is a valid, unsuspended breakpoint */
    if ((KdpBreakpointTable[BpIndex].Flags & KdpBreakpointActive) &&
        !(KdpBreakpointTable[BpIndex].Flags & KdpBreakpointSuspended))
    {
        /* Suspend it */
        KdpBreakpointTable[BpIndex].Flags |= KdpBreakpointSuspended;
        KdpLowWriteContent(BpIndex);
    }
}

VOID
NTAPI
KdpSuspendAllBreakPoints(VOID)
{
    ULONG BpEntry;

    /* Breakpoints are suspended */
    BreakpointsSuspended = TRUE;

    /* Loop every breakpoint */
    for (BpEntry = 1; BpEntry <= KD_BREAKPOINT_MAX; BpEntry++)
    {
        /* Suspend it */
        KdpSuspendBreakPoint(BpEntry);
    }
}
