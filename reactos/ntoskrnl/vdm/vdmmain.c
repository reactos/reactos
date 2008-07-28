/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/vdm/vdmmain.c
 * PURPOSE:         VDM Support Services
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static UCHAR OrigIVT[1024];
static UCHAR OrigBDA[256];

/* PRIVATE FUNCTIONS *********************************************************/

VOID
INIT_FUNCTION
NtEarlyInitVdm(VOID)
{
    PCHAR start = MmCreateHyperspaceMapping(0);
    memcpy(OrigIVT, start, 1024);
    memcpy(OrigBDA, start+0x400, 256);
    MmDeleteHyperspaceMapping(start);
}

VOID
NTAPI
Ki386VdmEnablePentiumExtentions(VOID)
{
    DPRINT1("VME detected but not yet supported\n");
}

VOID
NTAPI
KeI386VdmInitialize(VOID)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RegHandle;
    UNICODE_STRING Name;
    UCHAR KeyValueInfo[sizeof(KEY_VALUE_BASIC_INFORMATION) + 30];
    ULONG ReturnLength;

    /* Make sure that there is a WOW key */
    RtlInitUnicodeString(&Name,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\Wow");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&RegHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return;

    /* Check if VME is enabled */
    RtlInitUnicodeString(&Name, L"DisableVme");
    Status = ZwQueryValueKey(RegHandle,
                             &Name,
                             KeyValueBasicInformation,
                             &KeyValueInfo,
                             sizeof(KeyValueInfo),
                             &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Not present, so check if the CPU supports VME */
        if (KeGetPcr()->Prcb->FeatureBits & KF_V86_VIS)
        {
            /* Enable them. FIXME: Use IPI */
            Ki386VdmEnablePentiumExtentions();
            KeI386VirtualIntExtensions = TRUE;
        }
    }

    /* Close the key */
    ZwClose(RegHandle);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtVdmControl(IN ULONG ControlCode,
             IN PVOID ControlData)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Check which control code this is */
    switch (ControlCode)
    {
        /* VDM Execution start */
        case VdmStartExecution:

            /* Call the sub-function */
            Status = VdmpStartExecution();
            break;

        case VdmInitialize:

            /* Pretty much a hack, since a lot more needs to happen */
            memcpy(ControlData, OrigIVT, 1024);
            memcpy((PVOID)((ULONG_PTR)ControlData + 1024), OrigBDA, 256);
            Status = STATUS_SUCCESS;
            break;

        default:

            /* Unsupported */
            DPRINT1("Unknown VDM call: %lx\n", ControlCode);
            Status = STATUS_INVALID_PARAMETER;
    }

    /* Return the status */
    return Status;
}
