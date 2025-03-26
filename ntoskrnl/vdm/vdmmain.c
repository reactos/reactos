/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/vdm/vdmmain.c
 * PURPOSE:         VDM Support Services
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
VOID
NTAPI
Ki386VdmEnablePentiumExtentions(IN BOOLEAN Enable)
{
    ULONG EFlags, Cr4;

    /* Save interrupt state and disable them */
    EFlags = __readeflags();
    _disable();

    /* Enable or disable VME as required */
    Cr4 = __readcr4();
    __writecr4(Enable ? Cr4 | CR4_VME : Cr4 & ~CR4_VME);

    /* Restore interrupt state */
    __writeeflags(EFlags);
}

CODE_SEG("INIT")
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
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
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
            Ki386VdmEnablePentiumExtentions(TRUE);
            KeI386VirtualIntExtensions = TRUE;
        }
    }

    /* Close the key */
    ZwClose(RegHandle);
}

NTSTATUS
NTAPI
VdmpInitialize(PVOID ControlData)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PhysMemName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
    NTSTATUS Status;
    HANDLE PhysMemHandle;
    PVOID BaseAddress;
    volatile PVOID NullAddress = NULL;
    LARGE_INTEGER Offset;
    ULONG ViewSize;

    /* Open the physical memory section */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PhysMemName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenSection(&PhysMemHandle,
                           SECTION_ALL_ACCESS,
                           &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't open \\Device\\PhysicalMemory\n");
        return Status;
    }

    /* Map the BIOS and device registers into the address space */
    Offset.QuadPart = 0;
    ViewSize = PAGE_SIZE;
    BaseAddress = 0;
    Status = ZwMapViewOfSection(PhysMemHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't map physical memory (%x)\n", Status);
        ZwClose(PhysMemHandle);
        return Status;
    }

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Copy the first physical page into the first virtual page */
        RtlMoveMemory(NullAddress, BaseAddress, ViewSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail */
        DPRINT1("Couldn't copy first page (%x)\n", Status);
        ZwClose(PhysMemHandle);
        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Close physical memory section handle */
    ZwClose(PhysMemHandle);

    /* Unmap the section */
    Status = ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't unmap the section (%x)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
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

            /* Call the init sub-function */
            Status = VdmpInitialize(ControlData);
            break;

        default:

            /* Unsupported */
            DPRINT1("Unknown VDM call: %lx\n", ControlCode);
            Status = STATUS_INVALID_PARAMETER;
    }

    /* Return the status */
    return Status;
}
