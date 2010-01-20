/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/mmdbg.c
 * PURPOSE:         Memory Manager support routines for the Kernel Debugger
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmDbgCopyMemory(IN ULONG64 Address,
                IN PVOID Buffer,
                IN ULONG Size,
                IN ULONG Flags)
{
    NTSTATUS Status;

    /* For now, this must be a "unsafe" copy */
    ASSERT(Flags & MMDBG_COPY_UNSAFE);

    /* We only handle 1, 2, 4 and 8 byte requests */
    if ((Size != 1) &&
        (Size != 2) &&
        (Size != 4) &&
        (Size != MMDBG_COPY_MAX_SIZE))
    {
        /* Invalid size, fail */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* The copy must be aligned too */
    if ((Address & (Size - 1)) != 0)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* No physical memory support yet */
    if (Flags & MMDBG_COPY_PHYSICAL)
    {
        /* Fail */
        KdpDprintf("MmDbgCopyMemory: Failing %s for Physical Address 0x%I64x\n",
                   Flags & MMDBG_COPY_WRITE ? "write" : "read",
                   Address);
        return STATUS_UNSUCCESSFUL;
    }

    /* Simple check for invalid address */
    if ((MiAddressToPde((ULONG_PTR)Address)->u.Hard.Valid == 0) ||
        (MiAddressToPte((ULONG_PTR)Address)->u.Hard.Valid == 0))
    {
        /* Fail */
        KdpDprintf("MmDbgCopyMemory: Failing %s for invalid Address 0x%p\n",
                   Flags & MMDBG_COPY_WRITE ? "write" : "read",
                   (PVOID)(ULONG_PTR)Address);
        return STATUS_UNSUCCESSFUL;
    }

    /* If we are going to write to it then make sure it is writeable too */
    if ((Flags & MMDBG_COPY_WRITE) &&
        (!MI_IS_PAGE_WRITEABLE(MiAddressToPte((ULONG_PTR)Address))))
    {
        /* Fail */
        KdpDprintf("MmDbgCopyMemory: Failing write for Address 0x%p\n",
                   (PVOID)(ULONG_PTR)Address);
        return STATUS_UNSUCCESSFUL;
    }

    /* Use SEH to try to catch anything else somewhat cleanly */
    _SEH2_TRY
    {
        /* Check if this is read or write */
        if (Flags & MMDBG_COPY_WRITE)
        {
            /* Do the write */
            RtlCopyMemory((PVOID)(ULONG_PTR)Address,
                          Buffer,
                          Size);
        }
        else
        {
            /* Do the read */
            RtlCopyMemory(Buffer,
                          (PVOID)(ULONG_PTR)Address,
                          Size);
        }

        /* Copy succeeded */
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}
