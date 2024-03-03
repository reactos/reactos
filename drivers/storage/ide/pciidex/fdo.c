/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     IRP_MJ_PNP operations for FDOs
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoParseResources(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR BusMasterDescriptor = NULL;
    PVOID IoBase;
    ULONG i;

    PAGED_CODE();

    if (!ResourcesTranslated)
        return STATUS_INVALID_PARAMETER;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                switch (Descriptor->u.Port.Length)
                {
                    /* Bus master port base */
                    case 16:
                    {
                        if (!BusMasterDescriptor)
                            BusMasterDescriptor = Descriptor;
                        break;
                    }

                    default:
                        break;
                }
            }

            default:
                break;
        }
    }

    if (!BusMasterDescriptor)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    if ((BusMasterDescriptor->Type == CmResourceTypePort) &&
        (BusMasterDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)BusMasterDescriptor->u.Port.Start.QuadPart;
    }
    else
    {
        IoBase = MmMapIoSpace(BusMasterDescriptor->u.Memory.Start, 16, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        FdoExtension->Flags |= FDO_IO_BASE_MAPPED;
    }
    FdoExtension->BusMasterPortBase = IoBase;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStartDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    if (!NT_VERIFY(IoForwardIrpSynchronously(FdoExtension->Ldo, Irp)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Status = PciIdeXFdoParseResources(FdoExtension,
                                      IoStack->Parameters.StartDevice.AllocatedResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to parse resources 0x%lx\n", Status);
        return Status;
    }

    Status = PciIdeXStartMiniport(FdoExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Miniport initialization failed 0x%lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
PciIdeXFdoFreeResources(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    if (FdoExtension->Flags & FDO_IO_BASE_MAPPED)
    {
        MmUnmapIoSpace(FdoExtension->BusMasterPortBase, 16);
        FdoExtension->Flags &= ~FDO_IO_BASE_MAPPED;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStopDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    PciIdeXFdoFreeResources(FdoExtension);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoRemoveDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoExtension;
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();

    PciIdeXFdoFreeResources(FdoExtension);

    ExAcquireFastMutex(&FdoExtension->DeviceSyncMutex);

    for (i = 0; i < MAX_IDE_CHANNEL; ++i)
    {
        PdoExtension = FdoExtension->Channels[i];

        if (PdoExtension)
        {
            IoDeleteDevice(PdoExtension->Common.Self);

            FdoExtension->Channels[i] = NULL;
            break;
        }
    }

    ExReleaseFastMutex(&FdoExtension->DeviceSyncMutex);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(FdoExtension->Ldo, Irp);

    IoDetachDevice(FdoExtension->Ldo);
    IoDeleteDevice(FdoExtension->Common.Self);

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryPnpDeviceState(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (FdoExtension->Common.PageFiles ||
        FdoExtension->Common.HibernateFiles ||
        FdoExtension->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
PPDO_DEVICE_EXTENSION
PciIdeXPdoCreateDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG ChannelNumber)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\PciIde999Channel9-FFF")];
    PDEVICE_OBJECT Pdo;
    PPDO_DEVICE_EXTENSION PdoExtension;
    ULONG Alignment;
    static ULONG DeviceNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\PciIde%uChannel%u-%x",
                                FdoExtension->ControllerNumber,
                                ChannelNumber,
                                DeviceNumber++);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(FdoExtension->Common.Self->DriverObject,
                            sizeof(*PdoExtension),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create PDO 0x%lx\n", Status);
        return NULL;
    }

    DPRINT("Created device object %p '%wZ'\n", Pdo, &DeviceName);

    /* DMA buffers alignment */
    Alignment = FdoExtension->Properties.AlignmentRequirement;
    Alignment = max(Alignment, FdoExtension->Common.Self->AlignmentRequirement);
    Alignment = max(Alignment, FILE_WORD_ALIGNMENT);
    Pdo->AlignmentRequirement = Alignment;

    PdoExtension = Pdo->DeviceExtension;

    RtlZeroMemory(PdoExtension, sizeof(*PdoExtension));
    PdoExtension->Common.Self = Pdo;
    PdoExtension->Channel = ChannelNumber;
    PdoExtension->ParentController = FdoExtension;

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return PdoExtension;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryBusRelations(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoExtension;
    IDE_CHANNEL_STATE ChannelState;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i;

    PAGED_CODE();

    DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                            FIELD_OFFSET(DEVICE_RELATIONS,
                                                         Objects[MAX_IDE_CHANNEL]),
                                            TAG_PCIIDEX);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 0;

    ExAcquireFastMutex(&FdoExtension->DeviceSyncMutex);

    for (i = 0; i < MAX_IDE_CHANNEL; ++i)
    {
        PdoExtension = FdoExtension->Channels[i];

        /* Ignore disabled channels */
        ChannelState = PciIdeXChannelState(FdoExtension, i);
        if (ChannelState == ChannelDisabled)
        {
            if (PdoExtension)
            {
                PdoExtension->ReportedMissing = TRUE;
            }

            DPRINT("Channel %lu is disabled\n", i);
            continue;
        }

        /* Need to create a PDO */
        if (!PdoExtension)
        {
            PdoExtension = PciIdeXPdoCreateDevice(FdoExtension, i);

            FdoExtension->Channels[i] = PdoExtension;
        }

        if (PdoExtension && !PdoExtension->ReportedMissing)
        {
            DeviceRelations->Objects[DeviceRelations->Count++] = PdoExtension->Common.Self;
            ObReferenceObject(PdoExtension->Common.Self);
        }
    }

    ExReleaseFastMutex(&FdoExtension->DeviceSyncMutex);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryDeviceUsageNotification(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (!NT_VERIFY(IoForwardIrpSynchronously(FdoExtension->Ldo, Irp)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &FdoExtension->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &FdoExtension->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &FdoExtension->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryInterface(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PAGED_CODE();

    if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                           &GUID_TRANSLATOR_INTERFACE_STANDARD))
    {
        CM_RESOURCE_TYPE ResourceType;
        ULONG BusNumber;

        ResourceType = (ULONG_PTR)IoStack->Parameters.QueryInterface.InterfaceSpecificData;

        /* In native mode the IDE controller does not use any legacy interrupt resources */
        if ((FdoExtension->Flags & FDO_IN_NATIVE_MODE) ||
            ResourceType != CmResourceTypeInterrupt ||
            IoStack->Parameters.QueryInterface.Size < sizeof(TRANSLATOR_INTERFACE))
        {
            return STATUS_NOT_SUPPORTED;
        }

        return HalGetInterruptTranslator(PCIBus,
                                         0,
                                         InterfaceTypeUndefined,
                                         sizeof(TRANSLATOR_INTERFACE),
                                         IoStack->Parameters.QueryInterface.Version,
                                         (PTRANSLATOR_INTERFACE)IoStack->
                                         Parameters.QueryInterface.Interface,
                                         &BusNumber);
    }

    return STATUS_NOT_SUPPORTED;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            Status = PciIdeXFdoStartDevice(FdoExtension, Irp);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return Status;
        }

        case IRP_MN_STOP_DEVICE:
        {
            Status = PciIdeXFdoStopDevice(FdoExtension);
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
            return PciIdeXFdoRemoveDevice(FdoExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            Status = PciIdeXFdoQueryPnpDeviceState(FdoExtension, Irp);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = PciIdeXFdoQueryBusRelations(FdoExtension, Irp);
            if (!NT_SUCCESS(Status))
            {
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                return Status;
            }

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            Status = PciIdeXFdoQueryDeviceUsageNotification(FdoExtension, Irp);
            break;
        }

        case IRP_MN_QUERY_INTERFACE:
        {
            Status = PciIdeXFdoQueryInterface(FdoExtension, IoStack);
            if (Status == STATUS_NOT_SUPPORTED)
                break;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FdoExtension->Ldo, Irp);
}
