/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA channel device object (FDO) dispatch routines
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

DECLARE_PAGED_WSTRING(AtapDevSymLinkFormat, L"\\Device\\ScsiPort%lu");
DECLARE_PAGED_WSTRING(AtapDosSymLinkFormat, L"\\DosDevices\\Scsi%lu:");

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
    DECLARE_PAGED_WSTRING(FdoFormat, L"\\Device\\Ide\\IdePort%lu");

    PAGED_CODE();

    if (ChanExt->PortData.PortFlags & PORT_FLAG_SYMLINK_CREATED)
        return STATUS_SUCCESS;

    Status = RtlStringCbPrintfW(ChannelNameBuffer,
                                sizeof(ChannelNameBuffer),
                                FdoFormat,
                                ChanExt->DeviceObjectNumber);
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
        if (!NT_SUCCESS(Status))
            continue;

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

        ChanExt->ScsiPortNumber = ScsiAdapter;
        ChanExt->PortData.PortFlags |= PORT_FLAG_SYMLINK_CREATED;
        break;
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoAllocateLocalBuffer(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS LowestAcceptableAddress;
    PHYSICAL_ADDRESS BoundryAddressMultiple;

    PAGED_CODE();

    if (PortData->LocalBuffer)
        return STATUS_SUCCESS;

    LowestAcceptableAddress.QuadPart = 0;
    HighestAcceptableAddress.QuadPart = 0xFFFFFFFF; // 32-bit DMA
    BoundryAddressMultiple.QuadPart = 0x10000; // 64k, for PATA compability
    PortData->LocalBuffer = MmAllocateContiguousMemorySpecifyCache(ATA_LOCAL_BUFFER_SIZE,
                                                                   LowestAcceptableAddress,
                                                                   HighestAcceptableAddress,
                                                                   BoundryAddressMultiple,
                                                                   MmNonCached);
    if (!PortData->LocalBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    PhysicalAddress = MmGetPhysicalAddress(PortData->LocalBuffer);

    PortData->LocalSgList.NumberOfElements = 1;
    PortData->LocalSgList.Elements[0].Length = ATA_LOCAL_BUFFER_SIZE;
    PortData->LocalSgList.Elements[0].Address.QuadPart = PhysicalAddress.QuadPart;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoCreatePortThread(
    _In_ PATAPORT_PORT_DATA PortData)
{
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, OBJ_KERNEL_HANDLE);
    HANDLE ThreadHandle;
    NTSTATUS Status;

    PAGED_CODE();

    if (PortData->Worker.Thread)
        return STATUS_SUCCESS;

    KeInitializeEvent(&PortData->Worker.ThreadEvent, NotificationEvent, FALSE);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  AtaPortWorkerThread,
                                  PortData);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_ALL_ACCESS,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&PortData->Worker.Thread,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    ZwClose(ThreadHandle);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoDestroyPortThread(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;

    PAGED_CODE();

    if (!PortData->Worker.Thread)
        return;

    PortData->PortFlags |= PORT_FLAG_EXIT_THREAD;
    AtaPortSignalWorkerThread(PortData);

    KeWaitForSingleObject(PortData->Worker.Thread, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(PortData->Worker.Thread);

    PortData->Worker.Thread = NULL;
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

    if (!(ChanExt->PortData.PortFlags & PORT_FLAG_SYMLINK_CREATED))
        return;

    /* Delete the '\DosDevices\\ScsiX:' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDevSymLinkFormat,
                                ChanExt->ScsiPortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Delete the '\Device\\ScsiPortX' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDosSymLinkFormat,
                                ChanExt->ScsiPortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Unregister the SCSI port adapter */
    --IoGetConfigurationInformation()->ScsiPortCount;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;
    PCIIDEX_CHANNEL_INTERFACE ChannelInterface = { 0 };
    NTSTATUS Status;

    PAGED_CODE();

    INFO("Starting channel %lu\n", ChanExt->DeviceObjectNumber);

    /* Get the interface of ATA channel */
    Status = AtaPnpQueryInterface(&ChanExt->Common,
                                  &GUID_PCIIDE_INTERFACE_ROS,
                                  &ChannelInterface,
                                  PCIIDEX_INTERFACE_VERSION,
                                  sizeof(ChannelInterface));
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to query channel interface %lx\n", Status);
        return Status;
    }
    PortData->ChannelContext = ChannelInterface.ChannelContext;
    PortData->AttachChannel = ChannelInterface.AttachChannel;
    PortData->SetDeviceData = ChannelInterface.SetDeviceData;
    PortData->GetInitTaskFile = ChannelInterface.GetInitTaskFile;
    PortData->DowngradeInterfaceSpeed = ChannelInterface.DowngradeInterfaceSpeed;
    PortData->InterruptObject = ChannelInterface.InterruptObject;
    PortData->AbortChannel = ChannelInterface.AbortChannel;
    PortData->ResetChannel = ChannelInterface.ResetChannel;
    PortData->EnumerateChannel = ChannelInterface.EnumerateChannel;
    PortData->IdentifyDevice = ChannelInterface.IdentifyDevice;
    PortData->SetTransferMode = ChannelInterface.SetTransferMode;
    PortData->AllocateSlot = ChannelInterface.AllocateSlot;
    PortData->PreparePrdTable = ChannelInterface.PreparePrdTable;
    PortData->PrepareIo = ChannelInterface.PrepareIo;
    PortData->StartIo = ChannelInterface.StartIo;
    PortData->MaxTargetId = ChannelInterface.MaxTargetId;
    PortData->MaximumTransferLength = ChannelInterface.MaximumTransferLength;
    PortData->MaximumPhysicalPages = ChannelInterface.MaximumPhysicalPages;
    PortData->QueueDepth = ChannelInterface.QueueDepth;
    PortData->PortNumber = ChannelInterface.Channel;
    PortData->DmaAdapter = ChannelInterface.DmaAdapter;
    PortData->ChannelObject = ChannelInterface.ChannelObject;

    PortData->InterruptFlags = PORT_INT_FLAG_IS_IO_ACTIVE;
    PortData->FreeSlotsBitmap =
    PortData->MaxSlotsBitmap = NUM_TO_BITMAP(PortData->QueueDepth);
    /* We need the slot numbers to start from zero */
    PortData->LastUsedSlot = RTL_BITS_OF(ULONG) - 1;

    if (!(ChannelInterface.TransferModeSupported & ~PIO_ALL))
        PortData->PortFlags |= PORT_FLAG_PIO_ONLY;

    if (ChannelInterface.Flags & ATA_CHANNEL_FLAG_PIO_VIA_DMA)
        PortData->PortFlags |= PORT_FLAG_PIO_VIA_DMA;

    if (ChannelInterface.Flags & ATA_CHANNEL_FLAG_NCQ)
        PortData->PortFlags |= PORT_FLAG_NCQ;

    if (ChannelInterface.Flags & ATA_CHANNEL_FLAG_IS_AHCI)
        PortData->PortFlags |= PORT_FLAG_IS_AHCI;

    if (ChannelInterface.Flags & ATA_CHANNEL_FLAG_IS_EXTERNAL)
        PortData->PortFlags |= PORT_FLAG_IS_EXTERNAL;

    if (ChannelInterface.Flags & ATA_CHANNEL_FLAG_PIO_FOR_LBA48_XFER)
        PortData->PortFlags |= PORT_FLAG_PIO_FOR_LBA48_XFER;

    if (ChannelInterface.HwSyncObject)
    {
        PortData->PortFlags |= PORT_FLAG_IS_SIMPLEX;
        PortData->HwSyncObject = ChannelInterface.HwSyncObject;
    }

    /* Reserve PIO memory resources early. Storage drivers should not fail paging I/O operations */
    if (!(ChannelInterface.Flags & ATA_CHANNEL_FLAG_PIO_VIA_DMA) && !PortData->ReservedVaSpace)
    {
        PortData->ReservedVaSpace = MmAllocateMappingAddress(ATA_RESERVED_PAGES * PAGE_SIZE,
                                                             ATAPORT_TAG);
    }

    Status = AtaFdoAllocateLocalBuffer(PortData);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to allocate local buffer 0x%lx\n", PortData->PortNumber, Status);
        return Status;
    }

    InitializeListHead(&PortData->PortQueueList);
    KeInitializeEvent(&PortData->QueueStoppedEvent, NotificationEvent, FALSE);

    KeInitializeDpc(&PortData->Worker.Dpc, AtaPortWorkerSignalDpc, PortData);
    KeInitializeDpc(&PortData->Worker.NotificationDpc, AtaStorageNotificationlDpc, PortData);
    KeInitializeSpinLock(&PortData->Worker.Lock);
    KeInitializeEvent(&PortData->Worker.EnumerationEvent, NotificationEvent, FALSE);

    KeInitializeEvent(&PortData->Worker.CompletionEvent, NotificationEvent, FALSE);
    PortData->Worker.InternalRequest.Complete = AtaPortCompleteInternalRequest;
#if DBG
    PortData->Worker.InternalRequest.Signature = ATA_DEVICE_REQUEST_SIGNATURE;
#endif

    Status = AtaFdoCreatePortThread(PortData);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to create port thread 0x%lx\n", PortData->PortNumber, Status);
        return Status;
    }

    Status = AtaFdoCreateSymLinks(ChanExt);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to create symbolic links 0x%lx\n", PortData->PortNumber, Status);
        return Status;
    }

    AtaSetPortRegistryKey(ChanExt, DD_ATA_REG_MAX_TARGET_ID, PortData->MaxTargetId);

    Status = IoRegisterDeviceInterface(ChanExt->Common.Self,
                                       &GUID_DEVINTERFACE_STORAGEPORT,
                                       NULL,
                                       &ChanExt->StorageInterfaceName);
    if (NT_SUCCESS(Status))
    {
        INFO("InterfaceName: '%wZ'\n", &ChanExt->StorageInterfaceName);

        Status = IoSetDeviceInterfaceState(&ChanExt->StorageInterfaceName, TRUE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString(&ChanExt->StorageInterfaceName);
            ChanExt->StorageInterfaceName.Buffer = NULL;
        }
    }

    *ChannelInterface.PortContext = PortData;
    *ChannelInterface.PortNotification = AtaPortNotification;
    *ChannelInterface.Slots = PortData->Slots;

    Status = IoInitializeTimer(ChanExt->Common.Self, AtaPortIoTimer, PortData);
    if (!NT_SUCCESS(Status))
        return Status;

    IoStartTimer(ChanExt->Common.Self);
    PortData->PortFlags |= PORT_FLAG_IO_TIMER_ACTIVE;

    Status = PortData->AttachChannel(PortData->ChannelContext, TRUE);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to attach channel %lx\n", PortData->PortNumber, Status);
        return Status;
    }
    PortData->PortFlags |= PORT_FLAG_CHANNEL_ATTACHED;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoDetachChannel(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;

    PAGED_CODE();

    if (PortData->AttachChannel)
    {
        PortData->AttachChannel(PortData->ChannelContext, FALSE);
        PortData->AttachChannel = NULL;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoStopDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    AtaFdoDetachChannel(ChanExt);

    if (ChanExt->StorageInterfaceName.Buffer)
        IoSetDeviceInterfaceState(&ChanExt->StorageInterfaceName, FALSE);

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

    if (ChanExt->StorageInterfaceName.Buffer)
    {
        IoSetDeviceInterfaceState(&ChanExt->StorageInterfaceName, FALSE);

        RtlFreeUnicodeString(&ChanExt->StorageInterfaceName);
        ChanExt->StorageInterfaceName.Buffer = NULL;
    }

    AtaFdoDestroyPortThread(ChanExt);

    if (ChanExt->PortData.PortFlags & PORT_FLAG_IO_TIMER_ACTIVE)
    {
        IoStopTimer(ChanExt->Common.Self);
        ChanExt->PortData.PortFlags &= ~PORT_FLAG_IO_TIMER_ACTIVE;
    }

    if (ChanExt->PortData.PortFlags & PORT_FLAG_CHANNEL_ATTACHED)
    {
        AtaFdoDetachChannel(ChanExt);
        ChanExt->PortData.PortFlags &= ~PORT_FLAG_CHANNEL_ATTACHED;
    }

    if (FinalRemove)
    {
        ATA_SCSI_ADDRESS AtaScsiAddress;

        IoReleaseRemoveLockAndWait(&ChanExt->Common.RemoveLock, Irp);

        AtaScsiAddress.AsULONG = 0;
        while (TRUE)
        {
            PATAPORT_DEVICE_EXTENSION DevExt;

            DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
            if (!DevExt)
                break;

            AtaReqFlushDeviceQueue(&DevExt->Device);
            AtaPdoFreeDevice(DevExt);
        }

        AtaFdoRemoveSymLinks(ChanExt);

        if (ChanExt->PortData.LocalBuffer)
        {
            MmFreeContiguousMemorySpecifyCache(ChanExt->PortData.LocalBuffer,
                                               ATA_LOCAL_BUFFER_SIZE,
                                               MmNonCached);
            ChanExt->PortData.LocalBuffer = NULL;
        }

        if (ChanExt->PortData.ReservedVaSpace)
        {
            MmFreeMappingAddress(ChanExt->PortData.ReservedVaSpace, ATAPORT_TAG);
            ChanExt->PortData.ReservedVaSpace = NULL;
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);

        IoDetachDevice(ChanExt->Common.LowerDeviceObject);
        IoDeleteDevice(ChanExt->Common.Self);
    }
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);

        IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    }

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

    INFO("(%p, %p) Ch.%lu %s\n",
         ChanExt->Common.Self,
         Irp,
         ChanExt->DeviceObjectNumber,
         GetIRPMinorFunctionString(IoGetCurrentIrpStackLocation(Irp)->MinorFunction));

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
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Common.LowerDeviceObject, Irp)))
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
            Status = AtaPnpQueryPnpDeviceState(&ChanExt->Common, Irp);
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
            Status = AtaPnpQueryDeviceUsageNotification(&ChanExt->Common, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ PVOID ReferenceTag)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldLevel;
    NTSTATUS Status;

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next; Entry != NULL; Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DevExt->Device.AtaScsiAddress.AsULONG != AtaScsiAddress.AsULONG)
            continue;

        if (DevExt->ReportedMissing || DevExt->RemovalPending)
            continue;

        if (ReferenceTag)
        {
            Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, ReferenceTag);
            if (!NT_SUCCESS(Status))
                break;
        }

        Result = DevExt;
        break;
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);

    return Result;
}

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindNextDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ BOOLEAN SearchRemoveDev,
    _In_ PVOID ReferenceTag)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldLevel;
    NTSTATUS Status;

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next; Entry != NULL; Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DevExt->Device.AtaScsiAddress.AsULONG <= AtaScsiAddress->AsULONG)
            continue;

        if (DevExt->ReportedMissing)
            continue;

        if (!SearchRemoveDev && DevExt->RemovalPending)
            continue;

        *AtaScsiAddress = DevExt->Device.AtaScsiAddress;

        if (ReferenceTag)
        {
            Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, ReferenceTag);
            if (!NT_SUCCESS(Status))
                continue;
        }
        Result = DevExt;
        break;
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);

    return Result;
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN DoInsert)
{
    PSINGLE_LIST_ENTRY Entry, PrevEntry;
    KIRQL OldLevel;
    ULONG Address = DevExt->Device.AtaScsiAddress.AsULONG;

    PAGED_CODE();

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        PATAPORT_DEVICE_EXTENSION CurrentDevExt;

        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DoInsert)
        {
            if (CurrentDevExt->Device.AtaScsiAddress.AsULONG > Address)
                break;
        }
        else
        {
            if (CurrentDevExt->Device.AtaScsiAddress.AsULONG == Address)
                break;
        }

        PrevEntry = Entry;
    }

    /* The device list is ordered by SCSI address (Path:Target:Lun), smallest first */
    if (PrevEntry)
    {
        /* Before the current entry */
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = DoInsert ? &DevExt->ListEntry : DevExt->ListEntry.Next;
    }
    else
    {
        /* In the beginning */
        if (DoInsert)
            PushEntryList(&ChanExt->PdoList, &DevExt->ListEntry);
        else
            PopEntryList(&DevExt->ListEntry);
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);
}
