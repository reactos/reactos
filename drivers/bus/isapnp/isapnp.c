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

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KEVENT BusSyncEvent;

_Guarded_by_(BusSyncEvent)
BOOLEAN ReadPortCreated = FALSE;

_Guarded_by_(BusSyncEvent)
LIST_ENTRY BusListHead;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateLogicalDeviceRequirements(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    RTL_BITMAP IrqBitmap[RTL_NUMBER_OF(LogDev->Irq)];
    RTL_BITMAP DmaBitmap[RTL_NUMBER_OF(LogDev->Dma)];
    ULONG IrqData[RTL_NUMBER_OF(LogDev->Irq)];
    ULONG DmaData[RTL_NUMBER_OF(LogDev->Dma)];
    ULONG ResourceCount = 0;
    ULONG ListSize, i, j;
    BOOLEAN FirstIrq = TRUE, FirstDma = TRUE;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

    /* Count number of requirements */
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].Description.Length)
            break;

        ResourceCount++;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            break;

        IrqData[i] = LogDev->Irq[i].Description.Mask;
        RtlInitializeBitMap(&IrqBitmap[i], &IrqData[i], 16);
        ResourceCount += RtlNumberOfSetBits(&IrqBitmap[i]);

        if (LogDev->Irq[i].Description.Information & 0x4)
        {
            /* Add room for level sensitive */
            ResourceCount += RtlNumberOfSetBits(&IrqBitmap[i]);
        }
    }
    if (ResourceCount == 0)
        return STATUS_SUCCESS;
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            break;

        DmaData[i] = LogDev->Dma[i].Description.Mask;
        RtlInitializeBitMap(&DmaBitmap[i], &DmaData[i], 8);
        ResourceCount += RtlNumberOfSetBits(&DmaBitmap[i]);
    }

    /* Allocate memory to store requirements */
    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
               + ResourceCount * sizeof(IO_RESOURCE_DESCRIPTOR);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RequirementsList->ListSize = ListSize;
    RequirementsList->InterfaceType = Isa;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = ResourceCount;

    /* Store requirements */
    Descriptor = RequirementsList->List[0].Descriptors;
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].Description.Length)
            break;

        DPRINT("Device.Io[%d].Information = 0x%02x\n", i, LogDev->Io[i].Description.Information);
        DPRINT("Device.Io[%d].Minimum = 0x%02x\n", i, LogDev->Io[i].Description.Minimum);
        DPRINT("Device.Io[%d].Maximum = 0x%02x\n", i, LogDev->Io[i].Description.Maximum);
        DPRINT("Device.Io[%d].Alignment = 0x%02x\n", i, LogDev->Io[i].Description.Alignment);
        DPRINT("Device.Io[%d].Length = 0x%02x\n", i, LogDev->Io[i].Description.Length);

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Io[i].Description.Information & 0x1)
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        else
            Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Length = LogDev->Io[i].Description.Length;
        Descriptor->u.Port.Alignment = LogDev->Io[i].Description.Alignment;
        Descriptor->u.Port.MinimumAddress.LowPart = LogDev->Io[i].Description.Minimum;
        Descriptor->u.Port.MaximumAddress.LowPart =
            LogDev->Io[i].Description.Maximum + LogDev->Io[i].Description.Length - 1;
        Descriptor++;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            break;

        DPRINT("Device.Irq[%d].Mask = 0x%02x\n", i, LogDev->Irq[i].Description.Mask);
        DPRINT("Device.Irq[%d].Information = 0x%02x\n", i, LogDev->Irq[i].Description.Information);

        for (j = 0; j < 15; j++)
        {
            if (!RtlCheckBit(&IrqBitmap[i], j))
                continue;

            if (FirstIrq)
                FirstIrq = FALSE;
            else
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            Descriptor->Type = CmResourceTypeInterrupt;
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
            Descriptor->u.Interrupt.MinimumVector = Descriptor->u.Interrupt.MaximumVector = j;
            Descriptor++;

            if (LogDev->Irq[i].Description.Information & 0x4)
            {
                /* Level interrupt */
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                Descriptor->Type = CmResourceTypeInterrupt;
                Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                Descriptor->u.Interrupt.MinimumVector = Descriptor->u.Interrupt.MaximumVector = j;
                Descriptor++;
            }
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            break;

        DPRINT("Device.Dma[%d].Mask = 0x%02x\n", i, LogDev->Dma[i].Description.Mask);
        DPRINT("Device.Dma[%d].Information = 0x%02x\n", i, LogDev->Dma[i].Description.Information);

        for (j = 0; j < 8; j++)
        {
            if (!RtlCheckBit(&DmaBitmap[i], j))
                continue;

            if (FirstDma)
                FirstDma = FALSE;
            else
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            Descriptor->Type = CmResourceTypeDma;
            switch (LogDev->Dma[i].Description.Information & 0x3)
            {
                case 0x0: Descriptor->Flags |= CM_RESOURCE_DMA_8; break;
                case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_8_AND_16; break;
                case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_16; break;
                default: break;
            }
            if (LogDev->Dma[i].Description.Information & 0x4)
                Descriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
            switch ((LogDev->Dma[i].Description.Information >> 5) & 0x3)
            {
                case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
                case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
                case 0x3: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
                default: break;
            }
            Descriptor->u.Dma.MinimumChannel = Descriptor->u.Dma.MaximumChannel = j;
            Descriptor++;
        }
    }

    PdoExt->RequirementsList = RequirementsList;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateLogicalDeviceResources(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    ULONG ResourceCount = 0;
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

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
        if (!LogDev->Io[i].CurrentBase)
            break;

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Io[i].Description.Information & 0x1)
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        else
            Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Length = LogDev->Io[i].Description.Length;
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
        Descriptor->u.Interrupt.Affinity = -1;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel == 4)
            break;

        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeDma;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        switch (LogDev->Dma[i].Description.Information & 0x3)
        {
            case 0x0: Descriptor->Flags |= CM_RESOURCE_DMA_8; break;
            case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_8 | CM_RESOURCE_DMA_16; break;
            case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_16; break;
            default: break;
        }
        if (LogDev->Dma[i].Description.Information & 0x4)
            Descriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
        switch ((LogDev->Dma[i].Description.Information >> 5) & 0x3)
        {
            case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
            case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
            case 0x3: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
            default: break;
        }
        Descriptor->u.Dma.Channel = LogDev->Dma[i].CurrentChannel;
    }

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
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

    for (i = 0; i < RTL_NUMBER_OF(Ports); i++)
    {
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Length = 0x01;
        Descriptor->u.Port.Start.LowPart = Ports[i];
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
                            FILE_DEVICE_SECURE_OPEN,
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
            if (NT_SUCCESS(IsaHwTryReadDataPort(FdoExt->ReadDataPort)))
            {
                /* Card identification */
                (VOID)IsaHwFillDeviceList(FdoExt);
            }
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
