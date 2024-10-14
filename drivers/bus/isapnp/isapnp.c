/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Driver entry
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *                  Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "isapnp.h"

#include <search.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KEVENT BusSyncEvent;

_Guarded_by_(BusSyncEvent)
BOOLEAN ReadPortCreated = FALSE;

_Guarded_by_(BusSyncEvent)
LIST_ENTRY BusListHead;

static PUCHAR Priority;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
int
__cdecl
IsaComparePriority(
    const void *A,
    const void *B)
{
    PAGED_CODE();

    return Priority[*(PUCHAR)A] - Priority[*(PUCHAR)B];
}

static
CODE_SEG("PAGE")
VOID
IsaDetermineBestConfig(
    _Out_writes_all_(ISAPNP_MAX_ALTERNATIVES) PUCHAR BestConfig,
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    UCHAR i;

    PAGED_CODE();

    for (i = 0; i < ISAPNP_MAX_ALTERNATIVES; i++)
    {
        BestConfig[i] = i;
    }

    Priority = Alternatives->Priority;
    qsort(BestConfig,
          Alternatives->Count,
          sizeof(*BestConfig),
          IsaComparePriority);
}

static
CODE_SEG("PAGE")
VOID
IsaConvertIoRequirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PISAPNP_IO_DESCRIPTION Description)
{
    PAGED_CODE();

    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO;
    if (Description->Information & 0x1)
        Descriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
    else
        Descriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
    Descriptor->u.Port.Length = Description->Length;
    Descriptor->u.Port.Alignment = Description->Alignment;
    Descriptor->u.Port.MinimumAddress.LowPart = Description->Minimum;
    Descriptor->u.Port.MaximumAddress.LowPart = Description->Maximum +
                                                Description->Length - 1;
}

static
CODE_SEG("PAGE")
VOID
IsaConvertIrqRequirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PISAPNP_IRQ_DESCRIPTION Description,
    _In_ ULONG Vector,
    _In_ BOOLEAN FirstDescriptor)
{
    PAGED_CODE();

    if (!FirstDescriptor)
        Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
    Descriptor->Type = CmResourceTypeInterrupt;
    if (Description->Information & 0xC)
    {
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        Descriptor->ShareDisposition = CmResourceShareShared;
    }
    else
    {
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    }
    Descriptor->u.Interrupt.MinimumVector =
    Descriptor->u.Interrupt.MaximumVector = Vector;
}

static
CODE_SEG("PAGE")
VOID
IsaConvertDmaRequirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PISAPNP_DMA_DESCRIPTION Description,
    _In_ ULONG Channel,
    _In_ BOOLEAN FirstDescriptor)
{
    UNREFERENCED_PARAMETER(Description);

    PAGED_CODE();

    if (!FirstDescriptor)
        Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
    Descriptor->Type = CmResourceTypeDma;
    Descriptor->ShareDisposition = CmResourceShareUndetermined;
    Descriptor->Flags = CM_RESOURCE_DMA_8; /* Ignore information byte for compatibility */
    Descriptor->u.Dma.MinimumChannel =
    Descriptor->u.Dma.MaximumChannel = Channel;
}

static
CODE_SEG("PAGE")
VOID
IsaConvertMemRangeRequirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PISAPNP_MEMRANGE_DESCRIPTION Description)
{
    PAGED_CODE();

    Descriptor->Type = CmResourceTypeMemory;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_MEMORY_24;
    if ((Description->Information & 0x40) || !(Description->Information & 0x01))
        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
    else
        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
    Descriptor->u.Memory.Length = Description->Length << 8;
    if (Description->Alignment == 0)
        Descriptor->u.Memory.Alignment = 0x10000;
    else
        Descriptor->u.Memory.Alignment = Description->Alignment;
    Descriptor->u.Memory.MinimumAddress.LowPart = Description->Minimum << 8;
    Descriptor->u.Memory.MaximumAddress.LowPart = (Description->Maximum << 8) +
                                                  (Description->Length << 8) - 1;
}

static
CODE_SEG("PAGE")
VOID
IsaConvertMemRange32Requirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PISAPNP_MEMRANGE32_DESCRIPTION Description)
{
    PAGED_CODE();

    Descriptor->Type = CmResourceTypeMemory;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_MEMORY_24;
    if ((Description->Information & 0x40) || !(Description->Information & 0x01))
        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
    else
        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
    Descriptor->u.Memory.Length = Description->Length;
    Descriptor->u.Memory.Alignment = Description->Alignment;
    Descriptor->u.Memory.MinimumAddress.LowPart = Description->Minimum;
    Descriptor->u.Memory.MaximumAddress.LowPart = Description->Maximum +
                                                  Description->Length - 1;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateLogicalDeviceRequirements(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    RTL_BITMAP TempBitmap;
    ULONG TempBuffer;
    ULONG ResourceCount = 0, AltCount = 0, AltOptionalCount = 0;
    ULONG ListSize, i, j;
    BOOLEAN FirstDescriptor;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PISAPNP_ALTERNATIVES Alternatives = LogDev->Alternatives;

    PAGED_CODE();

    /* Count number of requirements */
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        /*
         * Use the continue statement to count the number of requirements.
         * We handle a possible gap because depedent function can appear at
         * any position in the logical device's requirements list.
         */
        if (!LogDev->Io[i].Description.Length)
            continue;

        ResourceCount++;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            continue;

        TempBuffer = LogDev->Irq[i].Description.Mask;
        RtlInitializeBitMap(&TempBitmap,
                            &TempBuffer,
                            RTL_BITS_OF(LogDev->Irq[i].Description.Mask));
        ResourceCount += RtlNumberOfSetBits(&TempBitmap);
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            continue;

        TempBuffer = LogDev->Dma[i].Description.Mask;
        RtlInitializeBitMap(&TempBitmap,
                            &TempBuffer,
                            RTL_BITS_OF(LogDev->Dma[i].Description.Mask));
        ResourceCount += RtlNumberOfSetBits(&TempBitmap);
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange); i++)
    {
        if (!LogDev->MemRange[i].Description.Length)
            continue;

        ResourceCount++;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange32); i++)
    {
        if (!LogDev->MemRange32[i].Description.Length)
            continue;

        ResourceCount++;
    }
    if (Alternatives)
    {
        ULONG BitCount;

        if (HasIoAlternatives(Alternatives))
            AltCount++;
        if (HasIrqAlternatives(Alternatives))
            AltCount++;
        if (HasDmaAlternatives(Alternatives))
            AltCount++;
        if (HasMemoryAlternatives(Alternatives))
            AltCount++;
        if (HasMemory32Alternatives(Alternatives))
            AltCount++;
        ResourceCount += AltCount;

        if (HasIrqAlternatives(Alternatives))
        {
            for (i = 0; i < Alternatives->Count; i++)
            {
                TempBuffer = Alternatives->Irq[i].Mask;
                RtlInitializeBitMap(&TempBitmap,
                                    &TempBuffer,
                                    RTL_BITS_OF(Alternatives->Irq[i].Mask));
                BitCount = RtlNumberOfSetBits(&TempBitmap);

                if (BitCount > 1)
                    AltOptionalCount += BitCount - 1;
            }
        }
        if (HasDmaAlternatives(Alternatives))
        {
            for (i = 0; i < Alternatives->Count; i++)
            {
                TempBuffer = Alternatives->Dma[i].Mask;
                RtlInitializeBitMap(&TempBitmap,
                                    &TempBuffer,
                                    RTL_BITS_OF(Alternatives->Dma[i].Mask));
                BitCount = RtlNumberOfSetBits(&TempBitmap);

                if (BitCount > 1)
                    AltOptionalCount += BitCount - 1;
            }
        }
    }
    if (ResourceCount == 0)
        return STATUS_SUCCESS;

    /* Allocate memory to store requirements */
    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
    if (Alternatives)
    {
        ListSize += sizeof(IO_RESOURCE_DESCRIPTOR) * (ResourceCount - 1) * Alternatives->Count
                    + sizeof(IO_RESOURCE_LIST) * (Alternatives->Count - 1)
                    + sizeof(IO_RESOURCE_DESCRIPTOR) * AltOptionalCount;
    }
    else
    {
        ListSize += sizeof(IO_RESOURCE_DESCRIPTOR) * (ResourceCount - 1);
    }
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RequirementsList->ListSize = ListSize;
    RequirementsList->InterfaceType = Isa;
    RequirementsList->AlternativeLists = Alternatives ? Alternatives->Count : 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = ResourceCount;

    /* Store requirements */
    Descriptor = RequirementsList->List[0].Descriptors;
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].Description.Length)
            break;

        IsaConvertIoRequirement(Descriptor++, &LogDev->Io[i].Description);
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            continue;

        FirstDescriptor = TRUE;

        for (j = 0; j < RTL_BITS_OF(LogDev->Irq[i].Description.Mask); j++)
        {
            if (!(LogDev->Irq[i].Description.Mask & (1 << j)))
                continue;

            IsaConvertIrqRequirement(Descriptor++,
                                     &LogDev->Irq[i].Description,
                                     j,
                                     FirstDescriptor);

            if (FirstDescriptor)
                FirstDescriptor = FALSE;
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            continue;

        FirstDescriptor = TRUE;

        for (j = 0; j < RTL_BITS_OF(LogDev->Dma[i].Description.Mask); j++)
        {
            if (!(LogDev->Dma[i].Description.Mask & (1 << j)))
                continue;

            IsaConvertDmaRequirement(Descriptor++,
                                     &LogDev->Dma[i].Description,
                                     j,
                                     FirstDescriptor);

            if (FirstDescriptor)
                FirstDescriptor = FALSE;
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange); i++)
    {
        if (!LogDev->MemRange[i].Description.Length)
            continue;

        IsaConvertMemRangeRequirement(Descriptor++,
                                      &LogDev->MemRange[i].Description);
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange32); i++)
    {
        if (!LogDev->MemRange32[i].Description.Length)
            continue;

        IsaConvertMemRange32Requirement(Descriptor++,
                                        &LogDev->MemRange32[i].Description);
    }
    if (Alternatives)
    {
        UCHAR BestConfig[ISAPNP_MAX_ALTERNATIVES];
        PIO_RESOURCE_LIST AltList = &RequirementsList->List[0];
        PIO_RESOURCE_LIST NextList = AltList;

        IsaDetermineBestConfig(BestConfig, Alternatives);

        for (i = 0; i < RequirementsList->AlternativeLists; i++)
        {
            RtlMoveMemory(NextList, AltList, sizeof(IO_RESOURCE_LIST));

            /* Just because the 'NextList->Count++' correction */
            NextList->Count = ResourceCount;
            /*
             * For example, the ROM
             * 0x15, ...        // Logical device ID
             * 0x30,            // Start DF
             * 0x22, 0x04, 0x00 // IRQ
             * 0x30,            // Start DF
             * 0x22, 0xC0, 0x00 // IRQ
             * 0x38,            // End DF
             * 0x2A, 0x20, 0x3A // DMA
             * 0x22, 0x00, 0x08 // IRQ
             * 0x79, 0x00       // END
             *
             * will be represented as the following resource requirements list:
             * Interface 1 Bus 0 Slot 0 AlternativeLists 2
             * AltList 1, AltList->Count 3
             * [Option 0, ShareDisposition 1, Flags 1] INT: Min B Max B
             * [Option 0, ShareDisposition 0, Flags 0] DMA: Min 5 Max 5
             * [Option 0, ShareDisposition 1, Flags 1] INT: Min 2 Max 2
             * End Descriptors
             * AltList 2, AltList->Count 4
             * [Option 0, ShareDisposition 1, Flags 1] INT: Min B Max B
             * [Option 0, ShareDisposition 0, Flags 0] DMA: Min 5 Max 5
             * [Option 0, ShareDisposition 1, Flags 1] INT: Min 6 Max 6
             * [Option 8, ShareDisposition 1, Flags 1] INT: Min 7 Max 7
             * End Descriptors
             */

            /* Propagate the fixed resources to our new list */
            for (j = 0; j < AltList->Count - AltCount; j++)
            {
                RtlMoveMemory(&NextList->Descriptors[j],
                              &AltList->Descriptors[j],
                              sizeof(IO_RESOURCE_DESCRIPTOR));
            }

            Descriptor = &NextList->Descriptors[NextList->Count - AltCount];

            /*
             * Append alternatives.
             * NOTE: To keep it simple, we append these to the end of the list.
             */
            if (HasIoAlternatives(Alternatives))
            {
                IsaConvertIoRequirement(Descriptor++,
                                        &Alternatives->Io[BestConfig[i]]);
            }
            if (HasIrqAlternatives(Alternatives))
            {
                FirstDescriptor = TRUE;

                for (j = 0; j < RTL_BITS_OF(Alternatives->Irq[BestConfig[i]].Mask); j++)
                {
                    if (!(Alternatives->Irq[BestConfig[i]].Mask & (1 << j)))
                        continue;

                    IsaConvertIrqRequirement(Descriptor++,
                                             &Alternatives->Irq[BestConfig[i]],
                                             j,
                                             FirstDescriptor);

                    if (FirstDescriptor)
                        FirstDescriptor = FALSE;
                    else
                        NextList->Count++;
                }
            }
            if (HasDmaAlternatives(Alternatives))
            {
                FirstDescriptor = TRUE;

                for (j = 0; j < RTL_BITS_OF(Alternatives->Dma[BestConfig[i]].Mask); j++)
                {
                    if (!(Alternatives->Dma[BestConfig[i]].Mask & (1 << j)))
                        continue;

                    IsaConvertDmaRequirement(Descriptor++,
                                             &Alternatives->Dma[BestConfig[i]],
                                             j,
                                             FirstDescriptor);

                    if (FirstDescriptor)
                        FirstDescriptor = FALSE;
                    else
                        NextList->Count++;
                }
            }
            if (HasMemoryAlternatives(Alternatives))
            {
                IsaConvertMemRangeRequirement(Descriptor++,
                                              &Alternatives->MemRange[BestConfig[i]]);
            }
            if (HasMemory32Alternatives(Alternatives))
            {
                IsaConvertMemRange32Requirement(Descriptor++,
                                                &Alternatives->MemRange32[BestConfig[i]]);
            }

            NextList = (PIO_RESOURCE_LIST)(NextList->Descriptors + NextList->Count);
        }
    }

    PdoExt->RequirementsList = RequirementsList;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
BOOLEAN
FindIoDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_opt_ ULONG Base,
    _In_ ULONG RangeStart,
    _In_ ULONG RangeEnd,
    _Out_opt_ PUCHAR Information,
    _Out_opt_ PULONG Length,
    _Out_opt_ PUCHAR WriteOrder)
{
    ULONG i;
    BOOLEAN Match;
    PISAPNP_IO_DESCRIPTION Description;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Io); i++)
    {
        Description = &LogDevice->Io[i].Description;

        Match = Base ? (Base >= Description->Minimum) && (Base <= Description->Maximum)
                     : (RangeStart >= Description->Minimum) &&
                       (RangeEnd <= (ULONG)(Description->Maximum + Description->Length - 1));

        if (Match)
        {
            if (Information)
                *Information = Description->Information;
            if (Length)
                *Length = Description->Length;
            if (WriteOrder)
                *WriteOrder = LogDevice->Io[i].Index;

            return TRUE;
        }
    }

    if (!LogDevice->Alternatives)
        return FALSE;

    for (i = 0; i < LogDevice->Alternatives->Count; i++)
    {
        Description = &LogDevice->Alternatives->Io[i];

        Match = Base ? (Base >= Description->Minimum) && (Base <= Description->Maximum)
                     : (RangeStart >= Description->Minimum) &&
                       (RangeEnd <= (ULONG)(Description->Maximum + Description->Length - 1));

        if (Match)
        {
            if (Information)
                *Information = Description->Information;
            if (Length)
                *Length = Description->Length;
            if (WriteOrder)
                *WriteOrder = LogDevice->Alternatives->IoIndex;

            return TRUE;
        }
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindIrqDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Vector,
    _Out_opt_ PUCHAR WriteOrder)
{
    ULONG i, j;
    PISAPNP_IRQ_DESCRIPTION Description;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Irq); i++)
    {
        Description = &LogDevice->Irq[i].Description;

        for (j = 0; j < RTL_BITS_OF(Description->Mask); j++)
        {
            if (Description->Mask & (1 << j))
            {
                if (j == Vector)
                {
                    if (WriteOrder)
                        *WriteOrder = LogDevice->Irq[i].Index;

                    return TRUE;
                }
            }
        }
    }

    if (!LogDevice->Alternatives)
        return FALSE;

    for (i = 0; i < LogDevice->Alternatives->Count; i++)
    {
        Description = &LogDevice->Alternatives->Irq[i];

        for (j = 0; j < RTL_BITS_OF(Description->Mask); j++)
        {
            if (Description->Mask & (1 << j))
            {
                if (j == Vector)
                {
                    if (WriteOrder)
                        *WriteOrder = LogDevice->Alternatives->IrqIndex;

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindDmaDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Channel,
    _Out_opt_ PUCHAR WriteOrder)
{
    ULONG i, j;
    PISAPNP_DMA_DESCRIPTION Description;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Dma); i++)
    {
        Description = &LogDevice->Dma[i].Description;

        for (j = 0; j < RTL_BITS_OF(Description->Mask); j++)
        {
            if (Description->Mask & (1 << j))
            {
                if (j == Channel)
                {
                    if (WriteOrder)
                        *WriteOrder = LogDevice->Dma[i].Index;

                    return TRUE;
                }
            }
        }
    }

    if (!LogDevice->Alternatives)
        return FALSE;

    for (i = 0; i < LogDevice->Alternatives->Count; i++)
    {
        Description = &LogDevice->Alternatives->Dma[i];

        for (j = 0; j < RTL_BITS_OF(Description->Mask); j++)
        {
            if (Description->Mask & (1 << j))
            {
                if (j == Channel)
                {
                    if (WriteOrder)
                        *WriteOrder = LogDevice->Alternatives->DmaIndex;

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindMemoryDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG RangeStart,
    _In_ ULONG RangeEnd,
    _Out_opt_ PBOOLEAN Memory32,
    _Out_opt_ PUCHAR Information,
    _Out_opt_ PUCHAR WriteOrder)
{
    ULONG i;
    PISAPNP_MEMRANGE_DESCRIPTION Description;
    PISAPNP_MEMRANGE32_DESCRIPTION Description32;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(LogDevice->MemRange); i++)
    {
        Description = &LogDevice->MemRange[i].Description;

        if ((RangeStart >= (ULONG)(Description->Minimum << 8)) &&
            (RangeEnd <= (ULONG)((Description->Maximum << 8) + (Description->Length << 8) - 1)))
        {
            if (Memory32)
                *Memory32 = FALSE;
            if (Information)
                *Information = Description->Information;
            if (WriteOrder)
                *WriteOrder = LogDevice->MemRange[i].Index;

            return TRUE;
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDevice->MemRange32); i++)
    {
        Description32 = &LogDevice->MemRange32[i].Description;

        if ((RangeStart >= Description32->Minimum) &&
            (RangeEnd <= (Description32->Maximum + Description32->Length - 1)))
        {
            if (Memory32)
                *Memory32 = TRUE;
            if (Information)
                *Information = Description32->Information;
            if (WriteOrder)
                *WriteOrder = LogDevice->MemRange32[i].Index;

            return TRUE;
        }
    }

    if (!LogDevice->Alternatives)
        return FALSE;

    for (i = 0; i < LogDevice->Alternatives->Count; i++)
    {
        Description = &LogDevice->Alternatives->MemRange[i];

        if ((RangeStart >= (ULONG)(Description->Minimum << 8)) &&
            (RangeEnd <= (ULONG)((Description->Maximum << 8) + (Description->Length << 8) - 1)))
        {
            if (Memory32)
                *Memory32 = FALSE;
            if (Information)
                *Information = Description->Information;
            if (WriteOrder)
                *WriteOrder = LogDevice->Alternatives->MemRangeIndex;

            return TRUE;
        }
    }
    for (i = 0; i < LogDevice->Alternatives->Count; i++)
    {
        Description32 = &LogDevice->Alternatives->MemRange32[i];

        if ((RangeStart >= Description32->Minimum) &&
            (RangeEnd <= (Description32->Maximum + Description32->Length - 1)))
        {
            if (Memory32)
                *Memory32 = TRUE;
            if (Information)
                *Information = Description32->Information;
            if (WriteOrder)
                *WriteOrder = LogDevice->Alternatives->MemRange32Index;

            return TRUE;
        }
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateLogicalDeviceResources(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    ULONG ResourceCount = 0;
    UCHAR Information;
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

    if (!(LogDev->Flags & ISAPNP_HAS_RESOURCES))
        return STATUS_SUCCESS;

    /* Count number of required resources */
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        if (LogDev->Io[i].CurrentBase)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (LogDev->Irq[i].CurrentNo)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel != 4)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange); i++)
    {
        if (LogDev->MemRange[i].CurrentBase)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange32); i++)
    {
        if (LogDev->MemRange32[i].CurrentBase)
            ResourceCount++;
        else
            break;
    }
    if (ResourceCount == 0)
        return STATUS_SUCCESS;

    /* Allocate memory to store resources */
    ListSize = sizeof(CM_RESOURCE_LIST)
               + (ResourceCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!ResourceList)
        return STATUS_NO_MEMORY;

    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = ResourceCount;

    /* Store resources */
    ResourceCount = 0;
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        ULONG CurrentLength;

        if (!LogDev->Io[i].CurrentBase)
            break;

        if (!FindIoDescriptor(LogDev,
                              LogDev->Io[i].CurrentBase,
                              0,
                              0,
                              &Information,
                              &CurrentLength,
                              NULL))
        {
            goto InvalidBiosResources;
        }

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;
        if (Information & 0x1)
            Descriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
        else
            Descriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Length = CurrentLength;
        Descriptor->u.Port.Start.LowPart = LogDev->Io[i].CurrentBase;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].CurrentNo)
            break;

        if (!FindIrqDescriptor(LogDev, LogDev->Irq[i].CurrentNo, NULL))
            goto InvalidBiosResources;

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Irq[i].CurrentType & 0x01)
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        else
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->u.Interrupt.Level = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Vector = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel == 4)
            break;

        if (!FindDmaDescriptor(LogDev, LogDev->Dma[i].CurrentChannel, NULL))
            goto InvalidBiosResources;

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeDma;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_DMA_8; /* Ignore information byte for compatibility */
        Descriptor->u.Dma.Channel = LogDev->Dma[i].CurrentChannel;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange); i++)
    {
        if (!LogDev->MemRange[i].CurrentBase)
            break;

        if (!FindMemoryDescriptor(LogDev,
                                  LogDev->MemRange[i].CurrentBase,
                                  LogDev->MemRange[i].CurrentLength,
                                  NULL,
                                  &Information,
                                  NULL))
        {
            goto InvalidBiosResources;
        }

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_24;
        if ((Information & 0x40) || !(Information & 0x01))
            Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
        else
            Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Length = LogDev->MemRange[i].Description.Length;
        Descriptor->u.Memory.Start.QuadPart = LogDev->MemRange[i].CurrentBase;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange32); i++)
    {
        if (!LogDev->MemRange32[i].CurrentBase)
            break;

        if (!FindMemoryDescriptor(LogDev,
                                  LogDev->MemRange32[i].CurrentBase,
                                  LogDev->MemRange32[i].CurrentLength,
                                  NULL,
                                  &Information,
                                  NULL))
        {
            goto InvalidBiosResources;
        }

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_24;
        if ((Information & 0x40) || !(Information & 0x01))
            Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
        else
            Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Length = LogDev->MemRange32[i].Description.Length;
        Descriptor->u.Memory.Start.QuadPart = LogDev->MemRange32[i].CurrentBase;
    }

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
    return STATUS_SUCCESS;

InvalidBiosResources:
    DPRINT("Invalid boot resources! (CSN %u, LDN %u)\n", LogDev->CSN, LogDev->LDN);

    LogDev->Flags &= ~ISAPNP_HAS_RESOURCES;
    ExFreePoolWithTag(ResourceList, TAG_ISAPNP);
    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaCreateClose;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaForwardOrIgnore;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaForwardOrIgnore(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PISAPNP_COMMON_EXTENSION CommonExt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    DPRINT("%s(%p, %p) Minor - %X\n", __FUNCTION__, DeviceObject, Irp,
           IoGetCurrentIrpStackLocation(Irp)->MinorFunction);

    if (CommonExt->Signature == IsaPnpBus)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(((PISAPNP_FDO_EXTENSION)CommonExt)->Ldo, Irp);
    }
    else
    {
        NTSTATUS Status = Irp->IoStatus.Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateReadPortDORequirements(
    _In_ PISAPNP_PDO_EXTENSION PdoExt,
    _In_opt_ ULONG SelectedReadPort)
{
    ULONG ResourceCount, ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    const ULONG Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS };
    const ULONG ReadPorts[] = { 0x274, 0x3E4, 0x204, 0x2E4, 0x354, 0x2F4 };

    PAGED_CODE();

    if (SelectedReadPort)
    {
        /*
         * [IO descriptor: ISAPNP_WRITE_DATA, required]
         * [IO descriptor: ISAPNP_WRITE_DATA, optional]
         * [IO descriptor: ISAPNP_ADDRESS, required]
         * [IO descriptor: ISAPNP_ADDRESS, optional]
         * [IO descriptor: Selected Read Port, required]
         * [IO descriptor: Read Port 1, optional]
         * [IO descriptor: Read Port 2, optional]
         * [...]
         * [IO descriptor: Read Port X - 1, optional]
         */
        ResourceCount = RTL_NUMBER_OF(Ports) * 2 + RTL_NUMBER_OF(ReadPorts);
    }
    else
    {
        /*
         * [IO descriptor: ISAPNP_WRITE_DATA, required]
         * [IO descriptor: ISAPNP_WRITE_DATA, optional]
         * [IO descriptor: ISAPNP_ADDRESS, required]
         * [IO descriptor: ISAPNP_ADDRESS, optional]
         * [IO descriptor: Read Port 1, required]
         * [IO descriptor: Read Port 1, optional]
         * [IO descriptor: Read Port 2, required]
         * [IO descriptor: Read Port 2, optional]
         * [...]
         * [IO descriptor: Read Port X, required]
         * [IO descriptor: Read Port X, optional]
         */
        ResourceCount = (RTL_NUMBER_OF(Ports) + RTL_NUMBER_OF(ReadPorts)) * 2;
    }
    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               sizeof(IO_RESOURCE_DESCRIPTOR) * (ResourceCount - 1);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RequirementsList->ListSize = ListSize;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = ResourceCount;

    Descriptor = &RequirementsList->List[0].Descriptors[0];

    /* Store the Data port and the Address port */
    for (i = 0; i < RTL_NUMBER_OF(Ports) * 2; i++)
    {
        if ((i % 2) == 0)
        {
            /* Expected port */
            Descriptor->Type = CmResourceTypePort;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
            Descriptor->u.Port.Length = 0x01;
            Descriptor->u.Port.Alignment = 0x01;
            Descriptor->u.Port.MinimumAddress.LowPart =
            Descriptor->u.Port.MaximumAddress.LowPart = Ports[i / 2];
        }
        else
        {
            /* ... but mark it as optional */
            Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            Descriptor->Type = CmResourceTypePort;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
            Descriptor->u.Port.Alignment = 0x01;
        }

        Descriptor++;
    }

    /* Store the Read Ports */
    if (SelectedReadPort)
    {
        BOOLEAN Selected = FALSE;

        DBG_UNREFERENCED_LOCAL_VARIABLE(Selected);

        for (i = 0; i < RTL_NUMBER_OF(ReadPorts); i++)
        {
            if (ReadPorts[i] != SelectedReadPort)
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            else
                Selected = TRUE;
            Descriptor->Type = CmResourceTypePort;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
            Descriptor->u.Port.Length = 0x04;
            Descriptor->u.Port.Alignment = 0x01;
            Descriptor->u.Port.MinimumAddress.LowPart = ReadPorts[i];
            Descriptor->u.Port.MaximumAddress.LowPart = ReadPorts[i] +
                                                        Descriptor->u.Port.Length - 1;

            Descriptor++;
        }

        ASSERT(Selected == TRUE);
    }
    else
    {
        for (i = 0; i < RTL_NUMBER_OF(ReadPorts) * 2; i++)
        {
            if ((i % 2) == 0)
            {
                /* Expected port */
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                Descriptor->u.Port.Length = 0x04;
                Descriptor->u.Port.Alignment = 0x01;
                Descriptor->u.Port.MinimumAddress.LowPart = ReadPorts[i / 2];
                Descriptor->u.Port.MaximumAddress.LowPart = ReadPorts[i / 2] +
                                                            Descriptor->u.Port.Length - 1;
            }
            else
            {
                /* ... but mark it as optional */
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
                Descriptor->u.Port.Alignment = 0x01;
            }

            Descriptor++;
        }
    }

    PdoExt->RequirementsList = RequirementsList;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateReadPortDOResources(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    const USHORT Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS };
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

    ListSize = sizeof(CM_RESOURCE_LIST) +
               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (RTL_NUMBER_OF(Ports) - 1);
    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!ResourceList)
        return STATUS_NO_MEMORY;

    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Internal;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = RTL_NUMBER_OF(Ports);

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
    for (i = 0; i < RTL_NUMBER_OF(Ports); i++)
    {
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Length = 0x01;
        Descriptor->u.Port.Start.LowPart = Ports[i];

        Descriptor++;
    }

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateReadPortDO(
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    PISAPNP_PDO_EXTENSION PdoExt;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(ReadPortCreated == FALSE);

    DPRINT("Creating Read Port\n");

    Status = IoCreateDevice(FdoExt->DriverObject,
                            sizeof(ISAPNP_PDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &FdoExt->ReadPortPdo);
    if (!NT_SUCCESS(Status))
        return Status;

    PdoExt = FdoExt->ReadPortPdo->DeviceExtension;
    RtlZeroMemory(PdoExt, sizeof(ISAPNP_PDO_EXTENSION));
    PdoExt->Common.Signature = IsaPnpReadDataPort;
    PdoExt->Common.Self = FdoExt->ReadPortPdo;
    PdoExt->Common.State = dsStopped;
    PdoExt->FdoExt = FdoExt;

    Status = IsaPnpCreateReadPortDORequirements(PdoExt, 0);
    if (!NT_SUCCESS(Status))
        goto Failure;

    Status = IsaPnpCreateReadPortDOResources(PdoExt);
    if (!NT_SUCCESS(Status))
        goto Failure;

    FdoExt->ReadPortPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;

Failure:
    IsaPnpRemoveReadPortDO(FdoExt->ReadPortPdo);

    FdoExt->ReadPortPdo = NULL;

    return Status;
}

CODE_SEG("PAGE")
VOID
IsaPnpRemoveReadPortDO(
    _In_ PDEVICE_OBJECT Pdo)
{
    PISAPNP_PDO_EXTENSION ReadPortExt = Pdo->DeviceExtension;

    PAGED_CODE();

    DPRINT("Removing Read Port\n");

    if (ReadPortExt->RequirementsList)
        ExFreePoolWithTag(ReadPortExt->RequirementsList, TAG_ISAPNP);

    if (ReadPortExt->ResourceList)
        ExFreePoolWithTag(ReadPortExt->ResourceList, TAG_ISAPNP);

    IoDeleteDevice(Pdo);
}

CODE_SEG("PAGE")
NTSTATUS
IsaPnpFillDeviceRelations(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN IncludeDataPort)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY CurrentEntry;
    PISAPNP_LOGICAL_DEVICE IsaDevice;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG PdoCount, i = 0;

    PAGED_CODE();

    IsaPnpAcquireBusDataLock();

    /* Try to claim the Read Port for our FDO */
    if (!ReadPortCreated)
    {
        Status = IsaPnpCreateReadPortDO(FdoExt);
        if (!NT_SUCCESS(Status))
            return Status;

        ReadPortCreated = TRUE;
    }

    IsaPnpReleaseBusDataLock();

    /* Inactive ISA bus */
    if (!FdoExt->ReadPortPdo)
        IncludeDataPort = FALSE;

    IsaPnpAcquireDeviceDataLock(FdoExt);

    /* If called from the FDO dispatch routine && Active bus */
    if (IncludeDataPort && FdoExt->ReadPortPdo)
    {
        PISAPNP_PDO_EXTENSION ReadPortExt = FdoExt->ReadPortPdo->DeviceExtension;

        if ((ReadPortExt->Flags & ISAPNP_READ_PORT_ALLOW_FDO_SCAN) &&
            !(ReadPortExt->Flags & ISAPNP_SCANNED_BY_READ_PORT))
        {
            DPRINT("Rescan ISA PnP bus\n");

            /* Run the isolation protocol */
            FdoExt->Cards = IsaHwTryReadDataPort(FdoExt->ReadDataPort);

            /* Card identification */
            if (FdoExt->Cards > 0)
                (VOID)IsaHwFillDeviceList(FdoExt);

            IsaHwWaitForKey();
        }

        ReadPortExt->Flags &= ~ISAPNP_SCANNED_BY_READ_PORT;
    }

    PdoCount = FdoExt->DeviceCount;
    if (IncludeDataPort)
        ++PdoCount;

    CurrentEntry = FdoExt->DeviceListHead.Flink;
    while (CurrentEntry != &FdoExt->DeviceListHead)
    {
        IsaDevice = CONTAINING_RECORD(CurrentEntry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

        if (!(IsaDevice->Flags & ISAPNP_PRESENT))
            --PdoCount;

        CurrentEntry = CurrentEntry->Flink;
    }

    DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                            FIELD_OFFSET(DEVICE_RELATIONS, Objects[PdoCount]),
                                            TAG_ISAPNP);
    if (!DeviceRelations)
    {
        IsaPnpReleaseDeviceDataLock(FdoExt);
        return STATUS_NO_MEMORY;
    }

    if (IncludeDataPort)
    {
        PISAPNP_PDO_EXTENSION ReadPortExt = FdoExt->ReadPortPdo->DeviceExtension;

        DeviceRelations->Objects[i++] = FdoExt->ReadPortPdo;
        ObReferenceObject(FdoExt->ReadPortPdo);

        /* The Read Port PDO can only be removed by FDO */
        ReadPortExt->Flags |= ISAPNP_ENUMERATED;
    }

    CurrentEntry = FdoExt->DeviceListHead.Flink;
    while (CurrentEntry != &FdoExt->DeviceListHead)
    {
        PISAPNP_PDO_EXTENSION PdoExt;

        IsaDevice = CONTAINING_RECORD(CurrentEntry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

        if (!(IsaDevice->Flags & ISAPNP_PRESENT))
            goto SkipPdo;

        if (!IsaDevice->Pdo)
        {
            Status = IoCreateDevice(FdoExt->DriverObject,
                                    sizeof(ISAPNP_PDO_EXTENSION),
                                    NULL,
                                    FILE_DEVICE_CONTROLLER,
                                    FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
                                    FALSE,
                                    &IsaDevice->Pdo);
            if (!NT_SUCCESS(Status))
                goto SkipPdo;

            IsaDevice->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
            /* The power pagable flag is always unset */

            PdoExt = IsaDevice->Pdo->DeviceExtension;

            RtlZeroMemory(PdoExt, sizeof(ISAPNP_PDO_EXTENSION));
            PdoExt->Common.Signature = IsaPnpLogicalDevice;
            PdoExt->Common.Self = IsaDevice->Pdo;
            PdoExt->Common.State = dsStopped;
            PdoExt->IsaPnpDevice = IsaDevice;
            PdoExt->FdoExt = FdoExt;

            if (!NT_SUCCESS(IsaPnpCreateLogicalDeviceRequirements(PdoExt)) ||
                !NT_SUCCESS(IsaPnpCreateLogicalDeviceResources(PdoExt)))
            {
                if (PdoExt->RequirementsList)
                {
                    ExFreePoolWithTag(PdoExt->RequirementsList, TAG_ISAPNP);
                    PdoExt->RequirementsList = NULL;
                }

                if (PdoExt->ResourceList)
                {
                    ExFreePoolWithTag(PdoExt->ResourceList, TAG_ISAPNP);
                    PdoExt->ResourceList = NULL;
                }

                IoDeleteDevice(IsaDevice->Pdo);
                IsaDevice->Pdo = NULL;
                goto SkipPdo;
            }
        }
        else
        {
            PdoExt = IsaDevice->Pdo->DeviceExtension;
        }
        DeviceRelations->Objects[i++] = IsaDevice->Pdo;
        ObReferenceObject(IsaDevice->Pdo);

        PdoExt->Flags |= ISAPNP_ENUMERATED;

        CurrentEntry = CurrentEntry->Flink;
        continue;

SkipPdo:
        if (IsaDevice->Pdo)
        {
            PdoExt = IsaDevice->Pdo->DeviceExtension;

            if (PdoExt)
                PdoExt->Flags &= ~ISAPNP_ENUMERATED;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    IsaPnpReleaseDeviceDataLock(FdoExt);

    DeviceRelations->Count = i;

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return Status;
}

static CODE_SEG("PAGE") DRIVER_ADD_DEVICE IsaAddDevice;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT Fdo;
    PISAPNP_FDO_EXTENSION FdoExt;
    NTSTATUS Status;
    static ULONG BusNumber = 0;

    PAGED_CODE();

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

    Status = IoCreateDevice(DriverObject,
                            sizeof(*FdoExt),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create FDO (0x%08lx)\n", Status);
        return Status;
    }

    FdoExt = Fdo->DeviceExtension;
    RtlZeroMemory(FdoExt, sizeof(*FdoExt));

    FdoExt->Common.Self = Fdo;
    FdoExt->Common.Signature = IsaPnpBus;
    FdoExt->Common.State = dsStopped;
    FdoExt->DriverObject = DriverObject;
    FdoExt->BusNumber = BusNumber++;
    FdoExt->Pdo = PhysicalDeviceObject;
    FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                              PhysicalDeviceObject);
    if (!FdoExt->Ldo)
    {
        IoDeleteDevice(Fdo);
        return STATUS_DEVICE_REMOVED;
    }

    InitializeListHead(&FdoExt->DeviceListHead);
    KeInitializeEvent(&FdoExt->DeviceSyncEvent, SynchronizationEvent, TRUE);

    IsaPnpAcquireBusDataLock();
    InsertTailList(&BusListHead, &FdoExt->BusLink);
    IsaPnpReleaseBusDataLock();

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_POWER)
static DRIVER_DISPATCH_RAISED IsaPower;

static
NTSTATUS
NTAPI
IsaPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PISAPNP_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    if (DevExt->Signature != IsaPnpBus)
    {
        switch (IoGetCurrentIrpStackLocation(Irp)->MinorFunction)
        {
            case IRP_MN_SET_POWER:
            case IRP_MN_QUERY_POWER:
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                break;

            default:
                Status = Irp->IoStatus.Status;
                break;
        }

        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(((PISAPNP_FDO_EXTENSION)DevExt)->Ldo, Irp);
}

_Dispatch_type_(IRP_MJ_PNP)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaPnp;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PISAPNP_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    if (DevExt->Signature == IsaPnpBus)
        return IsaFdoPnp((PISAPNP_FDO_EXTENSION)DevExt, Irp, IrpSp);
    else
        return IsaPdoPnp((PISAPNP_PDO_EXTENSION)DevExt, Irp, IrpSp);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IsaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IsaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IsaForwardOrIgnore;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = IsaForwardOrIgnore;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IsaPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IsaPower;
    DriverObject->DriverExtension->AddDevice = IsaAddDevice;

    /* FIXME: Fix SDK headers */
#if 0
    _No_competing_thread_begin_
#endif

    KeInitializeEvent(&BusSyncEvent, SynchronizationEvent, TRUE);
    InitializeListHead(&BusListHead);

    /* FIXME: Fix SDK headers */
#if 0
    _No_competing_thread_end_
#endif

    return STATUS_SUCCESS;
}

/* EOF */
