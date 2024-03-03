/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA bus enumeration
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

VOID
AtaPdoSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    USHORT Cylinders, Heads, SectorsPerTrack;
    ULONG64 TotalSectors;

    DevExt->Flags &= ~(DEVICE_LBA_MODE | DEVICE_LBA48 | DEVICE_HAS_FUA);

    /* Using LBA addressing mode */
    if (IdentifyData->Capabilities.LbaSupported)
    {
        DevExt->Flags |= DEVICE_LBA_MODE;

        if (AtaHas48BitAddressFeature(IdentifyData))
        {
            /* Using LBA48 addressing mode */
            TotalSectors = ((ULONG64)IdentifyData->Max48BitLBA[1] << 32) |
                           IdentifyData->Max48BitLBA[0];

            DevExt->Flags |= DEVICE_LBA48;

            if (AtaHasForceUnitAccessCommands(IdentifyData))
                DevExt->Flags |= DEVICE_HAS_FUA;
        }
        else
        {
            /* Using LBA28 addressing mode */
            TotalSectors = IdentifyData->UserAddressableSectors;
        }
    }
    else
    {
        /* Using CHS addressing mode */
        if (AtaCurrentGeometryValid(IdentifyData))
        {
            Cylinders = IdentifyData->NumberOfCurrentCylinders;
            Heads = IdentifyData->NumberOfCurrentHeads;
            SectorsPerTrack = IdentifyData->CurrentSectorsPerTrack;
        }
        else
        {
            Cylinders = IdentifyData->NumCylinders;
            Heads = IdentifyData->NumHeads;
            SectorsPerTrack = IdentifyData->NumSectorsPerTrack;
        }
        DevExt->Cylinders = Cylinders;
        DevExt->Heads = Heads;
        DevExt->SectorsPerTrack = SectorsPerTrack;

        ASSERT(DevExt->SectorsPerTrack != 0);
        ASSERT(DevExt->Heads != 0);

        TotalSectors = Cylinders * Heads * SectorsPerTrack;
    }
    DevExt->TotalSectors = TotalSectors;

    DevExt->SectorSize = AtaBytesPerLogicalSector(IdentifyData);

    ASSERT(DevExt->TotalSectors != 0);
    ASSERT(DevExt->SectorSize >= 512);

    INFO("Total sectors %I64u of size %lu\n", DevExt->TotalSectors, DevExt->SectorSize);
}


static
CODE_SEG("PAGE")
VOID
AtaPdoEnableQueuedCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
#if 0
    ULONG NcqQueueDepth;

    PAGED_CODE();

    NcqQueueDepth = AtaDeviceQueueDepth(&DevExt->IdentifyDeviceData);
    if (NcqQueueDepth == 0)
        return;

    DevExt->MaxQueuedSlotsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - NcqQueueDepth);

    /* Don't set the bitmap to larger than the HBA can handle */
    DevExt->MaxQueuedSlotsBitmap &= DevExt->MaxRequestsBitmap;

    DevExt->Flags |= DEVICE_NCQ;

    INFO("NCQ enabled, queue depth %lu\n", NcqQueueDepth);
#endif
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaPdoIsSuperFloppy(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    if (DevExt->InquiryData.DeviceType == DIRECT_ACCESS_DEVICE)
    {
        /* ATAPI SuperDisk drives */
        return (strstr(DevExt->FriendlyName, " LS-120") ||
                strstr(DevExt->FriendlyName, " LS-240"));
    }

    return FALSE;
}

VOID
AtaCopyAtaString(
    _Out_writes_bytes_all_(Length) PUCHAR Destination,
    _In_reads_bytes_(Length) PUCHAR Source,
    _In_ ULONG Length)
{
    ULONG i;

    ASSERT((Length % sizeof(USHORT)) == 0);

    /* Copy the ATA string and swap it */
    for (i = 0; i < (Length / sizeof(USHORT)); i += sizeof(USHORT))
    {
        Destination[i] = Source[i + 1];
        Destination[i + 1] = Source[i];
    }
}

CODE_SEG("PAGE")
PCHAR
AtaCopyIdString(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter)
{
    PCHAR Dest = Destination;

    PAGED_CODE();

    while (MaxLength)
    {
        UCHAR Char = *Source;

        /* Only characters from space to tilde are allowed in an ID */
        if (Char > ' ' && Char <= '~' && Char != ',')
        {
            *Dest = Char;
        }
        else
        {
            *Dest = DefaultCharacter;
        }

        ++Source;
        ++Dest;
        --MaxLength;
    }

    return Dest;
}

static
VOID
AtaSwapIdString(
    _Inout_updates_bytes_(Length * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG Length)
{
    PUSHORT Word = Buffer;

    PAGED_CODE();

    while (Length--)
    {
        *Word = RtlUshortByteSwap(*Word);
        ++Word;
    }
}

static
CODE_SEG("PAGE")
PCHAR
AtaTrimIdString(
    _In_ _Post_z_ PCHAR Start,
    _In_ PCHAR End)
{
    PCHAR Current = End - 1;

    PAGED_CODE();

    /* Remove trailing spaces */
    while (Current >= Start && *Current == ' ')
    {
        --Current;
    }
    Current[1] = ANSI_NULL;

    return (Current + 1);
}

static
CODE_SEG("PAGE")
VOID
AtaPdoFillIdentificationStrings(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    PCHAR End;

    PAGED_CODE();

    /*
     * For ATAPI devices Inquiry Data is preferred over Identify Data.
     * ATA strings are *not* byte-swapped for early ATAPI drives (NEC CDR-260, etc.).
     */
    if (IS_ATAPI(DevExt))
    {
        /* Combine VendorId and ProductId, separated by a space */
        End = AtaCopyIdString(DevExt->FriendlyName,
                              DevExt->InquiryData.VendorId,
                              RTL_FIELD_SIZE(INQUIRYDATA, VendorId),
                              ' ');
        End = AtaTrimIdString(DevExt->FriendlyName, End);
        *End++ = ' ';
        End = AtaCopyIdString(End,
                              DevExt->InquiryData.ProductId,
                              RTL_FIELD_SIZE(INQUIRYDATA, ProductId),
                              ' ');
        AtaTrimIdString(DevExt->FriendlyName, End);

        /* Copy ProductRevisionLevel */
        End = AtaCopyIdString(DevExt->RevisionNumber,
                              DevExt->InquiryData.ProductRevisionLevel,
                              RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel),
                              ' ');
        AtaTrimIdString(DevExt->RevisionNumber, End);
    }
    else
    {
        /* Copy ModelNumber from a byte-swapped ATA string */
        End = AtaCopyIdString(DevExt->FriendlyName,
                              IdentifyData->ModelNumber,
                              ATAPORT_FN_FIELD,
                              ' ');
        AtaSwapIdString(DevExt->FriendlyName, ATAPORT_FN_FIELD / 2);
        AtaTrimIdString(DevExt->FriendlyName, End);

        /* Copy FirmwareRevision from a byte-swapped ATA string */
        End = AtaCopyIdString(DevExt->RevisionNumber,
                              IdentifyData->FirmwareRevision,
                              ATAPORT_RN_FIELD,
                              ' ');
        AtaSwapIdString(DevExt->RevisionNumber, ATAPORT_RN_FIELD / 2);
        AtaTrimIdString(DevExt->RevisionNumber, End);
    }

    /* Format the serial number */
    if (AtaDeviceSerialNumberValid(IdentifyData))
    {
        ULONG i;
        NTSTATUS Status;
        size_t Remaining;

        End = DevExt->SerialNumber;
        Remaining = sizeof(DevExt->SerialNumber);

        for (i = 0; i < sizeof(IdentifyData->SerialNumber); ++i)
        {
            Status = RtlStringCchPrintfExA(End,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           "%2x",
                                           IdentifyData->SerialNumber[i]);
            ASSERT(NT_SUCCESS(Status));
        }
    }
    else
    {
        /* Bogus S/N is ignored */
    }

    INFO("FriendlyName: '%s'\n", DevExt->FriendlyName);
    INFO("RevisionNumber: '%s'\n", DevExt->RevisionNumber);
    INFO("SerialNumber: '%s'\n", AtaPdoSerialNumberValid(DevExt) ?
                                 DevExt->SerialNumber : "<INVALID>");
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    AtaPdoFillIdentificationStrings(DevExt);

    if (IS_ATAPI(DevExt))
    {
        if (AtaPdoIsSuperFloppy(DevExt))
            DevExt->Flags |= DEVICE_IS_SUPER_FLOPPY;
    }
    else
    {
        if (ChanExt->AhciCapabilities & AHCI_CAP_SNCQ)
            AtaPdoEnableQueuedCommands(DevExt);

        AtaPdoSetAddressingMode(DevExt);
        AtaCreateStandardInquiryData(DevExt);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaIdentifyStringsEqual(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData1,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData2)
{
    PAGED_CODE();

    if (!RtlEqualMemory(IdentifyData1->SerialNumber,
                        IdentifyData2->SerialNumber,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber)))
    {
        return FALSE;
    }

    if (!RtlEqualMemory(IdentifyData1->FirmwareRevision,
                        IdentifyData2->FirmwareRevision,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, FirmwareRevision) +
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber)))
    {
        return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
ULONG
AtaInquiryStringsEqual(
    _In_ PINQUIRYDATA InquiryData1,
    _In_ PINQUIRYDATA InquiryData2)
{
    PAGED_CODE();

    return RtlEqualMemory(InquiryData1->VendorId,
                          InquiryData2->VendorId,
                          RTL_FIELD_SIZE(INQUIRYDATA, VendorId) +
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductId) +
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel));

}

static
VOID
CODE_SEG("PAGE")
AtaPdoPrepareForAtapiEnum(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    static const CHAR Cdr260ModelId[] =  "NEC                 CD-ROM DRIVE";

    PAGED_CODE();

    DevExt->CdbSize = AtaDeviceCdbSizeInWords(&DevExt->IdentifyPacketData);
    if (IS_AHCI(DevExt))
        DevExt->CdbSize *= 2; // Used for data transfers to ACMD region

    if (AtaDeviceHasCdbInterrupt(&DevExt->IdentifyPacketData))
        DevExt->Flags |= DEVICE_HAS_CDB_INTERRUPT;
    else
        DevExt->Flags &= ~DEVICE_HAS_CDB_INTERRUPT;

    if (AtaDeviceDmaDirectionRequired(&DevExt->IdentifyPacketData))
        DevExt->Flags |= DEVICE_NEED_DMA_DIRECTION;
    else
        DevExt->Flags &= ~DEVICE_NEED_DMA_DIRECTION;

    if (RtlEqualMemory(DevExt->IdentifyPacketData.ModelNumber,
                       Cdr260ModelId,
                       sizeof(Cdr260ModelId) - 1))
    {
        DevExt->Flags |= DEVICE_IS_NEC_CDR260;
    }
    else
    {
        DevExt->Flags &= ~DEVICE_IS_NEC_CDR260;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaSendTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_TASKFILE TaskFile,
    _In_ ATA_SRB_TYPE SrbType,
    _In_ ULONG RequestFlags,
    _In_ ULONG TimeoutInSeconds,
    _In_opt_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_opt_ PVOID SenseInfoBuffer,
    _In_ ULONG SenseInfoBufferLength)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    KEVENT Event;

    PAGED_CODE();

    AtaScsiAddress = DevExt->AtaScsiAddress;

    Srb = &ChanExt->InternalSrb[SrbType];
    Srb->PathId = AtaScsiAddress.PathId;
    Srb->TargetId = AtaScsiAddress.TargetId;
    Srb->Lun = AtaScsiAddress.Lun;

    Srb->DataBuffer = Buffer;
    Srb->DataTransferLength = BufferLength;

    Srb->TimeOutValue = TimeoutInSeconds;

    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->SrbFlags = SRB_FLAGS_NO_QUEUE_FREEZE;
    if (Buffer)
        Srb->SrbFlags |= SRB_FLAGS_DATA_IN;

    Srb->SenseInfoBuffer = SenseInfoBuffer;
    Srb->SenseInfoBufferLength = SenseInfoBufferLength;

    Srb->SrbExtension = (PVOID)(ULONG_PTR)SRB_FLAG_INTERNAL;
    SRB_SET_FLAGS(Srb, RequestFlags);

    Irp = Srb->OriginalRequest;
    Irp->UserBuffer = TaskFile;
    Irp->UserEvent = &Event;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    AtaReqStartSrb(DevExt, Srb);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    return Irp->IoStatus.Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaSendInquiry(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Out_ PINQUIRYDATA InquiryData)
{
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PSENSE_DATA SenseData;
    NTSTATUS Status;

    PAGED_CODE();

    Srb = &ChanExt->InternalSrb[SRB_TYPE_ENUM];
    Srb->CdbLength = 6;

    Cdb = (PCDB)&Srb->Cdb;

    RtlZeroMemory(Cdb, 12);
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = DevExt->AtaScsiAddress.Lun;
    Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    /* Pass a sense buffer to identify broken Xbox CD/DVD drives */
    SenseData = &ChanExt->SenseData;
    RtlZeroMemory(SenseData, sizeof(*SenseData));

    Status = AtaSendTaskFile(DevExt,
                             NULL,
                             SRB_TYPE_ENUM,
                             SRB_FLAG_PACKET_COMMAND,
                             10,
                             InquiryData,
                             INQUIRYDATABUFFERSIZE,
                             SenseData,
                             sizeof(*SenseData));
    if (NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
    {
        ERR("Inquiry failed, SK 0x%x, ASC 0x%x, ASCQ 0x%x\n",
            SenseData->SenseKey,
            SenseData->AdditionalSenseCode,
            SenseData->AdditionalSenseCodeQualifier);

        ASSERT(0);

        /*
         * The INQUIRY command is mandatory to be implemented by ATAPI devices,
         * but some Xbox drives violate this.
         */
        if ((DevExt->AtaScsiAddress.Lun == 0) &&
            (SenseData->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST))
        {
            if (0) // TODO IsXboxDrive
            {
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_IO_DEVICE_ERROR;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoIdentifySpinUpPuisDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_TASKFILE TaskFile = &ChanExt->TaskFile[SRB_TYPE_ENUM];
    NTSTATUS Status;

    PAGED_CODE();

    // TODO: not quite right, see the spec
    RtlZeroMemory(TaskFile, sizeof(*TaskFile));
    TaskFile->Command = IDE_COMMAND_SET_FEATURE;
    TaskFile->Feature = IDE_FEATURE_PUIS_SPIN_UP;

    Status = AtaSendTaskFile(DevExt, TaskFile, SRB_TYPE_ENUM, 0, 20, NULL, 0, NULL, 0);

    TRACE("PUIS status 0x%lx\n", Status);
}

static
CODE_SEG("PAGE")
ATA_DEVICE_CLASS
AtaFdoIdentifyDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATAPORT_DEVICE_EXTENSION OldDevExt,
    _Out_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData)
{
    ATA_DEVICE_CLASS DeviceClass;
    ULONG DiscoveryOrder, RetryCount;
    BOOLEAN StartFromAtapi;
    NTSTATUS Status;

    PAGED_CODE();

    if (OldDevExt)
    {
        StartFromAtapi = IS_ATAPI(OldDevExt);
    }
    else
    {
        ULONG KeyValue;

        /* Query the last device type */
        AtaGetRegistryKey(ChanExt, DevExt, L"DeviceType", &KeyValue, DEV_UNKNOWN);

        StartFromAtapi = (KeyValue == DEV_ATAPI);
    }

    if (IS_AHCI(DevExt))
    {
        ULONG Signature;

        Signature = AhciPortEnumerate(ChanExt, DevExt->AtaScsiAddress.TargetId);

        if (Signature == AHCI_PXSIG_INVALID)
            return DEV_UNKNOWN;

        StartFromAtapi = (Signature == AHCI_PXSIG_ATAPI);
    }

    /*
     * Try the last device type first.
     * This may speed up device detection by eliminating I/O errors.
     */
    if (StartFromAtapi)
        DiscoveryOrder = DEV_ATAPI ^ DEV_ATA;
    else
        DiscoveryOrder = 0;

    /* Look for ATA/ATAPI devices */
    for (DeviceClass = DEV_ATA; DeviceClass <= DEV_ATAPI; ++DeviceClass)
    {
        PATA_TASKFILE TaskFile = &ChanExt->TaskFile[SRB_TYPE_ENUM];

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));

        /* Send the identify command */
        if ((DeviceClass ^ DiscoveryOrder) == DEV_ATA)
            TaskFile->Command = IDE_COMMAND_IDENTIFY;
        else
            TaskFile->Command = IDE_COMMAND_ATAPI_IDENTIFY;

        /*
         * Disable interrupts for the identify command, use polling instead.
         *
         * - On some single device 1 configurations or non-existent IDE channels
         *   the status register stuck permanently at a value of 0,
         *   and we incorrectly assume that the device is present.
         *   This will result in a taskfile timeout
         *   which must be avoided as it would cause hangs at boot time.
         *
         * - The NEC CDR-260 drive uses the Packet Command protocol (?) for this command,
         *   which causes a spurious (completion status) interrupt after reading the data buffer.
         */

        for (RetryCount = 0; RetryCount < 2; ++RetryCount)
        {
            TRACE("Send identify %s\n",
                  TaskFile->Command == IDE_COMMAND_ATAPI_IDENTIFY ? "ATAPI" : "ATA");

            Status = AtaSendTaskFile(DevExt,
                                     TaskFile,
                                     SRB_TYPE_ENUM,
                                     SRB_FLAG_POLL | SRB_FLAG_DEVICE_CHECK,
                                     15,
                                     IdentifyDeviceData,
                                     sizeof(*IdentifyDeviceData),
                                     NULL,
                                     0);
            if (!NT_SUCCESS(Status))
                break;

            /* The device was unable to return complete response data */
            if (AtaDeviceInPuisState(IdentifyDeviceData))
            {
                /* The drive needs to be spun up */
                AtaFdoIdentifySpinUpPuisDevice(ChanExt, DevExt);
                continue;
            }

            return DeviceClass ^ DiscoveryOrder;
        }
    }

    return DEV_UNKNOWN;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoInitializeDeviceRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PDEVICE_RELATIONS DeviceRelations)
{
    PSINGLE_LIST_ENTRY Entry;
    PATAPORT_DEVICE_EXTENSION DevExt;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    /* Initialize the device relations structure */
    DeviceRelations->Count = 0;
    for (Entry = ChanExt->DeviceList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);
        if (DevExt->ReportedMissing)
            continue;

        DeviceRelations->Objects[DeviceRelations->Count++] = DevExt->Common.Self;
        ObReferenceObject(DevExt->Common.Self);
    }
    ChanExt->DeviceCount = DeviceRelations->Count;

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PATAPORT_DEVICE_EXTENSION EnumDevExt;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG DeviceNumber, MaxDevices, Lun, MaxLun, Size;
#if DBG
    LARGE_INTEGER TimeStart, TimeFinish;
    ULONG QbrTimeMs;
#endif

    PAGED_CODE();

#if DBG
    KeQuerySystemTime(&TimeStart);
#endif

    /* Get the channel timing information */
    if (IS_PCIIDE(ChanExt))
    {
        if (AtaAcpiGetTimingMode(ChanExt, &ChanExt->TimingMode))
            ChanExt->Flags |= CHANNEL_HAS_GTM;
        else
            ChanExt->Flags &= ~CHANNEL_HAS_GTM;
    }

    /* Allocate a dummy PDO extension to service I/O requests during device discovery */
    EnumDevExt = AtaPdoCreateDevice(ChanExt, AtaMarshallScsiAddress(ChanExt->PathId, 0xFF, 0));
    if (!EnumDevExt)
    {
        ERR("Failed to allocate PDO extension\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    EnumDevExt->Flags |= DEVICE_ENUM | DEVICE_PIO_ONLY;

    MaxDevices = AtaFdoMaxDeviceCount(ChanExt);
    MaxLun = 1;

    /*
     * Preallocate the device relations structure we need for the QBR IRP,
     * because we don't want to be in a state where allocation fails.
     */
    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects[MaxDevices * ATA_MAX_LUN_COUNT]);
    DeviceRelations = ExAllocatePoolUninitialized(PagedPool, Size, ATAPORT_TAG);
    if (!DeviceRelations)
    {
        ERR("Failed to allocate device relations\n");

        AtaPdoFreeDevice(EnumDevExt);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Detect and enumerate ATA/ATAPI devices */
    for (DeviceNumber = 0; DeviceNumber < MaxDevices; ++DeviceNumber)
    {
        if (!(ChanExt->DeviceBitmap & (1 << DeviceNumber)))
            continue;

        for (Lun = 0; Lun < MaxLun; ++Lun)
        {
            PATAPORT_DEVICE_EXTENSION DevExt, OldDevExt;
            ATA_DEVICE_CLASS DeviceClass;
            ATA_SCSI_ADDRESS AtaScsiAddress;
            NTSTATUS Status;

            TRACE("Scanning tid %lu lun %lu\n", DeviceNumber, Lun);

            AtaScsiAddress = AtaMarshallScsiAddress(ChanExt->PathId, DeviceNumber, Lun);

            /* Try to find an existing device */
            OldDevExt = AtaFdoFindDeviceByPath(ChanExt, AtaScsiAddress);
            if (OldDevExt)
            {
                /*
                 * Unrecoverable error, this device need to go away.
                 * For the target device it also removes the associated LUNs.
                 */
                if (OldDevExt->Flags & DEVICE_HARDWARE_ERROR)
                {
                    OldDevExt->ReportedMissing = TRUE;
                    continue;
                }

                DevExt = OldDevExt;
            }
            else
            {
                DevExt = EnumDevExt;

                AtaPdoUpdateScsiAddress(EnumDevExt, AtaScsiAddress);
            }

            /* Determine the type of the target device */
            if (Lun == 0)
            {
                DeviceClass = AtaFdoIdentifyDevice(ChanExt,
                                                   DevExt,
                                                   OldDevExt,
                                                   &EnumDevExt->IdentifyDeviceData);

                INFO("Device Type %u at [tid %lu lun %lu]\n", DeviceClass, DeviceNumber, Lun);

                /* Scan all logical units to support rare multi-lun ATAPI devices */
                if (DeviceClass == DEV_ATAPI)
                    MaxLun = AtaDeviceMaxLun(&DevExt->IdentifyPacketData);
                else
                    MaxLun = 1;

                /* No device present at this SCSI address */
                if (DeviceClass == DEV_UNKNOWN)
                {
                    if (OldDevExt)
                        OldDevExt->ReportedMissing = TRUE;

                    continue;
                }

                /* Check if the drive was replaced with another of the same type */
                if (OldDevExt)
                {
                    if (AtaPdoGetDeviceClass(OldDevExt) != DeviceClass ||
                        !AtaIdentifyStringsEqual(&EnumDevExt->IdentifyDeviceData,
                                                 &OldDevExt->IdentifyDeviceData))
                    {
                        OldDevExt->ReportedMissing = TRUE;
                    }
                    else if (DeviceClass != DEV_ATAPI)
                    {
                        /* It's the same device still */
                        continue;
                    }
                }
            }
            else
            {
                DeviceClass = DEV_ATAPI;
            }

            /* Send the INQUIRY command for each logical unit */
            if (DeviceClass == DEV_ATAPI)
            {
                /* Prepare the device extension for the I/O request */
                if (DevExt == EnumDevExt)
                    AtaPdoPrepareForAtapiEnum(DevExt);

                INFO("Send inquiry [tid %lu lun %lu]\n", DeviceNumber, Lun);

                Status = AtaSendInquiry(ChanExt, DevExt, &EnumDevExt->InquiryData);
                if (!NT_SUCCESS(Status))
                {
                    /* No device present at this SCSI address */
                    if (OldDevExt)
                        OldDevExt->ReportedMissing = TRUE;

                    continue;
                }
                else if (OldDevExt)
                {
                    /* Check if the device was replaced with another of the same type */
                    if (!AtaInquiryStringsEqual(&EnumDevExt->InquiryData,
                                                &OldDevExt->InquiryData))
                    {
                        OldDevExt->ReportedMissing = TRUE;
                    }
                    else
                    {
                        /* It's the same device still */
                        continue;
                    }
                }
            }

            /*
             * At this point, we assume that the drive is a new device,
             * allocate a PDO instance, and initialize it.
             */
            DevExt = AtaPdoCreateDevice(ChanExt, AtaScsiAddress);
            if (!DevExt)
            {
                /* We are out of memory, trying to continue process the QBR IRP anyway */
                continue;
            }
            AtaPdoUpdateScsiAddress(DevExt, AtaScsiAddress);

            /* Save the new device type */
            AtaSetRegistryKey(ChanExt, DevExt, L"DeviceType", DeviceClass);

            /* Copy parameter information from the device or from lun 0 for ATAPI devices */
            RtlCopyMemory(&DevExt->IdentifyDeviceData,
                          &EnumDevExt->IdentifyDeviceData,
                          sizeof(EnumDevExt->IdentifyDeviceData));

            /* The spindle motor requires an inrush of power at start-up */
            if (!AtaDeviceIsSsd(&DevExt->IdentifyDeviceData))
            {
                DevExt->Common.Self->Flags |= DO_POWER_INRUSH;
            }
            DevExt->Common.Self->Flags &= ~DO_DEVICE_INITIALIZING;

            if (DeviceClass == DEV_ATAPI)
            {
                /* Copy INQUIRY data from the device */
                RtlCopyMemory(&DevExt->InquiryData,
                              &EnumDevExt->InquiryData,
                              sizeof(EnumDevExt->InquiryData));

                DevExt->Flags |= DEVICE_IS_ATAPI;

                /* Copy the transport parameters */
                DevExt->Flags |= EnumDevExt->Flags & (DEVICE_HAS_CDB_INTERRUPT |
                                                      DEVICE_NEED_DMA_DIRECTION |
                                                      DEVICE_IS_NEC_CDR260);
                DevExt->CdbSize = EnumDevExt->CdbSize;
            }

            INFO("Found device %u [%lu %u %u %u]\n",
                 DevExt->InquiryData.DeviceType,
                 ChanExt->PortNumber,
                 ChanExt->PathId,
                 DeviceNumber,
                 Lun);

            AtaPdoInit(ChanExt, DevExt);

            /* Wait for a PnP START IRP */
            AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PNP);

            AtaFdoDeviceListInsert(ChanExt, DevExt);
        }
    }

    AtaPdoFreeDevice(EnumDevExt);
    AtaFdoInitializeDeviceRelations(ChanExt, DeviceRelations);

#if DBG
    KeQuerySystemTime(&TimeFinish);

    QbrTimeMs = (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000;

    if (QbrTimeMs >= 5000)
        WARN("QBR request took %lu ms\n", QbrTimeMs);
    else
        INFO("QBR request took %lu ms\n", QbrTimeMs);
#endif

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}
