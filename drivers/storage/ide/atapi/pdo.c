/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Child device object (PDO) dispatch routines
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static
CODE_SEG("PAGE")
VOID
AtaPdoSetConfig(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PATA_TASKFILE TaskFile = &DeviceExtension->ChannelExtension->TaskFile[SRB_TYPE_CONFIG];
    NTSTATUS Status;

    PAGED_CODE();

    /* ACPI initialization */
    if (DeviceExtension->GtfDataBuffer)
    {
        AtaAcpiExecuteTaskFile(DeviceExtension, DeviceExtension->GtfDataBuffer, TaskFile);
    }

    RtlZeroMemory(TaskFile, sizeof(*TaskFile));

    /* Make the device settings persistent for a software reset */
    TaskFile->Command = IDE_COMMAND_SET_FEATURE;
    TaskFile->Feature = IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON;
    Status = AtaSendTaskFile(DeviceExtension, TaskFile, SRB_TYPE_CONFIG, FALSE, 10, NULL, 0);
    TRACE("Feature Lock status 0x%lx\n", Status);

    /* Enable multiple mode */
    if (!IS_ATAPI(DeviceExtension))
    {
        DeviceExtension->MultiSectorTransfer =
            AtaMaximumSectorsPerDrq(&DeviceExtension->IdentifyDeviceData);

        if (DeviceExtension->MultiSectorTransfer != 0)
        {
            TaskFile->Command = IDE_COMMAND_SET_MULTIPLE;
            TaskFile->SectorCount = DeviceExtension->MultiSectorTransfer;

            Status = AtaSendTaskFile(DeviceExtension, TaskFile, SRB_TYPE_CONFIG, FALSE, 3, NULL, 0);
            if (!NT_SUCCESS(Status))
            {
                WARN("Multiple mode failed with status 0x%lx\n", Status);

                DeviceExtension->MultiSectorTransfer = 0;
            }
        }
    }

    /* Lock the security mode feature commands (secure erase and friends) */
    if (!AtapInPEMode && !IS_ATAPI(DeviceExtension))
    {
        TaskFile->Command = IDE_COMMAND_SECURITY_FREEZE_LOCK;
        TaskFile->Feature = 0;
        Status = AtaSendTaskFile(DeviceExtension, TaskFile, SRB_TYPE_CONFIG, FALSE, 10, NULL, 0);
        TRACE("SEC Lock status 0x%lx\n", Status);
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoStartDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    AtaPdoWmiRegistration(DeviceExtension, TRUE);

    /*
     * Get the IDE initialization commands to restore the boot up defaults.
     *
     * NOTE: This should be executed after _STM has been evaluated,
     * because it's expected that ACPI BIOS will use the identity data buffers
     * to construct the list of ATA commands to the drive.
    */
    if (!DeviceExtension->GtfDataBuffer)
    {
        DeviceExtension->GtfDataBuffer = AtaAcpiGetTaskFile(DeviceExtension);
    }

    AtaPdoSetConfig(DeviceExtension);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoStopDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoRemoveDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN FinalRemove)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeviceExtension->ChannelExtension;

    PAGED_CODE();

    AtaPdoWmiRegistration(DeviceExtension, FALSE);

    if (DeviceExtension->GtfDataBuffer)
        ExFreePoolWithTag(DeviceExtension->GtfDataBuffer, IDEPORT_TAG);

    if (FinalRemove && DeviceExtension->ReportedMissing)
    {
        PATAPORT_DEVICE_EXTENSION CurrentDeviceExtension;
        PSINGLE_LIST_ENTRY Entry, PrevEntry;
        ATA_SCSI_ADDRESS Address = DeviceExtension->AtaScsiAddress;

        IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, Irp);

        ExAcquireFastMutex(&ChannelExtension->DeviceSyncMutex);

        for (Entry = ChannelExtension->DeviceList.Next, PrevEntry = NULL;
             Entry != NULL;
             Entry = Entry->Next)
        {
            CurrentDeviceExtension = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

            if (CurrentDeviceExtension->AtaScsiAddress.AsULONG > Address.AsULONG)
                break;

            PrevEntry = Entry;
        }

        if (PrevEntry)
        {
            DeviceExtension->ListEntry.Next = PrevEntry->Next;
            PrevEntry->Next = DeviceExtension->ListEntry.Next;
        }
        else
        {
            PopEntryList(&DeviceExtension->ListEntry);
        }

        ExReleaseFastMutex(&ChannelExtension->DeviceSyncMutex);

        IoDeleteDevice(DeviceExtension->Common.Self);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStopRemoveDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (DeviceExtension->Common.PageFiles ||
        DeviceExtension->Common.HibernateFiles ||
        DeviceExtension->Common.DumpFiles)
    {
        return STATUS_DEVICE_BUSY;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryPnpDeviceState(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (DeviceExtension->Common.PageFiles ||
        DeviceExtension->Common.HibernateFiles ||
        DeviceExtension->Common.DumpFiles)
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
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PDEVICE_OBJECT Fdo, TopDeviceObject;
    PIO_STACK_LOCATION IoStack, SubStack;
    PIRP SubIrp;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    Fdo = DeviceExtension->ChannelExtension->Common.Self;
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
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    DEVICE_CAPABILITIES ParentCapabilities;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS Status;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    /* Get the capabilities of the parent device (pciidex, pcmcia, isapnp or the root PnP node) */
    RtlZeroMemory(&ParentCapabilities, sizeof(ParentCapabilities));
    ParentCapabilities.Size = sizeof(ParentCapabilities);
    ParentCapabilities.Version = 1;
    ParentCapabilities.Address = MAXULONG;
    ParentCapabilities.UINumber = MAXULONG;
    Status = AtaPdoRepeatRequest(DeviceExtension, Irp, &ParentCapabilities);
    if (!NT_SUCCESS(Status))
        return Status;

    DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;
    *DeviceCapabilities = ParentCapabilities;

    AtaScsiAddress = DeviceExtension->AtaScsiAddress;

    /* Override some fields */
    DeviceCapabilities->UINumber = AtaScsiAddress.TargetId;
    DeviceCapabilities->Address = (AtaScsiAddress.Lun << 4) | (AtaScsiAddress.TargetId & 0x0F);
    DeviceCapabilities->UniqueID = AtaPdoSerialNumberValid(DeviceExtension);
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
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    Status = AtaPdoRepeatRequest(DeviceExtension, Irp, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &DeviceExtension->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &DeviceExtension->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &DeviceExtension->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);
    IoInvalidateDeviceState(DeviceExtension->Common.Self);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryTargetDeviceRelations(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;

    PAGED_CODE();

    DeviceRelations = ExAllocatePoolUninitialized(PagedPool,
                                                  sizeof(*DeviceRelations),
                                                  IDEPORT_TAG);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceExtension->Common.Self;
    ObReferenceObject(DeviceExtension->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryId(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
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
            DeviceType = AtaTypeCodeToName(DeviceExtension, GetDeviceType);

            /* 'IDE\CdRomVBOX_CD-ROM_____________________________1.0_____' */
            CharCount = (sizeof("IDE\\") - 1) + strlen(DeviceType) +
                        ATAPORT_FN_FIELD + ATAPORT_RN_FIELD +
                        sizeof(ANSI_NULL);

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 IDEPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaCopyIdString(LocalBuffer,
                            (PUCHAR)DeviceExtension->FriendlyName,
                            ATAPORT_FN_FIELD,
                            '_');

            AtaCopyIdString(&LocalBuffer[ATAPORT_FN_FIELD],
                            (PUCHAR)DeviceExtension->RevisionNumber,
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

            DeviceType = AtaTypeCodeToName(DeviceExtension, GetDeviceType);
            GenericType = AtaTypeCodeToName(DeviceExtension, GetGenericType);

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
                                                 IDEPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            AtaCopyIdString(LocalBuffer,
                            (PUCHAR)DeviceExtension->FriendlyName,
                            ATAPORT_FN_FIELD,
                            '_');

            AtaCopyIdString(&LocalBuffer[ATAPORT_FN_FIELD],
                            (PUCHAR)DeviceExtension->RevisionNumber,
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
            GenericType = AtaTypeCodeToName(DeviceExtension, GetGenericType);

            /* 'GenCdRom' */
            CharCount = strlen(GenericType) + 2 * sizeof(ANSI_NULL); /* multi-string */

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 IDEPORT_TAG);
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
            if (AtaPdoSerialNumberValid(DeviceExtension))
            {
                /* The serial number in string form ('42562d3131303066333036662020202020202020') */
                CharCount = strlen(DeviceExtension->SerialNumber) + sizeof(ANSI_NULL);

                Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                     CharCount * sizeof(WCHAR),
                                                     IDEPORT_TAG);
                if (!Buffer)
                    return STATUS_INSUFFICIENT_RESOURCES;

                Status = RtlStringCchPrintfExW(Buffer,
                                               CharCount,
                                               &End,
                                               &Remaining,
                                               0,
                                               L"%hs",
                                               DeviceExtension->SerialNumber);
                ASSERT(NT_SUCCESS(Status));
            }
            else
            {
                ATA_SCSI_ADDRESS AtaScsiAddress;

                /*
                 * Use the 'Path.Target.Lun' format as a fallback.
                 * Note that on some ancient (pre-1994) IDE drives,
                 * IDENTIFY DEVICE words 7-255 are reserved and filled with zeros.
                 */
                CharCount = sizeof("FF.FF.FF");

                Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                     CharCount * sizeof(WCHAR),
                                                     IDEPORT_TAG);
                if (!Buffer)
                    return STATUS_INSUFFICIENT_RESOURCES;

                AtaScsiAddress = DeviceExtension->AtaScsiAddress;

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
            }

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
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
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
            CharCount = strlen(DeviceExtension->FriendlyName) + sizeof(ANSI_NULL);

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 IDEPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"%hs",
                                           DeviceExtension->FriendlyName);
            ASSERT(NT_SUCCESS(Status));

            INFO("TextDescription: '%S'\n", Buffer);
            break;
        }

        case DeviceTextLocationInformation:
        {
            /* Target ID (0 - Master, 1 - Slave) */
            CharCount = sizeof("0");

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 IDEPORT_TAG);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"%u",
                                           /* The C-Bus IDE can support up to 4 target devices */
                                           DeviceExtension->AtaScsiAddress.TargetId & 1);
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
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
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
            Status = AtaPdoStartDevice(DeviceExtension, Irp);
            break;

        case IRP_MN_STOP_DEVICE:
            Status = AtaPdoStopDevice(DeviceExtension, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Status = AtaPdoRemoveDevice(DeviceExtension,
                                        Irp,
                                        (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE));
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = AtaPdoQueryStopRemoveDevice(DeviceExtension, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = AtaPdoQueryCapabilities(DeviceExtension, Irp, IoStack);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = AtaPdoQueryPnpDeviceState(DeviceExtension, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = AtaPdoQueryId(DeviceExtension, Irp, IoStack);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            Status = AtaPdoQueryDeviceText(DeviceExtension, Irp, IoStack);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaPdoDeviceUsageNotification(DeviceExtension, Irp, IoStack);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
                Status = AtaPdoQueryTargetDeviceRelations(DeviceExtension, Irp);
            else
                Status = Irp->IoStatus.Status;
            break;
        }

        default:
            Status = Irp->IoStatus.Status;
            break;
    }

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

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
