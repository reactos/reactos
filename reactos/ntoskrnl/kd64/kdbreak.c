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

    /* Check whether we are not setting a breakpoint twice */
    for (i = 0; i < KD_BREAKPOINT_MAX; i++)
    {
        /* Check if the breakpoint is valid */
        if ((KdpBreakpointTable[i].Flags & KD_BREAKPOINT_ACTIVE) &&
            (KdpBreakpointTable[i].Address == Address))
        {
            /* Were we not able to remove it earlier? */
            if (KdpBreakpointTable[i].Flags & KD_BREAKPOINT_EXPIRED)
            {
                /* Just re-use it! */
                KdpBreakpointTable[i].Flags &= ~KD_BREAKPOINT_EXPIRED;
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
    for (i = 0; i < KD_BREAKPOINT_MAX; i++)
    {
        if (KdpBreakpointTable[i].Flags == 0)
            break;
    }

    /* Fail if no free entry was found */
    if (i == KD_BREAKPOINT_MAX) return 0;

    /* Save the breakpoint */
    KdpBreakpointTable[i].Address = Address;

    /* If we are setting the breakpoint in user space, save the active process context */
    if (Address < KD_HIGHEST_USER_BREAKPOINT_ADDRESS)
        KdpBreakpointTable[i].DirectoryTableBase = KeGetCurrentThread()->ApcState.Process->DirectoryTableBase[0];

    /* Try to save the old instruction */
    Status = KdpCopyMemoryChunks((ULONG_PTR)Address,
                                 &Content,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE,
                                 NULL);
    if (NT_SUCCESS(Status))
    {
        /* Memory accessible, set the breakpoint */
        KdpBreakpointTable[i].Content = Content;
        KdpBreakpointTable[i].Flags   = KD_BREAKPOINT_ACTIVE;

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
    }
    else
    {
        /* Memory is inaccessible now, setting breakpoint is deferred */
        KdpDprintf("Failed to set breakpoint at address 0x%p, adding deferred breakpoint.\n", Address);
        KdpBreakpointTable[i].Flags = KD_BREAKPOINT_ACTIVE | KD_BREAKPOINT_PENDING;
        KdpOweBreakpoint = TRUE;
    }

    /* Return the breakpoint handle */
    return i + 1;
}

VOID
NTAPI
KdSetOwedBreakpoints(VOID)
{
    BOOLEAN Enable;
    KD_BREAKPOINT_TYPE Content;
    ULONG i;
    NTSTATUS Status;

    /* If we don't owe any breakpoints, just return */
    if (!KdpOweBreakpoint) return;

    /* Enter the debugger */
    Enable = KdEnterDebugger(NULL, NULL);

    /*
     * Suppose we succeed in setting all the breakpoints.
     * If we fail to do so, the flag will be set again.
     */
    KdpOweBreakpoint = FALSE;

    /* Loop through current breakpoints and try to set or delete the pending ones */
    for (i = 0; i < KD_BREAKPOINT_MAX; i++)
    {
        if (KdpBreakpointTable[i].Flags & (KD_BREAKPOINT_PENDING | KD_BREAKPOINT_EXPIRED))
        {
            /*
             * Set the breakpoint only if it is in kernel space, or if it is
             * in user space and the active process context matches.
             */
            if (KdpBreakpointTable[i].Address < KD_HIGHEST_USER_BREAKPOINT_ADDRESS &&
                KdpBreakpointTable[i].DirectoryTableBase != KeGetCurrentThread()->ApcState.Process->DirectoryTableBase[0])
            {
                KdpOweBreakpoint = TRUE;
                continue;
            }

            /* Try to save the old instruction */
            Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[i].Address,
                                         &Content,
                                         KD_BREAKPOINT_SIZE,
                                         0,
                                         MMDBG_COPY_UNSAFE,
                                         NULL);
            if (!NT_SUCCESS(Status))
            {
                /* Memory is still inaccessible, breakpoint setting will be deferred again */
                // KdpDprintf("Failed to set deferred breakpoint at address 0x%p\n",
                //            KdpBreakpointTable[i].Address);
                KdpOweBreakpoint = TRUE;
                continue;
            }

            /* Check if we need to write the breakpoint */
            if (KdpBreakpointTable[i].Flags & KD_BREAKPOINT_PENDING)
            {
                /* Memory accessible, set the breakpoint */
                KdpBreakpointTable[i].Content = Content;

                /* Write the breakpoint */
                Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[i].Address,
                                             &KdpBreakpointInstruction,
                                             KD_BREAKPOINT_SIZE,
                                             0,
                                             MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                             NULL);
                if (!NT_SUCCESS(Status))
                {
                    /* This should never happen */
                    KdpDprintf("Unable to write deferred breakpoint to address 0x%p\n",
                               KdpBreakpointTable[i].Address);
                    KdpOweBreakpoint = TRUE;
                }
                else
                {
                    KdpBreakpointTable[i].Flags = KD_BREAKPOINT_ACTIVE;
                }

                continue;
            }

            /* Check if we need to restore the original instruction */
            if (KdpBreakpointTable[i].Flags & KD_BREAKPOINT_EXPIRED)
            {
                /* Write it back */
                Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[i].Address,
                                             &KdpBreakpointTable[i].Content,
                                             KD_BREAKPOINT_SIZE,
                                             0,
                                             MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                             NULL);
                if (!NT_SUCCESS(Status))
                {
                    /* This should never happen */
                    KdpDprintf("Failed to delete deferred breakpoint at address 0x%p\n",
                               KdpBreakpointTable[i].Address);
                    KdpOweBreakpoint = TRUE;
                }
                else
                {
                    /* Check if the breakpoint is suspended */
                    if (KdpBreakpointTable[i].Flags & KD_BREAKPOINT_SUSPENDED)
                    {
                        KdpBreakpointTable[i].Flags = KD_BREAKPOINT_SUSPENDED | KD_BREAKPOINT_ACTIVE;
                    }
                    else
                    {
                        /* Invalidate it */
                        KdpBreakpointTable[i].Flags = 0;
                    }
                }

                continue;
            }
        }
    }

    /* Exit the debugger */
    KdExitDebugger(Enable);
}

BOOLEAN
NTAPI
KdpLowWriteContent(IN ULONG BpIndex)
{
    NTSTATUS Status;

    /* Make sure that the breakpoint is actually active */
    if (KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_PENDING)
    {
        /* So we have a valid breakpoint, but it hasn't been used yet... */
        KdpBreakpointTable[BpIndex].Flags &= ~KD_BREAKPOINT_PENDING;
        return TRUE;
    }

    /* Is the original instruction a breakpoint anyway? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Then leave it that way... */
        return TRUE;
    }

    /* We have an active breakpoint with an instruction to bring back. Do it. */
    Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[BpIndex].Address,
                                 &KdpBreakpointTable[BpIndex].Content,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Memory is inaccessible now, restoring original instruction is deferred */
        KdpDprintf("Failed to delete breakpoint at address 0x%p\n",
                   KdpBreakpointTable[BpIndex].Address);
        KdpBreakpointTable[BpIndex].Flags |= KD_BREAKPOINT_EXPIRED;
        KdpOweBreakpoint = TRUE;
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
    if (KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_EXPIRED)
    {
        /* Just re-use it! */
        KdpBreakpointTable[BpIndex].Flags &= ~KD_BREAKPOINT_EXPIRED;
        return TRUE;
    }

    /* Are we merely writing a breakpoint on top of another breakpoint? */
    if (KdpBreakpointTable[BpIndex].Content == KdpBreakpointInstruction)
    {
        /* Nothing to do */
        return TRUE;
    }

    /* Ok, we actually have to overwrite the instruction now */
    Status = KdpCopyMemoryChunks((ULONG_PTR)KdpBreakpointTable[BpIndex].Address,
                                 &KdpBreakpointInstruction,
                                 KD_BREAKPOINT_SIZE,
                                 0,
                                 MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        KdpDprintf("Failed to restore breakpoint at address 0x%p\n",
                   KdpBreakpointTable[BpIndex].Address);
        KdpBreakpointTable[BpIndex].Flags |= KD_BREAKPOINT_PENDING;
        KdpOweBreakpoint = TRUE;
        return FALSE;
    }

    /* Clear any possible previous pending flag and return success */
    KdpBreakpointTable[BpIndex].Flags &= ~KD_BREAKPOINT_PENDING;
    return TRUE;
}

BOOLEAN
NTAPI
KdpDeleteBreakpoint(IN ULONG BpEntry)
{
    ULONG BpIndex = BpEntry - 1;

    /* Check for invalid breakpoint entry */
    if (!BpEntry || (BpEntry > KD_BREAKPOINT_MAX)) return FALSE;

    /* If the specified breakpoint table entry is not valid, then return FALSE. */
    if (!KdpBreakpointTable[BpIndex].Flags) return FALSE;

    /* Check if the breakpoint is suspended */
    if (KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_SUSPENDED)
    {
        /* Check if breakpoint is not being deleted */
        if (!(KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_EXPIRED))
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
        if ((KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_ACTIVE) &&
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
        if ((KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_ACTIVE) &&
            (KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_SUSPENDED))
        {
            /* Unsuspend them */
            KdpBreakpointTable[BpIndex].Flags &= ~KD_BREAKPOINT_SUSPENDED;
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
    if ((KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_ACTIVE) &&
        !(KdpBreakpointTable[BpIndex].Flags & KD_BREAKPOINT_SUSPENDED))
    {
        /* Suspend it */
        KdpBreakpointTable[BpIndex].Flags |= KD_BREAKPOINT_SUSPENDED;
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
