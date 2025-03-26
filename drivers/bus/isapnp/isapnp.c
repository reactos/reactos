/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Driver entry
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *                  Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#ifndef UNIT_TEST

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KEVENT BusSyncEvent;

_Guarded_by_(BusSyncEvent)
BOOLEAN ReadPortCreated = FALSE;

_Guarded_by_(BusSyncEvent)
LIST_ENTRY BusListHead;

#endif /* UNIT_TEST */

extern ULONG IsaConfigPorts[2];

/* FUNCTIONS ******************************************************************/

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

/*
 * For example, the PnP ROM
 * 0x15, 0x04, ...                                 // Logical device ID
 * 0x47, 0x01, 0x30, 0x03, 0x30, 0x03, 0x04, 0x04, // IO 330, len 4, align 4
 * 0x30,                                           // **** Start DF ****
 * 0x22, 0x04, 0x00,                               // IRQ 2
 * 0x31, 0x02,                                     // **** Start DF ****
 * 0x22, 0xC0, 0x00,                               // IRQ 6 or 7
 * 0x38,                                           // **** End DF ******
 * 0x2A, 0x20, 0x3A,                               // DMA 5
 * 0x22, 0x00, 0x08,                               // IRQ 12
 * 0x79, 0x00,                                     // END
 *
 * becomes the following resource requirements list:
 * Interface 1 Bus 0 Slot 0 AlternativeLists 2
 *
 * AltList #0, AltList->Count 4
 * [Option 0, ShareDisposition 1, Flags 11] IO: Min 0:330, Max 0:333, Align 4 Len 4
 * [Option 0, ShareDisposition 1, Flags 1]  INT: Min 2 Max 2
 * [Option 0, ShareDisposition 0, Flags 0]  DMA: Min 5 Max 5
 * [Option 0, ShareDisposition 1, Flags 1]  INT: Min B Max B
 *
 * AltList #1, AltList->Count 5
 * [Option 0, ShareDisposition 1, Flags 11] IO: Min 0:330, Max 0:333, Align 4 Len 4
 * [Option 0, ShareDisposition 1, Flags 1]  INT: Min 6 Max 6
 * [Option 8, ShareDisposition 1, Flags 1]  INT: Min 7 Max 7
 * [Option 0, ShareDisposition 0, Flags 0]  DMA: Min 5 Max 5
 * [Option 0, ShareDisposition 1, Flags 1]  INT: Min B Max B
 */
static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateLogicalDeviceRequirements(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ISAPNP_DEPENDENT_FUNCTION_STATE DfState;
    ULONG FirstFixedDescriptors, LastFixedDescriptors;
    ULONG ResourceCount, AltListCount, ListSize, i;
    BOOLEAN IsFirstAltList, IsFirstDescriptor;
    PIO_RESOURCE_LIST AltList;
    PISAPNP_RESOURCE Resource;

    PAGED_CODE();

    /* Count the number of requirements */
    DfState = dfNotStarted;
    FirstFixedDescriptors = 0;
    LastFixedDescriptors = 0;
    ResourceCount = 0;
    AltListCount = 1;
    Resource = PdoExt->IsaPnpDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        switch (Resource->Type)
        {
            case ISAPNP_RESOURCE_TYPE_START_DEPENDENT:
            {
                if (DfState == dfStarted)
                    ++AltListCount;

                DfState = dfStarted;
                break;
            }

            case ISAPNP_RESOURCE_TYPE_END_DEPENDENT:
            {
                DfState = dfDone;
                break;
            }

            case ISAPNP_RESOURCE_TYPE_IRQ:
            case ISAPNP_RESOURCE_TYPE_DMA:
            {
                RTL_BITMAP ResourceBitmap;
                ULONG BitmapSize, BitmapBuffer, BitCount;

                if (Resource->Type == ISAPNP_RESOURCE_TYPE_IRQ)
                {
                    BitmapSize = RTL_BITS_OF(Resource->IrqDescription.Mask);
                    BitmapBuffer = Resource->IrqDescription.Mask;
                }
                else
                {
                    BitmapSize = RTL_BITS_OF(Resource->DmaDescription.Mask);
                    BitmapBuffer = Resource->DmaDescription.Mask;
                }
                RtlInitializeBitMap(&ResourceBitmap, &BitmapBuffer, BitmapSize);

                BitCount = RtlNumberOfSetBits(&ResourceBitmap);
                switch (DfState)
                {
                    case dfNotStarted:
                        FirstFixedDescriptors += BitCount;
                        break;

                    case dfStarted:
                        ResourceCount += BitCount;
                        break;

                    case dfDone:
                        LastFixedDescriptors += BitCount;
                        break;

                    DEFAULT_UNREACHABLE;
                }

                break;
            }

            case ISAPNP_RESOURCE_TYPE_IO:
            case ISAPNP_RESOURCE_TYPE_MEMRANGE:
            case ISAPNP_RESOURCE_TYPE_MEMRANGE32:
            {
                switch (DfState)
                {
                    case dfNotStarted:
                        ++FirstFixedDescriptors;
                        break;

                    case dfStarted:
                        ++ResourceCount;
                        break;

                    case dfDone:
                        ++LastFixedDescriptors;
                        break;

                    DEFAULT_UNREACHABLE;
                }
                break;
            }

            default:
                ASSERT(FALSE);
                UNREACHABLE;
                break;
        }

        ++Resource;
    }

    /* This logical device has no resource requirements */
    if ((ResourceCount == 0) && (FirstFixedDescriptors == 0) && (LastFixedDescriptors == 0))
        return STATUS_SUCCESS;

    /* Allocate memory to store requirements */
    ListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List) +
               FIELD_OFFSET(IO_RESOURCE_LIST, Descriptors) * AltListCount +
               sizeof(IO_RESOURCE_DESCRIPTOR) * ResourceCount +
               sizeof(IO_RESOURCE_DESCRIPTOR) * AltListCount *
               (FirstFixedDescriptors + LastFixedDescriptors);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RequirementsList->ListSize = ListSize;
    RequirementsList->InterfaceType = Isa;
    RequirementsList->AlternativeLists = AltListCount;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;

    /* Store requirements */
    IsFirstAltList = TRUE;
    AltList = &RequirementsList->List[0];
    Descriptor = &RequirementsList->List[0].Descriptors[0];
    Resource = PdoExt->IsaPnpDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        switch (Resource->Type)
        {
            case ISAPNP_RESOURCE_TYPE_START_DEPENDENT:
            {
                if (!IsFirstAltList)
                {
                    /* Add room for the fixed descriptors */
                    AltList->Count += LastFixedDescriptors;

                    /* Move on to the next list */
                    AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
                    AltList->Version = 1;
                    AltList->Revision = 1;

                    /* Propagate the fixed resources to our new list */
                    RtlCopyMemory(&AltList->Descriptors,
                                  RequirementsList->List[0].Descriptors,
                                  sizeof(IO_RESOURCE_DESCRIPTOR) * FirstFixedDescriptors);
                    AltList->Count += FirstFixedDescriptors;

                    Descriptor = &AltList->Descriptors[FirstFixedDescriptors];
                }

                IsFirstAltList = FALSE;
                break;
            }

            case ISAPNP_RESOURCE_TYPE_END_DEPENDENT:
                break;

            case ISAPNP_RESOURCE_TYPE_IO:
            {
                IsaConvertIoRequirement(Descriptor++, &Resource->IoDescription);

                ++AltList->Count;
                break;
            }

            case ISAPNP_RESOURCE_TYPE_IRQ:
            {
                IsFirstDescriptor = TRUE;

                for (i = 0; i < RTL_BITS_OF(Resource->IrqDescription.Mask); i++)
                {
                    if (!(Resource->IrqDescription.Mask & (1 << i)))
                        continue;

                    IsaConvertIrqRequirement(Descriptor++,
                                             &Resource->IrqDescription,
                                             i,
                                             IsFirstDescriptor);
                    ++AltList->Count;

                    IsFirstDescriptor = FALSE;
                }

                break;
            }

            case ISAPNP_RESOURCE_TYPE_DMA:
            {
                IsFirstDescriptor = TRUE;

                for (i = 0; i < RTL_BITS_OF(Resource->DmaDescription.Mask); i++)
                {
                    if (!(Resource->DmaDescription.Mask & (1 << i)))
                        continue;

                    IsaConvertDmaRequirement(Descriptor++,
                                             &Resource->DmaDescription,
                                             i,
                                             IsFirstDescriptor);
                    ++AltList->Count;

                    IsFirstDescriptor = FALSE;
                }

                break;
            }

            case ISAPNP_RESOURCE_TYPE_MEMRANGE:
            {
                IsaConvertMemRangeRequirement(Descriptor++, &Resource->MemRangeDescription);

                ++AltList->Count;
                break;
            }

            case ISAPNP_RESOURCE_TYPE_MEMRANGE32:
            {
                IsaConvertMemRange32Requirement(Descriptor++, &Resource->MemRange32Description);

                ++AltList->Count;
                break;
            }

            default:
                ASSERT(FALSE);
                UNREACHABLE;
                break;
        }

        ++Resource;
    }

    /* Append the fixed resources */
    if (LastFixedDescriptors)
    {
        PIO_RESOURCE_LIST NextList = &RequirementsList->List[0];

        /* Make the descriptor point to the fixed resources */
        Descriptor -= LastFixedDescriptors;

        /* Propagate the fixed resources onto previous lists */
        AltListCount = RequirementsList->AlternativeLists - 1;
        for (i = 0; i < AltListCount; i++)
        {
            RtlCopyMemory(&NextList->Descriptors[NextList->Count - LastFixedDescriptors],
                          Descriptor,
                          sizeof(IO_RESOURCE_DESCRIPTOR) * LastFixedDescriptors);

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
    _Out_opt_ PULONG Length)
{
    PISAPNP_RESOURCE Resource;

    PAGED_CODE();

    Resource = LogDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        if (Resource->Type == ISAPNP_RESOURCE_TYPE_IO)
        {
            PISAPNP_IO_DESCRIPTION Description = &Resource->IoDescription;
            BOOLEAN Match;

            if (Base)
            {
                Match = (Base >= Description->Minimum) && (Base <= Description->Maximum);
            }
            else
            {
                Match = (RangeStart >= Description->Minimum) &&
                        (RangeEnd <= (ULONG)(Description->Maximum + Description->Length - 1));
            }

            if (Match)
            {
                if (Information)
                    *Information = Description->Information;
                if (Length)
                    *Length = Description->Length;

                return TRUE;
            }
        }

        ++Resource;
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindIrqDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Vector)
{
    PISAPNP_RESOURCE Resource;

    PAGED_CODE();

    Resource = LogDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        if (Resource->Type == ISAPNP_RESOURCE_TYPE_IRQ)
        {
            PISAPNP_IRQ_DESCRIPTION Description = &Resource->IrqDescription;

            if (Description->Mask & (1 << Vector))
                return TRUE;
        }

        ++Resource;
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindDmaDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Channel)
{
    PISAPNP_RESOURCE Resource;

    PAGED_CODE();

    Resource = LogDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        if (Resource->Type == ISAPNP_RESOURCE_TYPE_DMA)
        {
            PISAPNP_DMA_DESCRIPTION Description = &Resource->DmaDescription;

            if (Description->Mask & (1 << Channel))
                return TRUE;
        }

        ++Resource;
    }

    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
FindMemoryDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG RangeStart,
    _In_ ULONG RangeEnd,
    _Out_opt_ PUCHAR Information)
{
    PISAPNP_RESOURCE Resource;

    PAGED_CODE();

    Resource = LogDevice->Resources;
    while (Resource->Type != ISAPNP_RESOURCE_TYPE_END)
    {
        switch (Resource->Type)
        {
            case ISAPNP_RESOURCE_TYPE_MEMRANGE:
            {
                PISAPNP_MEMRANGE_DESCRIPTION Description;

                Description = &Resource->MemRangeDescription;

                if ((RangeStart >= (ULONG)(Description->Minimum << 8)) &&
                    (RangeEnd <= (ULONG)((Description->Maximum << 8) +
                                         (Description->Length << 8) - 1)))
                {
                    if (Information)
                        *Information = Description->Information;

                    return TRUE;
                }
                break;
            }

            case ISAPNP_RESOURCE_TYPE_MEMRANGE32:
            {
                PISAPNP_MEMRANGE32_DESCRIPTION Description32;

                Description32 = &Resource->MemRange32Description;

                if ((RangeStart >= Description32->Minimum) &&
                    (RangeEnd <= (Description32->Maximum + Description32->Length - 1)))
                {
                    if (Information)
                        *Information = Description32->Information;

                    return TRUE;
                }
                break;
            }

            default:
                break;
        }

        ++Resource;
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
        if (LogDev->Dma[i].CurrentChannel != DMACHANNEL_NONE)
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
                              &CurrentLength))
        {
            DPRINT1("I/O entry #%lu %x not found\n", i, LogDev->Io[i].CurrentBase);
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

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Irq[i].CurrentType & 0x01)
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        else
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->u.Interrupt.Level = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Vector = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel == DMACHANNEL_NONE)
            break;

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
                                  &Information))
        {
            DPRINT1("MEM entry #%lu %lx %lx not found\n",
                    i,
                    LogDev->MemRange[i].CurrentBase,
                    LogDev->MemRange[i].CurrentLength);
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
        Descriptor->u.Memory.Length = LogDev->MemRange[i].CurrentLength;
        Descriptor->u.Memory.Start.QuadPart = LogDev->MemRange[i].CurrentBase;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->MemRange32); i++)
    {
        if (!LogDev->MemRange32[i].CurrentBase)
            break;

        if (!FindMemoryDescriptor(LogDev,
                                  LogDev->MemRange32[i].CurrentBase,
                                  LogDev->MemRange32[i].CurrentLength,
                                  &Information))
        {
            DPRINT1("MEM32 entry #%lu %lx %lx not found\n",
                    i,
                    LogDev->MemRange32[i].CurrentBase,
                    LogDev->MemRange32[i].CurrentLength);
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
        Descriptor->u.Memory.Length = LogDev->MemRange32[i].CurrentLength;
        Descriptor->u.Memory.Start.QuadPart = LogDev->MemRange32[i].CurrentBase;
    }

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
    return STATUS_SUCCESS;

InvalidBiosResources:
    DPRINT1("Invalid boot resources! (CSN %u, LDN %u)\n", LogDev->CSN, LogDev->LDN);

    LogDev->Flags &= ~ISAPNP_HAS_RESOURCES;
    ExFreePoolWithTag(ResourceList, TAG_ISAPNP);
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
PIO_RESOURCE_REQUIREMENTS_LIST
IsaPnpCreateReadPortDORequirements(
    _In_opt_ ULONG SelectedReadPort)
{
    ULONG ResourceCount, ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
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
        ResourceCount = RTL_NUMBER_OF(IsaConfigPorts) * 2 + RTL_NUMBER_OF(ReadPorts);
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
        ResourceCount = (RTL_NUMBER_OF(IsaConfigPorts) + RTL_NUMBER_OF(ReadPorts)) * 2;
    }
    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               sizeof(IO_RESOURCE_DESCRIPTOR) * (ResourceCount - 1);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return NULL;

    RequirementsList->ListSize = ListSize;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = ResourceCount;

    Descriptor = &RequirementsList->List[0].Descriptors[0];

    /* Store the Data port and the Address port */
    for (i = 0; i < RTL_NUMBER_OF(IsaConfigPorts) * 2; i++)
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
            Descriptor->u.Port.MaximumAddress.LowPart = IsaConfigPorts[i / 2];
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

    return RequirementsList;
}

CODE_SEG("PAGE")
PCM_RESOURCE_LIST
IsaPnpCreateReadPortDOResources(VOID)
{
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

    ListSize = sizeof(CM_RESOURCE_LIST) +
               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (RTL_NUMBER_OF(IsaConfigPorts) - 1);
    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!ResourceList)
        return NULL;

    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Internal;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = RTL_NUMBER_OF(IsaConfigPorts);

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
    for (i = 0; i < RTL_NUMBER_OF(IsaConfigPorts); i++)
    {
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Length = 0x01;
        Descriptor->u.Port.Start.LowPart = IsaConfigPorts[i];

        Descriptor++;
    }

    return ResourceList;
}

#ifndef UNIT_TEST

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

    FdoExt->ReadPortPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;
}

CODE_SEG("PAGE")
VOID
IsaPnpRemoveReadPortDO(
    _In_ PDEVICE_OBJECT Pdo)
{
    PAGED_CODE();

    DPRINT("Removing Read Port\n");

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

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

    if (IsNEC_98)
    {
        IsaConfigPorts[0] = ISAPNP_WRITE_DATA_PC98;
        IsaConfigPorts[1] = ISAPNP_ADDRESS_PC98;
    }

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

#endif /* UNIT_TEST */

/* EOF */
