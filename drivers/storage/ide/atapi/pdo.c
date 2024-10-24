/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Child device object (PDO) dispatch routines
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static
CODE_SEG("PAGE")
VOID
AtaPdoConfigureDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    KeClearEvent(&DevExt->WorkerContext.ConfiguredEvent);

    AtaDeviceQueueAction(DevExt, ACTION_CONFIG);

    KeWaitForSingleObject(&DevExt->WorkerContext.ConfiguredEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoStartDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (!(DevExt->Flags & DEVICE_ACTIVE))
    {
        AtaPdoWmiRegistration(DevExt, TRUE);

        /* Update the type of device connected to the port */
        if (IS_AHCI(DevExt))
        {
            AtaAcpiSetDeviceData(DevExt, &DevExt->IdentifyDeviceData);
        }

        /*
         * Get the ATA initialization commands to restore the boot up defaults.
         * This should be executed after _STM or _SDD has been evaluated,
         * because it's expected that ACPI BIOS will use the identity data buffers
         * to construct the list of ATA commands to the drive.
        */
        if (!DevExt->GtfDataBuffer)
        {
            DevExt->GtfDataBuffer = AtaAcpiGetTaskFile(DevExt);
        }

        /* Set the new ATA volatile settings */
        AtaPdoConfigureDevice(DevExt);

        DevExt->Flags |= DEVICE_ACTIVE;
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
    AtaReqWaitForOutstandingIoToComplete(DevExt, 0);

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
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;

    PAGED_CODE();

    if (DevExt->GtfDataBuffer)
    {
        ExFreePoolWithTag(DevExt->GtfDataBuffer, ATAPORT_TAG);
        DevExt->GtfDataBuffer = NULL;
    }

    AtaPdoWmiRegistration(DevExt, FALSE);

    if (FinalRemove && DevExt->ReportedMissing)
    {
        IoReleaseRemoveLockAndWait(&DevExt->Common.RemoveLock, Irp);

        AtaReqFlushDeviceQueue(DevExt);

        AtaFdoDeviceListRemove(ChanExt, DevExt);

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

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryPnpDeviceState(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (DevExt->Common.PageFiles ||
        DevExt->Common.HibernateFiles ||
        DevExt->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    return STATUS_SUCCESS;
}

static IO_COMPLETION_ROUTINE AtaOnRepeaterCompletion;

static
NTSTATUS
NTAPI
AtaOnRepeaterCompletion(
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
AtaPdoRepeatRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PDEVICE_OBJECT Fdo, TopDeviceObject;
    PIO_STACK_LOCATION IoStack, SubStack;
    PIRP SubIrp;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    Fdo = DevExt->ChanExt->Common.Self;
    TopDeviceObject = IoGetAttachedDeviceReference(Fdo);

    SubIrp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!SubIrp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    SubStack = IoGetNextIrpStackLocation(SubIrp);
    RtlCopyMemory(SubStack, IoStack, sizeof(*IoStack));

    if (DeviceCapabilities)
        SubStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    IoSetCompletionRoutine(SubIrp,
                           AtaOnRepeaterCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    SubIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(TopDeviceObject, SubIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    ObDereferenceObject(TopDeviceObject);

    Status = SubIrp->IoStatus.Status;
    IoFreeIrp(SubIrp);

    return Status;
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
    Status = AtaPdoRepeatRequest(DevExt, Irp, &ParentCapabilities);
    if (!NT_SUCCESS(Status))
        return Status;

    DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;
    *DeviceCapabilities = ParentCapabilities;

    AtaScsiAddress = DevExt->AtaScsiAddress;

    /* Override some fields */
    DeviceCapabilities->UINumber = AtaScsiAddress.TargetId;
    DeviceCapabilities->Address = (AtaScsiAddress.Lun << 4) | (AtaScsiAddress.TargetId & 0x0F);
    DeviceCapabilities->UniqueID = AtaPdoIsSerialNumberValid(DevExt);
    DeviceCapabilities->Removable = FALSE;
    DeviceCapabilities->SurpriseRemovalOK = FALSE;
    DeviceCapabilities->D1Latency =
    DeviceCapabilities->D2Latency =
    DeviceCapabilities->D3Latency = 31 * 10000; /* 31 seconds */

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoDeviceUsageNotification(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    Status = AtaPdoRepeatRequest(DevExt, Irp, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &DevExt->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &DevExt->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &DevExt->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);
    IoInvalidateDeviceState(DevExt->Common.Self);

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

            AtaCopyIdString(LocalBuffer,
                            (PUCHAR)DevExt->FriendlyName,
                            ATAPORT_FN_FIELD,
                            '_');

            AtaCopyIdString(&LocalBuffer[ATAPORT_FN_FIELD],
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

            AtaCopyIdString(LocalBuffer,
                            (PUCHAR)DevExt->FriendlyName,
                            ATAPORT_FN_FIELD,
                            '_');

            AtaCopyIdString(&LocalBuffer[ATAPORT_FN_FIELD],
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
            /*
             * Note that on some ancient (pre-1994) IDE drives,
             * IDENTIFY DEVICE words 7-255 are reserved and filled with zeros.
             */
            if (AtaPdoIsSerialNumberValid(DevExt))
            {
                /* The serial number in string form ('42562d3131303066333036662020202020202020') */
                CharCount = strlen(DevExt->SerialNumber) + sizeof(ANSI_NULL);

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
                                               DevExt->SerialNumber);
            }
            else
            {
                ATA_SCSI_ADDRESS AtaScsiAddress;

                /* Use the 'Path.Target.Lun' format as a fallback */
                CharCount = sizeof("FF.FF.FF");

                Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                     CharCount * sizeof(WCHAR),
                                                     ATAPORT_TAG);
                if (!Buffer)
                    return STATUS_INSUFFICIENT_RESOURCES;

                AtaScsiAddress = DevExt->AtaScsiAddress;

                Status = RtlStringCchPrintfExW(Buffer,
                                               CharCount,
                                               &End,
                                               &Remaining,
                                               0,
                                               L"%x.%x.%x",
                                               AtaScsiAddress.PathId,
                                               AtaScsiAddress.TargetId,
                                               AtaScsiAddress.Lun);
            }
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
            UCHAR Value;

            CharCount = sizeof("99");

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 ATAPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            if (IS_AHCI(DevExt))
            {
                /* The channel number */
                Value = DevExt->AtaScsiAddress.PathId;
            }
            else
            {
                /*
                 * The Master/Slave designation (0 - Master, 1 - Slave).
                 * Note that the C-Bus IDE can support up to 4 target devices,
                 * so we extract bit 0 from the target ID.
                 */
                Value = DevExt->AtaScsiAddress.TargetId & 1;
            }

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"%u",
                                           Value);
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

    INFO("(%p, %p) Tid.%lu\n",
         DevExt->Common.Self,
         Irp,
         DevExt->AtaScsiAddress.TargetId);

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
            Status = AtaPdoQueryPnpDeviceState(DevExt, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = AtaPdoQueryId(DevExt, Irp, IoStack);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            Status = AtaPdoQueryDeviceText(DevExt, Irp, IoStack);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaPdoDeviceUsageNotification(DevExt, Irp, IoStack);
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

static
VOID
CODE_SEG("PAGE")
AtaPdoFreeExtension(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    if (DevExt->Requests)
        ExFreePoolWithTag(DevExt->Requests, ATAPORT_TAG);
}

VOID
CODE_SEG("PAGE")
AtaPdoFreeDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    AtaPdoFreeExtension(DevExt);

    IoDeleteDevice(DevExt->Common.Self);
}

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaPdoCreateDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PATAPORT_DEVICE_EXTENSION DevExt;
    PDEVICE_OBJECT Pdo;
    ULONG Alignment, QueueDepth;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\IdeDeviceP999T9L9-FFF")];
    static ULONG PdoNumber = 0;
    ULONG i;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\IdeDeviceP%luT%luL%lu-%lx",
                                ChanExt->ChannelNumber,
                                AtaScsiAddress.TargetId,
                                AtaScsiAddress.Lun,
                                PdoNumber++);
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
    Alignment = ChanExt->Common.Self->AlignmentRequirement;
    Pdo->AlignmentRequirement = max(Alignment, FILE_WORD_ALIGNMENT);

    Pdo->Flags |= DO_DIRECT_IO;

    DevExt = Pdo->DeviceExtension;

    RtlZeroMemory(DevExt, sizeof(*DevExt));
    DevExt->Common.Self = Pdo;
    DevExt->Common.Flags = ChanExt->Common.Flags & ~DO_IS_FDO;
    DevExt->ChanExt = ChanExt;
    DevExt->SectorSize = 512;

    if (IS_AHCI(DevExt))
    {
        DevExt->StartIo = AtaAhciStartIo;
        DevExt->PreparePrdTable = AtaAhciPreparePrdTable;

        QueueDepth = ((ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;
    }
    else
    {
        DevExt->StartIo = AtaPataStartIo;
        DevExt->PreparePrdTable = AtaPciIdePreparePrdTable;

        QueueDepth = 1;
    }
    DevExt->SendRequest = AtaReqSendRequest;

    DevExt->FreeRequestsBitmap =
    DevExt->MaxRequestsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - QueueDepth);

    DevExt->Requests = ExAllocatePoolZero(NonPagedPool,
                                          sizeof(ATA_DEVICE_REQUEST) * (QueueDepth + 1),
                                          ATAPORT_TAG);
    if (!DevExt->Requests)
        goto Failure;

    DevExt->InternalRequest = &DevExt->Requests[QueueDepth];
    DevExt->InternalRequest->Complete = AdaDeviceCompleteInternalRequest;

    for (i = 0; i < QueueDepth + 1; ++i)
    {
        PATA_DEVICE_REQUEST Request = &DevExt->Requests[i];

        Request->DevExt = DevExt;
#if DBG
        Request->Signature = ATA_DEVICE_REQUEST_SIGNATURE;
#endif
    }

    IoInitializeRemoveLock(&DevExt->Common.RemoveLock, ATAPORT_TAG, 0, 0);
    KeInitializeSpinLock(&DevExt->QueueLock);
    InitializeListHead(&DevExt->RequestList);
    InitializeSListHead(&DevExt->CompletionQueueList);
    InitializeListHead(&DevExt->DeviceQueueList);
    InitializeListHead(&DevExt->BypassQueueList);
    KeInitializeDpc(&DevExt->CompletionDpc, AtaReqCompletionDpc, DevExt);
    KeInitializeEvent(&DevExt->QueueStoppedEvent, NotificationEvent, FALSE);

    KeInitializeDpc(&DevExt->WorkerContext.Dpc, AtaDeviceWorker, DevExt);
    KeInitializeEvent(&DevExt->WorkerContext.EnumeratedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&DevExt->WorkerContext.ConfiguredEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&DevExt->WorkerContext.CompletedEvent, NotificationEvent, FALSE);

    return DevExt;

Failure:
    AtaPdoFreeExtension(DevExt);

    return NULL;
}

CODE_SEG("PAGE")
VOID
AtaPdoUpdateScsiAddress(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;

    PAGED_CODE();

    DevExt->AtaScsiAddress = AtaScsiAddress;
    DevExt->DeviceSelect = IDE_DRIVE_SELECT | AtaScsiAddress.Lun;

    if (IS_AHCI(DevExt))
    {
        DevExt->LocalBuffer = ChanExt->PortInfo[AtaScsiAddress.TargetId].LocalBuffer;
        DevExt->PortData = &ChanExt->PortData[AtaScsiAddress.TargetId];
    }
    else
    {
        ASSERT(AtaScsiAddress.TargetId < CHANNEL_PC98_MAX_DEVICES);

        DevExt->LocalBuffer = ChanExt->PortData->Pata.LocalBuffer;
        DevExt->PortData = ChanExt->PortData;

        /* Master/slave bit */
        DevExt->DeviceSelect |= ((AtaScsiAddress.TargetId & 1) << 4);
    }
}
