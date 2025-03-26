/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/i386/psldt.c
 * PURPOSE:         LDT support for x86
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
PspDeleteLdt(PEPROCESS Process)
{
    /* FIXME - LdtInformation must be null as long as we don't implement VDMs */
    ASSERT(Process->LdtInformation == NULL);
}

VOID
NTAPI
PspDeleteVdmObjects(PEPROCESS Process)
{
    /* If there are no VDM objects, just exit */
    if (Process->VdmObjects == NULL)
        return;

    /* FIXME: Need to do more than just freeing the main VdmObjects member! */
    UNIMPLEMENTED;

    /* Free VDM objects */
    ExFreePoolWithTag(Process->VdmObjects, TAG_KERNEL);
    Process->VdmObjects = NULL;
}

NTSTATUS
NTAPI
PspQueryDescriptorThread(IN PETHREAD Thread,
                         IN PVOID ThreadInformation,
                         IN ULONG ThreadInformationLength,
                         OUT PULONG ReturnLength OPTIONAL)
{
    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    LDT_ENTRY Descriptor;
    NTSTATUS Status;
    PAGED_CODE();

    /* Verify the size */
    if (ThreadInformationLength != sizeof(DESCRIPTOR_TABLE_ENTRY))
    {
        /* Fail */
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Enter SEH for the copy */
    _SEH2_TRY
    {
        /* Get the descriptor */
        RtlCopyMemory(&DescriptorEntry,
                      ThreadInformation,
                      sizeof(DESCRIPTOR_TABLE_ENTRY));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if this is a GDT selector */
    if (!(DescriptorEntry.Selector & 0x4))
    {
        /* Get the GDT entry */
        Status = Ke386GetGdtEntryThread(&Thread->Tcb,
                                        DescriptorEntry.Selector & 0xFFFFFFF8,
                                        (PKGDTENTRY)&Descriptor);
        if (!NT_SUCCESS(Status)) return Status;

        /* Enter SEH for the copy */
        _SEH2_TRY
        {
            /* Copy the GDT entry to caller */
            RtlCopyMemory(&((PDESCRIPTOR_TABLE_ENTRY)ThreadInformation)->
                          Descriptor,
                          &Descriptor,
                          sizeof(LDT_ENTRY));
            if (ReturnLength) *ReturnLength = sizeof(LDT_ENTRY);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* Success */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* This is only supported for VDM, which we don't implement */
        ASSERT(Thread->ThreadsProcess->LdtInformation == NULL);
        Status = STATUS_NO_LDT;
    }

    /* Return status to caller */
    return Status;
}
