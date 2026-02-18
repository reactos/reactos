/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Child device object (PDO) dispatch routines
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoStartDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (!(DevExt->Device.DeviceFlags & DEVICE_PNP_STARTED))
    {
        PATAPORT_PORT_DATA PortData = DevExt->Device.PortData;

        AtaPdoWmiRegistration(DevExt, TRUE);

        /* Update the type of device connected to the port */
        PortData->SetDeviceData(PortData->ChannelContext,
                                DevExt->Common.Self,
                                &DevExt->IdentifyDeviceData);

        /* Get the ATA initialization commands to restore the boot up defaults */
        if (!DevExt->GtfDataBuffer)
        {
            DevExt->GtfDataBuffer = PortData->GetInitTaskFile(PortData->ChannelContext,
                                                              DevExt->Common.Self);
        }

        /* Use the standard power policy for mass storage devices */
        DevExt->Device.PowerIdleCounter = PoRegisterDeviceForIdleDetection(DevExt->Common.Self,
                                                                           (ULONG)-1,
                                                                           (ULONG)-1,
                                                                           PowerDeviceD3);

        /* Set the new ATA volatile settings */
        KeClearEvent(&DevExt->Worker.ConfigureEvent);
        AtaDeviceQueueEvent(DevExt->Device.PortData, DevExt, ACTION_DEVICE_CONFIG);
        KeWaitForSingleObject(&DevExt->Worker.ConfigureEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        DevExt->Device.DeviceFlags |= DEVICE_PNP_STARTED;
    }

    AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_PNP);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoStopDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PNP);
    AtaReqWaitForOutstandingIoToComplete(&DevExt->Device, NULL);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoRemoveDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN FinalRemove)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;

    PAGED_CODE();

    AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_REMOVED);
    AtaReqFlushDeviceQueue(&DevExt->Device);
    AtaPdoWmiRegistration(DevExt, FALSE);

    if (FinalRemove && DevExt->ReportedMissing)
    {
        IoReleaseRemoveLockAndWait(&DevExt->Common.RemoveLock, Irp);

        AtaFdoDeviceListInsert(ChanExt, DevExt, FALSE);
        AtaPdoFreeDevice(DevExt);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStopRemoveDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (DevExt->Common.PageFiles ||
        DevExt->Common.HibernateFiles ||
        DevExt->Common.DumpFiles)
    {
        return STATUS_DEVICE_BUSY;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AtaPdoCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent(Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryCapabilities(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    DEVICE_CAPABILITIES ParentCapabilities;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS Status;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    /* Get the capabilities of the parent device */
    RtlZeroMemory(&ParentCapabilities, sizeof(ParentCapabilities));
    ParentCapabilities.Size = sizeof(ParentCapabilities);
    ParentCapabilities.Version = 1;
    ParentCapabilities.Address = MAXULONG;
    ParentCapabilities.UINumber = MAXULONG;
    Status = AtaPnpRepeatRequest(&DevExt->Common, Irp, &ParentCapabilities);
    if (!NT_SUCCESS(Status))
        return Status;

    DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;
    RtlCopyMemory(DeviceCapabilities, &ParentCapabilities, sizeof(*DeviceCapabilities));

    AtaScsiAddress = DevExt->Device.AtaScsiAddress;

    /* Override some fields */
    DeviceCapabilities->UINumber = AtaScsiAddress.TargetId;
    DeviceCapabilities->UniqueID = FALSE;
    DeviceCapabilities->SurpriseRemovalOK = FALSE;
    DeviceCapabilities->Removable = !!(DevExt->Device.DeviceFlags & DEVICE_IS_PDO_REMOVABLE);
    DeviceCapabilities->D1Latency =
    DeviceCapabilities->D2Latency =
    DeviceCapabilities->D3Latency = 31 * 10000; // 31 seconds (legacy ATA timeout)
    /*
     * See ACPI specification, _ADR (Address).
     * The goal of the LUN field here is to hide devices with LUN>0 from the ACPI driver,
     * since these devices are not mentioned in the ACPI spec at all.
     */
    DeviceCapabilities->Address = (AtaScsiAddress.Lun << 4) | AtaScsiAddress.TargetId;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryTargetDeviceRelations(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;

    PAGED_CODE();

    DeviceRelations = ExAllocatePoolUninitialized(PagedPool,
                                                  sizeof(*DeviceRelations),
                                                  ATAPORT_TAG);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DevExt->Common.Self;
    ObReferenceObject(DevExt->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryId(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    CHAR LocalBuffer[ATAPORT_FN_FIELD + ATAPORT_RN_FIELD];
    NTSTATUS Status;
    PWCHAR Buffer, End;
    PCSTR DeviceType, GenericType;
    size_t CharCount, Remaining;

    PAGED_CODE();

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
        {
            DeviceType = AtaTypeCodeToName(DevExt, GetDeviceType);

            /* 'IDE\CdRomVBOX_CD-ROM_____________________________1.0_____' */
            CharCount = (sizeof("IDE\\") - 1) + strlen(DeviceType) +
                        ATAPORT_FN_FIELD + ATAPORT_RN_FIELD +
                        sizeof(ANSI_NULL);

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaCopyIdStringSafe(LocalBuffer,
                                (PUCHAR)DevExt->FriendlyName,
                                ATAPORT_FN_FIELD,
                                '_');

            AtaCopyIdStringSafe(&LocalBuffer[ATAPORT_FN_FIELD],
                                (PUCHAR)DevExt->RevisionNumber,
                                ATAPORT_RN_FIELD,
                                '_');

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"IDE\\%hs%.48hs",
                                           DeviceType,
                                           LocalBuffer);
            ASSERT(NT_SUCCESS(Status));

            INFO("DeviceID: '%S'\n", Buffer);
            break;
        }

        case BusQueryHardwareIDs:
        {
            PWCHAR IdStart;

            DBG_UNREFERENCED_LOCAL_VARIABLE(IdStart);

            DeviceType = AtaTypeCodeToName(DevExt, GetDeviceType);
            GenericType = AtaTypeCodeToName(DevExt, GetGenericType);

            /*
             *               |------------------ 40 -----------------|-- 8 --|
             *               v                                       v       v
             *  1) 'IDE\CdRomVBOX_CD-ROM_____________________________1.0_____'
             *  2) 'IDE\VBOX_CD-ROM_____________________________1.0_____'
             *  3) 'IDE\CdRomVBOX_CD-ROM_____________________________'
             *  4) 'VBOX_CD-ROM_____________________________1.0_____'
             *  5) 'GenCdRom'
             */
            CharCount = strlen(DeviceType) * 2 +
                        strlen(GenericType) +
                        (sizeof("IDE\\") - 1) * 3 +
                        ATAPORT_FN_FIELD * 4 +
                        ATAPORT_RN_FIELD * 3 +
                        5 * sizeof(ANSI_NULL) +
                        sizeof(ANSI_NULL); /* multi-string */

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaCopyIdStringSafe(LocalBuffer,
                                (PUCHAR)DevExt->FriendlyName,
                                ATAPORT_FN_FIELD,
                                '_');

            AtaCopyIdStringSafe(&LocalBuffer[ATAPORT_FN_FIELD],
                                (PUCHAR)DevExt->RevisionNumber,
                                ATAPORT_RN_FIELD,
                                '_');

            INFO("HardwareIDs:\n");

            /* ID 1 */
            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"IDE\\%hs%.48hs",
                                           DeviceType,
                                           LocalBuffer);
            ASSERT(NT_SUCCESS(Status));

            INFO("  '%S'\n", Buffer);

            ++End;
            --Remaining;

            /* ID 2 */
            IdStart = End;
            Status = RtlStringCchPrintfExW(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"IDE\\%.48hs",
                                           LocalBuffer);
            ASSERT(NT_SUCCESS(Status));

            INFO("  '%S'\n", IdStart);

            ++End;
            --Remaining;

            /* ID 3 */
            IdStart = End;
            Status = RtlStringCchPrintfExW(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"IDE\\%hs%.40hs",
                                           DeviceType,
                                           LocalBuffer);
            ASSERT(NT_SUCCESS(Status));

            INFO("  '%S'\n", IdStart);

            ++End;
            --Remaining;

            /* ID 4 */
            IdStart = End;
            Status = RtlStringCchPrintfExW(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"%.48hs",
                                           LocalBuffer);
            ASSERT(NT_SUCCESS(Status));

            INFO("  '%S'\n", IdStart);

            ++End;
            --Remaining;

            /* ID 5 */
            IdStart = End;
            Status = RtlStringCchPrintfExW(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"%hs",
                                           GenericType);
            ASSERT(NT_SUCCESS(Status));

            *++End = UNICODE_NULL; /* multi-string */

            INFO("  '%S'\n", IdStart);
            break;
        }

        case BusQueryCompatibleIDs:
        {
            GenericType = AtaTypeCodeToName(DevExt, GetGenericType);

            /* 'GenCdRom' */
            CharCount = strlen(GenericType) + 2 * sizeof(ANSI_NULL); /* multi-string */

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"%hs",
                                           GenericType);
            ASSERT(NT_SUCCESS(Status));

            *++End = UNICODE_NULL; /* multi-string */

            INFO("CompatibleIDs: '%S'\n", Buffer);
            break;
        }

        case BusQueryInstanceID:
        {
            ATA_SCSI_ADDRESS AtaScsiAddress;

            /* 'Path.Larget.Lun' */
            CharCount = sizeof("FF.FF.FF");

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaScsiAddress = DevExt->Device.AtaScsiAddress;
            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"%x.%x.%x",
                                           AtaScsiAddress.PathId,
                                           AtaScsiAddress.TargetId,
                                           AtaScsiAddress.Lun);
            ASSERT(NT_SUCCESS(Status));

            INFO("InstanceID: '%S'\n", Buffer);
            break;
        }

        default:
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryDeviceText(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    PWCHAR Buffer;
    size_t CharCount;

    PAGED_CODE();

    switch (IoStack->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextDescription:
        {
            CharCount = strlen(DevExt->FriendlyName) + sizeof(ANSI_NULL);

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"%hs",
                                           DevExt->FriendlyName);
            ASSERT(NT_SUCCESS(Status));

            INFO("TextDescription: '%S'\n", Buffer);
            break;
        }

        case DeviceTextLocationInformation:
        {
            ATA_SCSI_ADDRESS AtaScsiAddress;

            CharCount = sizeof("Channel 255, Target 255, Lun 255");

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaScsiAddress = DevExt->Device.AtaScsiAddress;
            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"Channel %u, Target %u, Lun %u",
                                           AtaScsiAddress.PathId,
                                           AtaScsiAddress.TargetId,
                                           AtaScsiAddress.Lun);
            ASSERT(NT_SUCCESS(Status));

            INFO("TextLocationInformation: '%S'\n", Buffer);
            break;
        }

        default:
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoPnp(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    INFO("(%p, %p) Tid.%lu %s\n",
         DevExt->Common.Self,
         Irp,
         DevExt->Device.AtaScsiAddress.TargetId,
         GetIRPMinorFunctionString(IoGetCurrentIrpStackLocation(Irp)->MinorFunction));

    Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, Irp);
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
            Status = AtaPdoStartDevice(DevExt, Irp);
            break;

        case IRP_MN_STOP_DEVICE:
            Status = AtaPdoStopDevice(DevExt, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Status = AtaPdoRemoveDevice(DevExt,
                                        Irp,
                                        (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE));
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = AtaPdoQueryStopRemoveDevice(DevExt, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = AtaPdoQueryCapabilities(DevExt, Irp, IoStack);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = AtaPnpQueryPnpDeviceState(&DevExt->Common, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = AtaPdoQueryId(DevExt, Irp, IoStack);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            Status = AtaPdoQueryDeviceText(DevExt, Irp, IoStack);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaPnpQueryDeviceUsageNotification(&DevExt->Common, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
                Status = AtaPdoQueryTargetDeviceRelations(DevExt, Irp);
            else
                Status = Irp->IoStatus.Status;
            break;
        }

        default:
            Status = Irp->IoStatus.Status;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    if (IS_FDO(DeviceObject->DeviceExtension))
        return AtaFdoPnp(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoPnp(DeviceObject->DeviceExtension, Irp);
}

VOID
CODE_SEG("PAGE")
AtaPdoFreeDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    if (DevExt->Device.PowerIdleCounter)
    {
        PoRegisterDeviceForIdleDetection(DevExt->Common.Self,
                                         0,
                                         0,
                                         PowerDeviceD3);
    }

    if (DevExt->GtfDataBuffer)
    {
        ExFreePoolWithTag(DevExt->GtfDataBuffer, ATAPORT_TAG);
        DevExt->GtfDataBuffer = NULL;
    }

    if (DevExt->Device.Requests)
        ExFreePoolWithTag(DevExt->Device.Requests, ATAPORT_TAG);

    IoDeleteDevice(DevExt->Common.Self);
}

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaPdoCreateDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PATAPORT_DEVICE_EXTENSION DevExt;
    PDEVICE_OBJECT Pdo;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\IdeDeviceP999T9L9-FFF")];
    ULONG i;
    DECLARE_PAGED_WSTRING(PdoFormat, L"\\Device\\Ide\\IdeDeviceP%luT%luL%lu-%lx");
    static ULONG AtapPdoNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                PdoFormat,
                                ChanExt->DeviceObjectNumber,
                                AtaScsiAddress.TargetId,
                                AtaScsiAddress.Lun,
                                AtapPdoNumber++);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(ChanExt->Common.Self->DriverObject,
                            sizeof(*DevExt),
                            &DeviceName,
                            FILE_DEVICE_MASS_STORAGE,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create PDO '%wZ' with status 0x%lx\n", &DeviceName, Status);
        return NULL;
    }

    INFO("Created device object %p '%wZ'\n", Pdo, &DeviceName);

    /* DMA buffers alignment */
    Pdo->AlignmentRequirement = ChanExt->Common.Self->AlignmentRequirement;
    Pdo->AlignmentRequirement = max(Pdo->AlignmentRequirement, ATA_MIN_BUFFER_ALIGNMENT);

    Pdo->Flags |= DO_DIRECT_IO;

    DevExt = Pdo->DeviceExtension;

    RtlZeroMemory(DevExt, sizeof(*DevExt));
    AtaPnpInitializeCommonExtension(&DevExt->Common, Pdo, ChanExt->Common.Flags & ~DO_IS_FDO);
    DevExt->Common.FdoExt = ChanExt;
    DevExt->TransferModeAllowedMask = MAXULONG;
    DevExt->Device.SectorSize = ATA_MIN_SECTOR_SIZE;
    DevExt->Device.PortData = PortData;
    DevExt->Device.LocalBuffer = PortData->LocalBuffer;

    DevExt->Device.AtaScsiAddress = AtaScsiAddress;
    DevExt->Device.TransportFlags = AtaScsiAddress.TargetId;
    DevExt->Device.DeviceSelect = IDE_DRIVE_SELECT | AtaScsiAddress.Lun;

    if (!(PortData->PortFlags & PORT_FLAG_IS_AHCI))
    {
        /* Master/Slave select bit */
        DevExt->Device.DeviceSelect |= ((AtaScsiAddress.TargetId & 1) << 4);
    }

    DevExt->Device.DeviceFlags = DEVICE_UNINITIALIZED;

    if (PortData->PortFlags & PORT_FLAG_IS_EXTERNAL)
        DevExt->Device.DeviceFlags |= DEVICE_IS_PDO_REMOVABLE;

    if (PortData->PortFlags & PORT_FLAG_PIO_VIA_DMA)
        DevExt->Device.DeviceFlags |= DEVICE_PIO_VIA_DMA;

    if (PortData->PortFlags & PORT_FLAG_PIO_FOR_LBA48_XFER)
        DevExt->Device.DeviceFlags |= DEVICE_PIO_FOR_LBA48_XFER;

    /* Device's capability not explored, yet */
    DevExt->Device.DeviceFlags |= DEVICE_PIO_ONLY;

    DevExt->Device.FreeRequestsBitmap =
    DevExt->Device.MaxRequestsBitmap = NUM_TO_BITMAP(PortData->QueueDepth);

    DevExt->Device.Requests = ExAllocatePoolZero(NonPagedPool,
                                                 sizeof(ATA_DEVICE_REQUEST) * PortData->QueueDepth,
                                                 ATAPORT_TAG);
    if (!DevExt->Device.Requests)
        goto Failure;

    for (i = 0; i < PortData->QueueDepth; ++i)
    {
        PATA_DEVICE_REQUEST Request = &DevExt->Device.Requests[i];

        Request->Device = (PATA_IO_CONTEXT_COMMON)&DevExt->Device;
#if DBG
        Request->Signature = ATA_DEVICE_REQUEST_SIGNATURE;
#endif
    }

    KeInitializeSpinLock(&DevExt->Device.QueueLock);
    InitializeListHead(&DevExt->Device.DeviceQueueList);
    InitializeListHead(&DevExt->PowerIrpQueueList);
    KeInitializeEvent(&DevExt->Device.QueueStoppedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&DevExt->Worker.EnumerationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&DevExt->Worker.ConfigureEvent, NotificationEvent, FALSE);

    return DevExt;

Failure:
    AtaPdoFreeDevice(DevExt);

    return NULL;
}
