/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Report the PCI Express configuration windows to the resource arbiters
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

C_ASSERT(FIELD_OFFSET(MCFG_TABLE, Allocation) == 44);
C_ASSERT(sizeof(MCFG_ALLOCATION) == 16);

/* Buses of one segment decoded through one configuration window */
typedef struct _HALP_PCI_SEGMENT_WINDOW
{
    ULONGLONG BaseAddress;
    USHORT Segment;
    ULONG Buses[256 / 32];
} HALP_PCI_SEGMENT_WINDOW, *PHALP_PCI_SEGMENT_WINDOW;

/* FUNCTIONS ****************************************************************/

static
CODE_SEG("INIT")
BOOLEAN
NTAPI
HalpIsSegmentBusSet(
    _In_ PHALP_PCI_SEGMENT_WINDOW Window,
    _In_ ULONG Bus)
{
    return (Window->Buses[Bus / 32] & (1UL << (Bus % 32))) != 0;
}

/* Merges the MCFG entries into one bus map per segment */
static
CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpCollectSegmentWindows(
    _In_ PMCFG_TABLE Mcfg,
    _In_ ULONG EntryCount,
    _Out_writes_(EntryCount) PHALP_PCI_SEGMENT_WINDOW Windows,
    _Out_ PULONG WindowCount)
{
    PMCFG_ALLOCATION Entry;
    PHALP_PCI_SEGMENT_WINDOW Window;
    ULONG Index, Slot, Bus;

    *WindowCount = 0;

    for (Index = 0; Index < EntryCount; Index++)
    {
        Entry = &Mcfg->Allocation[Index];
        if (Entry->StartBusNumber > Entry->EndBusNumber)
            return STATUS_INVALID_PARAMETER;

        Window = NULL;
        for (Slot = 0; Slot < *WindowCount; Slot++)
        {
            if (Windows[Slot].Segment == Entry->PciSegmentGroup)
            {
                Window = &Windows[Slot];
                break;
            }
        }

        if (Window == NULL)
        {
            Window = &Windows[(*WindowCount)++];
            RtlZeroMemory(Window, sizeof(*Window));
            Window->Segment = Entry->PciSegmentGroup;
            Window->BaseAddress = Entry->BaseAddress;
        }
        else if (Window->BaseAddress != Entry->BaseAddress)
        {
            return STATUS_INVALID_PARAMETER;
        }

        for (Bus = Entry->StartBusNumber; Bus <= Entry->EndBusNumber; Bus++)
        {
            if (HalpIsSegmentBusSet(Window, Bus))
                return STATUS_INVALID_PARAMETER;

            Window->Buses[Bus / 32] |= 1UL << (Bus % 32);
        }
    }

    return STATUS_SUCCESS;
}

/* Walks the contiguous bus runs, filling descriptors when a list is given */
static
CODE_SEG("INIT")
ULONG
NTAPI
HalpDescribeSegmentWindows(
    _In_reads_(WindowCount) PHALP_PCI_SEGMENT_WINDOW Windows,
    _In_ ULONG WindowCount,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR Descriptors)
{
    ULONG Index, Bus, FirstBus, Count = 0;

    for (Index = 0; Index < WindowCount; Index++)
    {
        Bus = 0;
        while (Bus < 256)
        {
            if (!HalpIsSegmentBusSet(&Windows[Index], Bus))
            {
                Bus++;
                continue;
            }

            FirstBus = Bus;
            while ((Bus < 256) && HalpIsSegmentBusSet(&Windows[Index], Bus))
                Bus++;

            if (Descriptors)
            {
                Descriptors[Count].Type = CmResourceTypeMemory;
                Descriptors[Count].u.Memory.MinimumAddress.QuadPart =
                    Windows[Index].BaseAddress + ((ULONGLONG)FirstBus << 20);
                Descriptors[Count].u.Memory.MaximumAddress.QuadPart =
                    Windows[Index].BaseAddress + ((ULONGLONG)Bus << 20) - 1;

                DPRINT("HAL: PCI segment %u buses %lu-%lu use 0x%I64x\n",
                       Windows[Index].Segment, FirstBus, Bus - 1,
                       Descriptors[Count].u.Memory.MinimumAddress.QuadPart);
            }

            Count++;
        }
    }

    return Count;
}

/* A window published on an earlier boot must not outlive the MCFG that described it */
static
CODE_SEG("INIT")
VOID
NTAPI
HalpRemoveMmConfigRanges(
    _In_ PUNICODE_STRING KeyName,
    _In_ PUNICODE_STRING ValueName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    if (NT_SUCCESS(ZwOpenKey(&KeyHandle, KEY_SET_VALUE, &ObjectAttributes)))
    {
        ZwDeleteValueKey(KeyHandle, ValueName);
        ZwClose(KeyHandle);
    }
}

/**
 * @brief
 * Stores the MCFG configuration windows as Arbiters\ReservedResources\MmConfigRange,
 * one memory descriptor per contiguous bus run of a segment.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpPublishMmConfigRanges(VOID)
{
    UNICODE_STRING KeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet"
                            L"\\Control\\Arbiters\\ReservedResources");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"MmConfigRange");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PIO_RESOURCE_REQUIREMENTS_LIST List;
    PHALP_PCI_SEGMENT_WINDOW Windows;
    PMCFG_TABLE Mcfg;
    HANDLE KeyHandle;
    ULONG EntryCount, WindowCount, DescriptorCount, Size;
    NTSTATUS Status;

    Mcfg = HalAcpiGetTable(NULL, MCFG_SIGNATURE);
    if ((Mcfg == NULL) ||
        (Mcfg->Header.Length < FIELD_OFFSET(MCFG_TABLE, Allocation) + sizeof(MCFG_ALLOCATION)))
    {
        HalpRemoveMmConfigRanges(&KeyName, &ValueName);
        return STATUS_NOT_FOUND;
    }

    EntryCount = (Mcfg->Header.Length - FIELD_OFFSET(MCFG_TABLE, Allocation)) /
                 sizeof(MCFG_ALLOCATION);

    Windows = ExAllocatePoolWithTag(PagedPool,
                                    EntryCount * sizeof(HALP_PCI_SEGMENT_WINDOW),
                                    TAG_HAL);
    if (Windows == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = HalpCollectSegmentWindows(Mcfg, EntryCount, Windows, &WindowCount);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HAL: Conflicting MCFG entries, no configuration windows reported\n");
        ExFreePoolWithTag(Windows, TAG_HAL);
        HalpRemoveMmConfigRanges(&KeyName, &ValueName);
        return Status;
    }

    DescriptorCount = HalpDescribeSegmentWindows(Windows, WindowCount, NULL);
    Size = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors) +
           DescriptorCount * sizeof(IO_RESOURCE_DESCRIPTOR);

    List = ExAllocatePoolWithTag(PagedPool, Size, TAG_HAL);
    if (List == NULL)
    {
        ExFreePoolWithTag(Windows, TAG_HAL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(List, Size);
    List->ListSize = Size;
    List->AlternativeLists = 1;
    List->List[0].Version = 1;
    List->List[0].Revision = 1;
    List->List[0].Count = HalpDescribeSegmentWindows(Windows,
                                                     WindowCount,
                                                     List->List[0].Descriptors);

    ExFreePoolWithTag(Windows, TAG_HAL);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_RESOURCE_REQUIREMENTS_LIST,
                               List,
                               Size);
        ZwClose(KeyHandle);
    }

    if (!NT_SUCCESS(Status))
        DPRINT1("HAL: Failed to store MmConfigRange, status 0x%lx\n", Status);

    ExFreePoolWithTag(List, TAG_HAL);
    return Status;
}

/* EOF */
