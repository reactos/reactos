/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Driver entry
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

BOOLEAN ReadPortCreated = FALSE;

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

static IO_COMPLETION_ROUTINE ForwardIrpCompletion;

static
NTSTATUS
NTAPI
ForwardIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
IsaForwardIrpSynchronous(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, ForwardIrpCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(FdoExt->Ldo, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
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
    Irp->IoStatus.Information = FILE_OPENED;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

static DRIVER_DISPATCH IsaIoctl;

static
NTSTATUS
NTAPI
IsaIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        default:
            DPRINT1("Unknown ioctl code: %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static DRIVER_DISPATCH IsaReadWrite;

static
NTSTATUS
NTAPI
IsaReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaPnpCreateReadPortDORequirements(
    _In_ PISAPNP_PDO_EXTENSION PdoExt)
{
    ULONG ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    const ULONG Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS,
                            0x274, 0x3E4, 0x204, 0x2E4, 0x354, 0x2F4 };

    PAGED_CODE();

    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
               + 2 * RTL_NUMBER_OF(Ports) * sizeof(IO_RESOURCE_DESCRIPTOR);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_ISAPNP);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RequirementsList->ListSize = ListSize;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = 2 * RTL_NUMBER_OF(Ports);

    for (i = 0; i < 2 * RTL_NUMBER_OF(Ports); i += 2)
    {
        Descriptor = &RequirementsList->List[0].Descriptors[i];

        /* Expected port */
        Descriptor[0].Type = CmResourceTypePort;
        Descriptor[0].ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor[0].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor[0].u.Port.Length = Ports[i / 2] & 1 ? 0x01 : 0x04;
        Descriptor[0].u.Port.Alignment = 0x01;
        Descriptor[0].u.Port.MinimumAddress.LowPart = Ports[i / 2];
        Descriptor[0].u.Port.MaximumAddress.LowPart = Ports[i / 2] + Descriptor[0].u.Port.Length - 1;

        /* ... but mark it as optional */
        Descriptor[1].Option = IO_RESOURCE_ALTERNATIVE;
        Descriptor[1].Type = CmResourceTypePort;
        Descriptor[1].ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor[1].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor[1].u.Port.Alignment = 0x01;
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
    PdoExt->Common.IsFdo = FALSE;
    PdoExt->Common.Self = FdoExt->ReadPortPdo;
    PdoExt->Common.State = dsStopped;
    PdoExt->FdoExt = FdoExt;

    Status = IsaPnpCreateReadPortDORequirements(PdoExt);
    if (!NT_SUCCESS(Status))
        goto Failure;

    Status = IsaPnpCreateReadPortDOResources(PdoExt);
    if (!NT_SUCCESS(Status))
        goto Failure;

    FdoExt->ReadPortPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;

Failure:
    if (PdoExt->RequirementsList)
        ExFreePoolWithTag(PdoExt->RequirementsList, TAG_ISAPNP);

    if (PdoExt->ResourceList)
        ExFreePoolWithTag(PdoExt->ResourceList, TAG_ISAPNP);

    IoDeleteDevice(FdoExt->ReadPortPdo);

    return Status;
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

    /* Try to claim the Read Port for our FDO */
    if (!ReadPortCreated)
    {
        Status = IsaPnpCreateReadPortDO(FdoExt);
        if (!NT_SUCCESS(Status))
            return Status;

        ReadPortCreated = TRUE;
    }

    /* Inactive ISA bus */
    if (!FdoExt->ReadPortPdo)
        IncludeDataPort = FALSE;

    PdoCount = FdoExt->DeviceCount;
    if (IncludeDataPort)
        ++PdoCount;

    DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                            FIELD_OFFSET(DEVICE_RELATIONS, Objects[PdoCount]),
                                            TAG_ISAPNP);
    if (!DeviceRelations)
    {
        return STATUS_NO_MEMORY;
    }

    if (IncludeDataPort)
    {
        DeviceRelations->Objects[i++] = FdoExt->ReadPortPdo;
        ObReferenceObject(FdoExt->ReadPortPdo);
    }

    CurrentEntry = FdoExt->DeviceListHead.Flink;
    while (CurrentEntry != &FdoExt->DeviceListHead)
    {
        PISAPNP_PDO_EXTENSION PdoExt;

        IsaDevice = CONTAINING_RECORD(CurrentEntry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

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
            {
                break;
            }

            IsaDevice->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

            //Device->Pdo->Flags |= DO_POWER_PAGABLE;

            PdoExt = IsaDevice->Pdo->DeviceExtension;

            RtlZeroMemory(PdoExt, sizeof(ISAPNP_PDO_EXTENSION));

            PdoExt->Common.IsFdo = FALSE;
            PdoExt->Common.Self = IsaDevice->Pdo;
            PdoExt->Common.State = dsStopped;
            PdoExt->IsaPnpDevice = IsaDevice;
            PdoExt->FdoExt = FdoExt;

            Status = IsaPnpCreateLogicalDeviceRequirements(PdoExt);

            if (NT_SUCCESS(Status))
                Status = IsaPnpCreateLogicalDeviceResources(PdoExt);

            if (!NT_SUCCESS(Status))
            {
                IoDeleteDevice(IsaDevice->Pdo);
                IsaDevice->Pdo = NULL;
                break;
            }
        }
        DeviceRelations->Objects[i++] = IsaDevice->Pdo;

        ObReferenceObject(IsaDevice->Pdo);

        CurrentEntry = CurrentEntry->Flink;
    }

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

    PAGED_CODE();

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

    Status = IoCreateDevice(DriverObject,
                            sizeof(*FdoExt),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            TRUE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create FDO (0x%08lx)\n", Status);
        return Status;
    }

    FdoExt = Fdo->DeviceExtension;
    RtlZeroMemory(FdoExt, sizeof(*FdoExt));

    FdoExt->Common.Self = Fdo;
    FdoExt->Common.IsFdo = TRUE;
    FdoExt->Common.State = dsStopped;
    FdoExt->DriverObject = DriverObject;
    FdoExt->Pdo = PhysicalDeviceObject;
    FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                              PhysicalDeviceObject);

    InitializeListHead(&FdoExt->DeviceListHead);
    KeInitializeEvent(&FdoExt->DeviceSyncEvent, SynchronizationEvent, TRUE);

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

    if (!DevExt->IsFdo)
    {
        Status = Irp->IoStatus.Status;
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

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    if (DevExt->IsFdo)
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
    DriverObject->MajorFunction[IRP_MJ_READ] = IsaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = IsaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IsaIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IsaPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IsaPower;
    DriverObject->DriverExtension->AddDevice = IsaAddDevice;

    return STATUS_SUCCESS;
}

/* EOF */
