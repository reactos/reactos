/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA channel device object (FDO) dispatch routines
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const WCHAR AtapDevSymLinkFormat[] = L"\\Device\\ScsiPort%lu";
static const WCHAR AtapDosSymLinkFormat[] = L"\\DosDevices\\Scsi%lu:";

static KSYNCHRONIZE_ROUTINE AtaFdoEnableInterruptsSync;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoCreateSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    UNICODE_STRING ChannelName;
    ULONG ScsiAdapter, ScsiPortCount;
    WCHAR ChannelNameBuffer[sizeof("\\Device\\Ide\\IdePort999")];
    NTSTATUS Status;
    static const WCHAR IdeChannelFormat[] = L"\\Device\\Ide\\IdePort%lu";

    PAGED_CODE();

    Status = RtlStringCbPrintfW(ChannelNameBuffer,
                                sizeof(ChannelNameBuffer),
                                IdeChannelFormat,
                                ChanExt->ChannelNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&ChannelName, ChannelNameBuffer);

    ScsiPortCount = IoGetConfigurationInformation()->ScsiPortCount;

    /* Search for a free SCSI port adapter in the system */
    for (ScsiAdapter = 0; ScsiAdapter <= ScsiPortCount; ++ScsiAdapter)
    {
        WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];
        UNICODE_STRING SymLinkName;

        Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                    sizeof(SymLinkNameBuffer),
                                    AtapDevSymLinkFormat,
                                    ScsiAdapter);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

        /* Create a symbolic link '\Device\ScsiPortX' -> '\Device\Ide\IdePortN' */
        Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
        if (NT_SUCCESS(Status))
        {
            INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);

            Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                        sizeof(SymLinkNameBuffer),
                                        AtapDosSymLinkFormat,
                                        ScsiAdapter);
            ASSERT(NT_SUCCESS(Status));
            RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

            /* Create a symbolic link '\DosDevices\ScsiX:' -> '\Device\Ide\IdePortN' */
            Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
            if (NT_SUCCESS(Status))
            {
                INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);
            }

            /* Register ourselves (ATA channel) as a SCSI port adapter */
            ++IoGetConfigurationInformation()->ScsiPortCount;

            ChanExt->PortNumber = ScsiAdapter;
            ChanExt->Flags |= CHANNEL_SYMLINK_CREATED;
            break;
        }
    }

    return Status;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoRemoveSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    NTSTATUS Status;
    UNICODE_STRING SymLinkName;
    WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];

    PAGED_CODE();

    /* Delete the '\DosDevices\\ScsiX:' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDevSymLinkFormat,
                                ChanExt->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Delete the '\Device\\ScsiPortX' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDosSymLinkFormat,
                                ChanExt->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Unregister the SCSI port adapter */
    --IoGetConfigurationInformation()->ScsiPortCount;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoParseAhciResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR AbarDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypeMemory:
            {
                /* We save the last memory descriptor as ABAR */
                AbarDescriptor = Descriptor;
                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

    if (!AbarDescriptor || !InterruptDescriptor)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    ChanExt->IoBase = MmMapIoSpace(AbarDescriptor->u.Memory.Start,
                                            AbarDescriptor->u.Memory.Length,
                                            MmNonCached);
    if (!ChanExt->IoBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    ChanExt->IoLength = AbarDescriptor->u.Memory.Length;

    /* Interrupt */
    ChanExt->InterruptVector = InterruptDescriptor->u.Interrupt.Vector;
    ChanExt->InterruptLevel = InterruptDescriptor->u.Interrupt.Level;
    ChanExt->InterruptAffinity = InterruptDescriptor->u.Interrupt.Affinity;

    if (InterruptDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
        ChanExt->InterruptMode = Latched;
    else
        ChanExt->InterruptMode = LevelSensitive;

    if (InterruptDescriptor->ShareDisposition == CmResourceShareShared)
        ChanExt->Flags |= CHANNEL_INTERRUPT_SHARED;
    else
        ChanExt->Flags &= ~CHANNEL_INTERRUPT_SHARED;

    return STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
AtaFdoEnableInterruptsSync(
    _In_ PVOID SynchronizeContext)
{
    PATA_ENABLE_INTERRUPTS_CONTEXT Context = SynchronizeContext;
    PATAPORT_CHANNEL_EXTENSION ChanExt = Context->ChanExt;
    ULONG GlobalControl;

    GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);

    if (Context->Enable)
        GlobalControl |= AHCI_GHC_IE;
    else
        GlobalControl &= ~AHCI_GHC_IE;

    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoEnableInterrupts(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ BOOLEAN Enable)
{
    ATA_ENABLE_INTERRUPTS_CONTEXT SynchronizeContext;

    PAGED_CODE();

    SynchronizeContext.ChanExt = ChanExt;
    SynchronizeContext.Enable = Enable;

    KeSynchronizeExecution(ChanExt->InterruptObject,
                           AtaFdoEnableInterruptsSync,
                           &SynchronizeContext);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoFreeIoResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PAGED_CODE();

    if (ChanExt->InterruptObject)
    {
        AtaFdoEnableInterrupts(ChanExt, FALSE);

        IoDisconnectInterrupt(ChanExt->InterruptObject);
        ChanExt->InterruptObject = NULL;
    }

    if (ChanExt->IoBase)
    {
        MmUnmapIoSpace(ChanExt->IoBase, ChanExt->IoLength);
        ChanExt->IoBase = NULL;
    }
}

static
CODE_SEG("PAGE")
VOID
AtaFdoInitializeMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    ATA_SRB_TYPE i;

    PAGED_CODE();

    for (i = 0; i < SRB_TYPE_MAX; ++i)
    {
        Srb = &ChanExt->InternalSrb[i];
        Irp = &ChanExt->InternalIrp[i];

        Srb->OriginalRequest = Irp;
    }
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    NTSTATUS Status;

    PAGED_CODE();

    INFO("Starting channel %lu\n", ChanExt->ChannelNumber);

    ChanExt->MaximumTransferLength = ATA_MAX_TRANSFER_LENGTH;

    Status = AtaFdoParseAhciResources(ChanExt, ResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to parse resources 0x%lx\n", Status);
        return Status;
    }

    AtaFdoInitializeMemory(ChanExt);

    Status = AtaFdoAhciInit(ChanExt);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to init the HBA 0x%lx\n", Status);
        return Status;
    }

    if (!(ChanExt->Flags & CHANNEL_SYMLINK_CREATED))
    {
        Status = AtaFdoCreateSymLinks(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create symbolic links 0x%lx\n", Status);
            return Status;
        }
    }

    Status = IoRegisterDeviceInterface(ChanExt->Common.Self,
                                       &GUID_DEVINTERFACE_STORAGEPORT,
                                       NULL,
                                       &ChanExt->InterfaceName);
    if (NT_SUCCESS(Status))
    {
        INFO("InterfaceName: '%wZ'\n", &ChanExt->InterfaceName);

        Status = IoSetDeviceInterfaceState(&ChanExt->InterfaceName, TRUE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString(&ChanExt->InterfaceName);
            ChanExt->InterfaceName.Buffer = NULL;
        }
    }

    /* Reserve memory resources early. Storage drivers should not fail paging I/O operations */
    if (!ChanExt->ReservedVaSpace)
    {
        ChanExt->ReservedVaSpace =
            MmAllocateMappingAddress(ATA_RESERVED_PAGES * PAGE_SIZE, ATAPORT_TAG);
    }

    Status = IoConnectInterrupt(&ChanExt->InterruptObject,
                                AtaHbaIsr,
                                ChanExt,
                                NULL,
                                ChanExt->InterruptVector,
                                ChanExt->InterruptLevel,
                                ChanExt->InterruptLevel,
                                ChanExt->InterruptMode,
                                !!(ChanExt->Flags & CHANNEL_INTERRUPT_SHARED),
                                ChanExt->InterruptAffinity,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not connect to interrupt %lu, status 0x%lx\n",
            ChanExt->InterruptVector, Status);

        return Status;
    }

    AtaFdoEnableInterrupts(ChanExt, TRUE);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoStopDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    AtaFdoFreeIoResources(ChanExt);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove)
{
    NTSTATUS Status;

    PAGED_CODE();

    AtaFdoFreeIoResources(ChanExt);
    AtaFdoFreePortMemory(ChanExt);

    if (ChanExt->Flags & CHANNEL_SYMLINK_CREATED)
    {
        ChanExt->Flags &= ~CHANNEL_SYMLINK_CREATED;

        AtaFdoRemoveSymLinks(ChanExt);
    }

    if (ChanExt->InterfaceName.Buffer)
    {
        IoSetDeviceInterfaceState(&ChanExt->InterfaceName, FALSE);

        RtlFreeUnicodeString(&ChanExt->InterfaceName);
        ChanExt->InterfaceName.Buffer = NULL;
    }

    if (ChanExt->ReservedVaSpace)
    {
        MmFreeMappingAddress(ChanExt->ReservedVaSpace, ATAPORT_TAG);
        ChanExt->ReservedVaSpace = NULL;
    }

    if (ChanExt->AdapterObject)
    {
        PDMA_OPERATIONS DmaOperations = ChanExt->AdapterObject->DmaOperations;

        DmaOperations->PutDmaAdapter(ChanExt->AdapterObject);
        ChanExt->AdapterObject = NULL;
    }

    if (!Irp)
        return STATUS_SUCCESS;

    if (FinalRemove)
    {
        IoReleaseRemoveLockAndWait(&ChanExt->Common.RemoveLock, Irp);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Ldo, Irp);

        IoDetachDevice(ChanExt->Ldo);
        IoDeleteDevice(ChanExt->Common.Self);
    }
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Ldo, Irp);

        IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryPnpDeviceState(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (ChanExt->Common.PageFiles ||
        ChanExt->Common.HibernateFiles ||
        ChanExt->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryDeviceUsageNotification(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
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
            Counter = &ChanExt->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &ChanExt->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &ChanExt->Common.DumpFiles;
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
AtaFdoQueryId(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;

    PAGED_CODE();

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            Status = Irp->IoStatus.Status;

            /*
             * We provide a generic identifier necessary to install the device
             * in case our FDO is root-enumerated device.
             */
            if (Status == STATUS_NOT_SUPPORTED)
            {
                static const WCHAR IdeGenericId[] = L"*PNP0600\0";
                PWCHAR Buffer;

                Buffer = ExAllocatePoolUninitialized(PagedPool, sizeof(IdeGenericId), ATAPORT_TAG);
                if (!Buffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory(Buffer, IdeGenericId, sizeof(IdeGenericId));

                Irp->IoStatus.Information = (ULONG_PTR)Buffer;
                Status = STATUS_SUCCESS;
            }

            break;
        }

        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(ChanExt->Ldo, Irp);

            IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

            return Status;
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    Status = IoAcquireRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto CompleteIrp;
            }
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Status = AtaFdoStartDevice(ChanExt,
                                       IoStack->Parameters.
                                       StartDevice.AllocatedResourcesTranslated);
            goto CompleteIrp;
        }

        case IRP_MN_STOP_DEVICE:
            Status = AtaFdoStopDevice(ChanExt, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            return AtaFdoRemoveDevice(ChanExt,
                                      Irp,
                                       (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE));

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = AtaFdoQueryPnpDeviceState(ChanExt, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = AtaFdoQueryBusRelations(ChanExt, Irp);
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaFdoQueryDeviceUsageNotification(ChanExt, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            return AtaFdoQueryId(ChanExt, Irp, IoStack);

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(ChanExt->Ldo, Irp);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = ChanExt->DeviceList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if ((DevExt->AtaScsiAddress.AsULONG == AtaScsiAddress.AsULONG) &&
            !DevExt->ReportedMissing)
        {
            Result = DevExt;
            break;
        }
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);

    return Result;
}

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListRemove(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PSINGLE_LIST_ENTRY Entry, PrevEntry;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = ChanExt->DeviceList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        PATAPORT_DEVICE_EXTENSION CurrentDevExt;

        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (CurrentDevExt->AtaScsiAddress.AsULONG == DevExt->AtaScsiAddress.AsULONG)
            break;

        PrevEntry = Entry;
    }

    if (PrevEntry)
    {
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = DevExt->ListEntry.Next;
    }
    else
    {
        PopEntryList(&DevExt->ListEntry);
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);
}

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_DEVICE_EXTENSION CurrentDevExt;
    PSINGLE_LIST_ENTRY Entry, PrevEntry;
    ATA_SCSI_ADDRESS Address = DevExt->AtaScsiAddress;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = ChanExt->DeviceList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (CurrentDevExt->AtaScsiAddress.AsULONG > Address.AsULONG)
            break;

        PrevEntry = Entry;
    }

    /* The device list is ordered by SCSI address, smallest first */
    if (PrevEntry)
    {
        /* Insert before the current entry */
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = &DevExt->ListEntry;
    }
    else
    {
        /* Insert in the beginning */
        PushEntryList(&ChanExt->DeviceList, &DevExt->ListEntry);
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);
}
