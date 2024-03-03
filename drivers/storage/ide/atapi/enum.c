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
    ULONG64 TotalSectors;

    DevExt->Flags &= ~(DEVICE_LBA_MODE | DEVICE_LBA48 | DEVICE_HAS_FUA);

    /* Using LBA addressing mode */
    if (AtaDevHasLbaTranslation(IdentifyData))
    {
        DevExt->Flags |= DEVICE_LBA_MODE;

        if (AtaDevHas48BitAddressFeature(IdentifyData))
        {
            /* Using LBA48 addressing mode */
            TotalSectors = AtaDevUserAddressableSectors48Bit(IdentifyData);
            ASSERT(TotalSectors <= ATA_MAX_LBA_48);

            DevExt->Flags |= DEVICE_LBA48;

            if (AtaDevHasForceUnitAccessCommands(IdentifyData))
                DevExt->Flags |= DEVICE_HAS_FUA;
        }
        else
        {
            /* Using LBA28 addressing mode */
            TotalSectors = AtaDevUserAddressableSectors28Bit(IdentifyData);
            ASSERT(TotalSectors <= ATA_MAX_LBA_28);
        }
    }
    else
    {
        USHORT Cylinders, Heads, SectorsPerTrack;

        /* Using CHS addressing mode */
        if (AtaDevIsCurrentGeometryValid(IdentifyData))
        {
            AtaDevCurrentChsTranslation(IdentifyData, &Cylinders, &Heads, &SectorsPerTrack);
        }
        else
        {
            AtaDevDefaultChsTranslation(IdentifyData, &Cylinders, &Heads, &SectorsPerTrack);
        }
        DevExt->Cylinders = Cylinders;
        DevExt->Heads = Heads;
        DevExt->SectorsPerTrack = SectorsPerTrack;

        TotalSectors = Cylinders * Heads * SectorsPerTrack;
    }

    /* The sector count can be 0 */
    if (TotalSectors == 0)
    {
        ERR("Unknown geometry\n");

        /* Fix up sector count for the READ CAPACITY command */
        TotalSectors = 1;

        /* Avoid dividing by zero in READ/WRITE commands */
        DevExt->SectorsPerTrack = 1;
        DevExt->Heads = 1;
    }

    DevExt->TotalSectors = TotalSectors;

    DevExt->SectorSize = AtaDevBytesPerLogicalSector(IdentifyData);
    ASSERT(DevExt->SectorSize >= 512);
    DevExt->SectorSize = min(DevExt->SectorSize, 512);

    INFO("Total sectors %I64u of size %lu, CHS %u:%u:%u\n",
         DevExt->TotalSectors,
         DevExt->SectorSize,
         DevExt->Cylinders,
         DevExt->Heads,
         DevExt->SectorsPerTrack);
}

static
CODE_SEG("PAGE")
VOID
AtaPdoEnableQueuedCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG QueueDepth;

    PAGED_CODE();

    QueueDepth = AtaDevQueueDepth(&DevExt->IdentifyDeviceData);
    if (QueueDepth == 0)
        return;

    DevExt->MaxQueuedSlotsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - QueueDepth);

    /* Don't set the bitmap to larger than the HBA can handle */
    DevExt->MaxQueuedSlotsBitmap &= DevExt->MaxRequestsBitmap;

    /* DevExt->Flags |= DEVICE_NCQ; */

    INFO("NCQ enabled, queue depth %lu/%lu\n",
         QueueDepth,
         ((DevExt->ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1);
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

    ASSUME((Length >= sizeof(USHORT)) && (Length % sizeof(USHORT)) == 0);

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

    while (MaxLength != 0)
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
    if (AtaDevIsSerialNumberValid(IdentifyData))
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
    INFO("SerialNumber: '%s'\n", AtaPdoIsSerialNumberValid(DevExt) ?
                                 DevExt->SerialNumber : "<INVALID>");
}

static
CODE_SEG("PAGE")
VOID
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

        if (AtaDevIsTape(&DevExt->IdentifyPacketData))
            WARN("TODO: Tape drive detected\n");
    }
    else
    {
        if (ChanExt->AhciCapabilities & AHCI_CAP_SNCQ)
            AtaPdoEnableQueuedCommands(DevExt);

        AtaPdoSetAddressingMode(DevExt);
        AtaCreateStandardInquiryData(DevExt);
    }

    if (!AtaDevIsSsd(&DevExt->IdentifyDeviceData))
    {
        /* The spindle motor requires an inrush of power at start-up */
        DevExt->Common.Self->Flags |= DO_POWER_INRUSH;
    }
    if (DevExt->InquiryData.RemovableMedia)
    {
        /* Distinguish between fixed and removable drives */
        DevExt->Common.Self->Characteristics |= FILE_REMOVABLE_MEDIA;
    }
    DevExt->Common.Self->Flags &= ~DO_DEVICE_INITIALIZING;
}

static
VOID
CODE_SEG("PAGE")
AtaPdoPrepareForAtapiEnum(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    static const CHAR Cdr260ModelId[] =  "NEC                 CD-ROM DRIVE";

    PAGED_CODE();

    DevExt->CdbSize = AtaDevCdbSizeInWords(&DevExt->IdentifyPacketData);
    if (IS_AHCI(DevExt))
        DevExt->CdbSize *= 2; // Used for data transfers to ACMD region

    if (AtaDevHasCdbInterrupt(&DevExt->IdentifyPacketData))
        DevExt->Flags |= DEVICE_HAS_CDB_INTERRUPT;
    else
        DevExt->Flags &= ~DEVICE_HAS_CDB_INTERRUPT;

    if (AtaDevIsDmaDirectionRequired(&DevExt->IdentifyPacketData))
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
VOID
AtaDeviceSetupSpinUp(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;

    PAGED_CODE();

    Request->Flags = 0;
    Request->TimeOut = 20;

    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_PUIS_SPIN_UP;
}

static
CODE_SEG("PAGE")
VOID
AtaDeviceSetupInquiry(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PCDB Cdb;

    PAGED_CODE();

    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_PACKET_COMMAND |
                     REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;
    Request->DataTransferLength = INQUIRYDATABUFFERSIZE;

    RtlZeroMemory(Request->Cdb, sizeof(Request->Cdb));

    Cdb = (PCDB)&Request->Cdb;
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = DevExt->AtaScsiAddress.Lun;
    Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaPdoSendCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    KeClearEvent(&DevExt->WorkerContext.CompletedEvent);
    AtaDeviceQueueAction(DevExt, ACTION_INTERNAL_COMMAND);
    KeWaitForSingleObject(&DevExt->WorkerContext.CompletedEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    return DevExt->WorkerContext.CommandSuccess;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaPdoInquiryDataEqual(
    _In_ PINQUIRYDATA InquiryData1,
    _In_ PINQUIRYDATA InquiryData2)
{
    PAGED_CODE();

    if (InquiryData1->DeviceType != InquiryData2->DeviceType)
        return FALSE;

    return RtlEqualMemory(InquiryData1->VendorId,
                          InquiryData2->VendorId,
                          RTL_FIELD_SIZE(INQUIRYDATA, VendorId) +
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductId) +
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel));
}

static
CODE_SEG("PAGE")
ATA_DEVICE_CLASS
AtaPdoIdentifyTargetDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ATA_DEVICE_CLASS DeviceClass;
    ULONG DiscoveryOrder, RetryCount;
    BOOLEAN StartFromAtapi;

    KeClearEvent(&DevExt->WorkerContext.EnumeratedEvent);
    AtaDeviceQueueAction(DevExt, ACTION_ENUM);
    KeWaitForSingleObject(&DevExt->WorkerContext.EnumeratedEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    if (DevExt->WorkerContext.ConnectionStatus == CONN_STATUS_NO_DEVICE)
        return DEV_UNKNOWN;

    if (DevExt->WorkerContext.ConnectionStatus == CONN_STATUS_DEV_UNKNOWN)
    {
        if (DevExt->Flags & DEVICE_ENUM)
        {
            ULONG KeyValue;

            /* Query the last device type */
            AtaGetRegistryKey(ChanExt, DevExt, L"DeviceType", &KeyValue, DEV_UNKNOWN);

            StartFromAtapi = (KeyValue == DEV_ATAPI);
        }
        else
        {
            StartFromAtapi = IS_ATAPI(DevExt);
        }
    }
    else
    {
        StartFromAtapi = (DevExt->WorkerContext.ConnectionStatus == CONN_STATUS_DEV_ATAPI);
    }

    /*
     * Try the known device type first.
     * This may speed up device detection by eliminating I/O errors.
     */
    if (StartFromAtapi)
        DiscoveryOrder = DEV_ATAPI ^ DEV_ATA;
    else
        DiscoveryOrder = 0;

    /* Look for ATA/ATAPI devices */
    for (DeviceClass = DEV_ATA; DeviceClass <= DEV_ATAPI; ++DeviceClass)
    {
        UCHAR Command;

        if ((DeviceClass ^ DiscoveryOrder) == DEV_ATA)
            Command = IDE_COMMAND_IDENTIFY;
        else
            Command = IDE_COMMAND_ATAPI_IDENTIFY;

        /* Send the identify command */
        for (RetryCount = 0; RetryCount < 2; ++RetryCount)
        {
            PIDENTIFY_DEVICE_DATA IdentifyData = DevExt->LocalBuffer;

            TRACE("Send %s identify\n", Command == IDE_COMMAND_IDENTIFY ? "ATA" : "ATAPI");

            AtaDeviceSetupIdentify(DevExt, Command);
            if (!AtaPdoSendCommand(DevExt))
                break;

            /* The drive needs to be spun up */
            if (AtaDevInPuisState(IdentifyData))
            {
                AtaDeviceSetupSpinUp(DevExt);
                (VOID)AtaPdoSendCommand(DevExt);

                /* The device was unable to return complete identity data */
                if (AtaDevIsIdentifyDataIncomplete(IdentifyData))
                    continue;
            }

            /* Verify the checksum */
            if (!AtaDevIsIdentifyDataValid(IdentifyData))
            {
                ERR("Identify data CRC error\n");
                return DEV_UNKNOWN;
            }

            /* Update identify data */
            RtlCopyMemory(&DevExt->IdentifyDeviceData,
                          IdentifyData,
                          sizeof(*IdentifyData));

            return DeviceClass ^ DiscoveryOrder;
        }
    }

    return DEV_UNKNOWN;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaPdoIdentifyLogicalDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    BOOLEAN Success;

    PAGED_CODE();

    AtaDeviceSetupInquiry(DevExt);
    Success = AtaPdoSendCommand(DevExt);

    if (Success)
    {
        RtlCopyMemory(&DevExt->InquiryData,
                      DevExt->LocalBuffer,
                      sizeof(DevExt->InquiryData));
    }

    return Success;
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

        AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_ENUM);
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
        if (AtaAcpiGetTimingMode(ChanExt, &ChanExt->PortData->Pata.TimingMode))
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

    MaxLun = 1;

    /* Detect and enumerate ATA/ATAPI devices */
    for (DeviceNumber = 0; DeviceNumber < MaxDevices; ++DeviceNumber)
    {
        if (IS_AHCI(ChanExt)) // TODO
        {
            if (!(ChanExt->PortBitmap & (1 << DeviceNumber)))
                continue;
        }

        for (Lun = 0; Lun < MaxLun; ++Lun)
        {
            PATAPORT_DEVICE_EXTENSION DevExt, OldDevExt;
            ATA_DEVICE_CLASS DeviceClass;
            ATA_SCSI_ADDRESS AtaScsiAddress;

            TRACE("Scanning tid %lu lun %lu\n", DeviceNumber, Lun);

            AtaScsiAddress = AtaMarshallScsiAddress(ChanExt->PathId, DeviceNumber, Lun);

            ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

            /* Try to find an existing device */
            OldDevExt = AtaFdoFindDeviceByPath(ChanExt, AtaScsiAddress);

            ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);

            /* Select the device extension */
            if (OldDevExt && !OldDevExt->ReportedMissing)
            {
                /* We let the running IRPs complete, then we can perform device detection */
                AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_ENUM);
                AtaReqWaitForOutstandingIoToComplete(DevExt, 0);

                DevExt = OldDevExt;
            }
            else
            {
                AtaPdoUpdateScsiAddress(EnumDevExt, AtaScsiAddress);

                DevExt = EnumDevExt;
            }

            /* Determine the type of the target device */
            if (Lun == 0)
            {
                DeviceClass = AtaPdoIdentifyTargetDevice(ChanExt, DevExt);

                INFO("Device Type %u at [tid %lu lun %lu]\n", DeviceClass, DeviceNumber, Lun);

                /* Save the new device type */
                AtaSetRegistryKey(ChanExt, DevExt, L"DeviceType", DeviceClass);

                /* Scan all logical units to support rare multi-lun ATAPI devices */
                if (DeviceClass == DEV_ATAPI)
                    MaxLun = AtaDevMaxLun(&DevExt->IdentifyPacketData);
                else
                    MaxLun = 1;

                /* No device present at this SCSI address */
                if (DeviceClass == DEV_UNKNOWN)
                {
                    /* No device present at this SCSI address */
                    DevExt->ReportedMissing = TRUE;
                    continue;
                }

                /* Check if the drive was replaced with another of the same type */
                if (!(DevExt->Flags & DEVICE_ENUM))
                {
                    if (AtaPdoGetDeviceClass(DevExt) != DeviceClass ||
                        !AtaDeviceIdentifyDataEqual(&EnumDevExt->IdentifyDeviceData,
                                                    &OldDevExt->IdentifyDeviceData))
                    {
                        DevExt->ReportedMissing = TRUE;
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
                if (DevExt->Flags & DEVICE_ENUM)
                    AtaPdoPrepareForAtapiEnum(DevExt);

                if (!AtaPdoIdentifyLogicalDevice(DevExt))
                {
                    /* No device present at this SCSI address */
                    DevExt->ReportedMissing = TRUE;
                    continue;
                }
                else if (!(DevExt->Flags & DEVICE_ENUM))
                {
                    /* Check if the device was replaced with another of the same type */
                    if (!AtaPdoInquiryDataEqual(&EnumDevExt->InquiryData,
                                                &OldDevExt->InquiryData))
                    {
                        DevExt->ReportedMissing = TRUE;
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

            /* Copy parameter information from the device or from lun 0 for ATAPI devices */
            RtlCopyMemory(&DevExt->IdentifyDeviceData,
                          &EnumDevExt->IdentifyDeviceData,
                          sizeof(EnumDevExt->IdentifyDeviceData));

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

            AtaPdoInit(ChanExt, DevExt);

            // TODO Hack
            if (!IS_AHCI(DevExt))
                DevExt->Flags |= DEVICE_PIO_ONLY;

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
        WARN("QBR request took %lu ms, %lu devices\n", QbrTimeMs, ChanExt->DeviceCount);
    else
        INFO("QBR request took %lu ms, %lu devices\n", QbrTimeMs, ChanExt->DeviceCount);
#endif

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}
