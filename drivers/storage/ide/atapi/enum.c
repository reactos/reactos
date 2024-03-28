/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA bus enumeration
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

extern const WCHAR* const AtapTargetToDeviceTypeKey[];

/* FUNCTIONS ******************************************************************/

VOID
AtaPdoSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceExtension->IdentifyDeviceData;
    USHORT Cylinders, Heads, SectorsPerTrack;
    ULONG64 TotalSectors;

    DeviceExtension->Flags &= ~(DEVICE_LBA_MODE | DEVICE_LBA48 | DEVICE_HAS_FUA);

    /* Using LBA addressing mode */
    if (IdentifyData->Capabilities.LbaSupported)
    {
        DeviceExtension->Flags |= DEVICE_LBA_MODE;

        if (AtaHas48BitAddressFeature(IdentifyData))
        {
            /* Using LBA48 addressing mode */
            TotalSectors = ((ULONG64)IdentifyData->Max48BitLBA[1] << 32) |
                           IdentifyData->Max48BitLBA[0];

            DeviceExtension->Flags |= DEVICE_LBA48;

            if (AtaHasForcedUnitAccessCommands(IdentifyData))
                DeviceExtension->Flags |= DEVICE_HAS_FUA;
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
        DeviceExtension->Cylinders = Cylinders;
        DeviceExtension->Heads = Heads;
        DeviceExtension->SectorsPerTrack = SectorsPerTrack;

        TotalSectors = Cylinders * Heads * SectorsPerTrack;
    }
    DeviceExtension->TotalSectors = TotalSectors;

    DeviceExtension->SectorSize = AtaBytesPerLogicalSector(IdentifyData);

    ASSERT(DeviceExtension->TotalSectors != 0);
    ASSERT(DeviceExtension->SectorSize != 0);

    ERR("Total sectors %I64u of size %lu\n",
        DeviceExtension->TotalSectors,
        DeviceExtension->SectorSize);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoSet32IoMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PATA_TASKFILE TaskFile = &ChannelExtension->TaskFile[SRB_TYPE_ENUM];
    PIDENTIFY_DEVICE_DATA IdentifyData;

    PAGED_CODE();

    if (ChannelExtension->Flags & CHANNEL_IO_MODE_DETECTED)
        return;

#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
        return;
#endif

    IdentifyData = ExAllocatePoolUninitialized(NonPagedPool, sizeof(*IdentifyData), IDEPORT_TAG);
    if (!IdentifyData)
        return;

    /* Switch the controller over to 32-bit I/O mode */
    ChannelExtension->Flags |= CHANNEL_IO32;

    RtlZeroMemory(TaskFile, sizeof(*TaskFile));

    /* Send the identify command */
    if (IS_ATAPI(DeviceExtension))
        TaskFile->Command = IDE_COMMAND_ATAPI_IDENTIFY;
    else
        TaskFile->Command = IDE_COMMAND_IDENTIFY;

    AtaSendTaskFile(DeviceExtension,
                    TaskFile,
                    SRB_TYPE_ENUM,
                    TRUE,
                    4,
                    IdentifyData,
                    sizeof(*IdentifyData));

    /* Check if the controller is able to translate a 32-bit I/O cycle into two 16-bit cycles */
    if (RtlEqualMemory(IdentifyData, &DeviceExtension->IdentifyDeviceData, sizeof(*IdentifyData)))
    {
        INFO("32-bit I/O supported\n");
        goto Exit;
    }

    /* 32-bit I/O not supported */
    ChannelExtension->Flags &= ~CHANNEL_IO32;

Exit:
    ChannelExtension->Flags |= CHANNEL_IO_MODE_DETECTED;

    ExFreePoolWithTag(IdentifyData, IDEPORT_TAG);
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaPdoIsSuperFloppy(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    return (strstr(DeviceExtension->FriendlyName, " LS-120") ||
            strstr(DeviceExtension->FriendlyName, " LS-240"));
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    AtaFdoSet32IoMode(ChannelExtension, DeviceExtension);

    if (IS_ATAPI(DeviceExtension))
    {
        if (AtaPdoIsSuperFloppy(DeviceExtension))
            DeviceExtension->Flags |= DEVICE_IS_SUPER_FLOPPY;
    }
    else
    {
        AtaPdoSetAddressingMode(DeviceExtension);
    }

    return STATUS_SUCCESS;
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
ULONG
AtaChecksum(
    _In_ ULONG Crc,
    _In_reads_bytes_(Length) const VOID *Buffer,
    _In_ ULONG Length)
{
    const UCHAR *Data = Buffer;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < Length; ++i)
    {
        Crc += *Data++;
        Crc = _rotl(Crc, 7);
    }

    return Crc;
}

static
CODE_SEG("PAGE")
ULONG
AtaPdoBuildDeviceSignature(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Crc;

    PAGED_CODE();

    Crc = 0;
    Crc = AtaChecksum(Crc,
                      DeviceExtension->IdentifyDeviceData.SerialNumber,
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber));
    Crc = AtaChecksum(Crc,
                      DeviceExtension->IdentifyDeviceData.FirmwareRevision,
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, FirmwareRevision) +
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber));
    return Crc;
}

static
CODE_SEG("PAGE")
VOID
AtaPdoFillIdentificationStrings(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceExtension->IdentifyDeviceData;
    PCHAR End;

    PAGED_CODE();

    /*
     * For ATAPI devices Inquiry Data is preferred over Identify Data.
     * ATA strings are *not* byte-swapped for early ATAPI drives (NEC CDR-260, etc.).
     */
    if (IS_ATAPI(DeviceExtension))
    {
        /* Combine VendorId and ProductId, separated by a space */
        End = AtaCopyIdString(DeviceExtension->FriendlyName,
                              DeviceExtension->InquiryData.VendorId,
                              8,
                              ' ');
        End = AtaTrimIdString(DeviceExtension->FriendlyName, End);
        *End++ = ' ';
        End = AtaCopyIdString(End,
                              DeviceExtension->InquiryData.ProductId,
                              16,
                              ' ');
        AtaTrimIdString(DeviceExtension->FriendlyName, End);

        /* Copy ProductRevisionLevel */
        End = AtaCopyIdString(DeviceExtension->RevisionNumber,
                              DeviceExtension->InquiryData.ProductRevisionLevel,
                              4,
                              ' ');
        AtaTrimIdString(DeviceExtension->RevisionNumber, End);
    }
    else
    {
        /* Copy ModelNumber from a byte-swapped ATA string */
        End = AtaCopyIdString(DeviceExtension->FriendlyName,
                              IdentifyData->ModelNumber,
                              ATAPORT_FN_FIELD,
                              ' ');
        AtaSwapIdString(DeviceExtension->FriendlyName, ATAPORT_FN_FIELD / 2);
        AtaTrimIdString(DeviceExtension->FriendlyName, End);

        /* Copy FirmwareRevision from a byte-swapped ATA string */
        End = AtaCopyIdString(DeviceExtension->RevisionNumber,
                              IdentifyData->FirmwareRevision,
                              ATAPORT_RN_FIELD,
                              ' ');
        AtaSwapIdString(DeviceExtension->RevisionNumber, ATAPORT_RN_FIELD / 2);
        AtaTrimIdString(DeviceExtension->RevisionNumber, End);
    }

    /* Format the serial number */
    if (AtaDeviceSerialNumberValid(IdentifyData))
    {
        ULONG i;
        NTSTATUS Status;
        size_t Remaining;

        End = DeviceExtension->SerialNumber;
        Remaining = sizeof(DeviceExtension->SerialNumber);

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

    INFO("FriendlyName: '%s'\n", DeviceExtension->FriendlyName);
    INFO("RevisionNumber: '%s'\n", DeviceExtension->RevisionNumber);
    INFO("SerialNumber: '%s'\n", AtaPdoSerialNumberValid(DeviceExtension) ?
                                 DeviceExtension->SerialNumber : "<INVALID>");
}

static
CODE_SEG("PAGE")
VOID
AtaCreateStandardInquiryData(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN IsAtaDevice)
{
    PINQUIRYDATA InquiryData = &DeviceExtension->InquiryData;
    PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceExtension->IdentifyDeviceData;
    ULONG Offset;
    PUCHAR VersionDescriptor;

    PAGED_CODE();

    /* Convert IDE identify data into SCSI identify data */

    if (!IsAtaDevice)
    {
        InquiryData->DeviceType = READ_ONLY_DIRECT_ACCESS_DEVICE;
        InquiryData->RemovableMedia = AtaDeviceIsRemovable(IdentifyData);
        InquiryData->AdditionalLength =
            INQUIRYDATABUFFERSIZE - RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

        /* Copy ModelNumber from a byte-swapped ATA string */
        AtaCopyIdString((PCHAR)InquiryData->VendorId,
                        IdentifyData->ModelNumber,
                        sizeof(IdentifyData->ModelNumber),
                        ' ');
        AtaSwapIdString(InquiryData->VendorId, sizeof(IdentifyData->ModelNumber) / 2);

        /* Copy FirmwareRevision from a byte-swapped ATA string */
        AtaCopyIdString((PCHAR)InquiryData->ProductRevisionLevel,
                        IdentifyData->FirmwareRevision,
                        sizeof(InquiryData->ProductRevisionLevel),
                        ' ');
        AtaSwapIdString(InquiryData->ProductRevisionLevel,
                        sizeof(InquiryData->ProductRevisionLevel) / 2);
        return;
    }

    InquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    InquiryData->RemovableMedia = AtaDeviceIsRemovable(IdentifyData);
    InquiryData->Versions = 0x07; // SPC-5
    InquiryData->ResponseDataFormat = 2; // "Complies to this standard"
    /* InquiryData->CommandQueue = 1; TODO */

    /* T10 vendor ID */
    RtlCopyMemory(InquiryData->VendorId, "ATA     ", RTL_FIELD_SIZE(INQUIRYDATA, VendorId));

    /* Product ID */
    AtaCopyAtaString(InquiryData->ProductId,
                     IdentifyData->ModelNumber,
                     RTL_FIELD_SIZE(INQUIRYDATA, ProductId));

    /* Product revision level */
    if (IdentifyData->FirmwareRevision[4] == ' ' &&
        IdentifyData->FirmwareRevision[5] == ' ' &&
        IdentifyData->FirmwareRevision[6] == ' ' &&
        IdentifyData->FirmwareRevision[7] == ' ')
    {
        Offset = 0;
    }
    else
    {
        Offset = 4;
    }
    AtaCopyAtaString(InquiryData->ProductRevisionLevel,
                     &IdentifyData->FirmwareRevision[Offset],
                     RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel));

    VersionDescriptor = &InquiryData->Reserved3[3];

    /* SAM-5 (no version claimed) */
    *VersionDescriptor++ = 0x00;
    *VersionDescriptor++ = 0xA0;

    /* SPC-5 (no version claimed) */
    *VersionDescriptor++ = 0x05;
    *VersionDescriptor++ = 0xC0;

    /* SBC-4 (no version claimed) */
    *VersionDescriptor++ = 0x06;
    *VersionDescriptor++ = 0x00;

    if (AtaDeviceIsZonedDevice(IdentifyData))
    {
        /* ZBC BSR INCITS 536 revision 05 */
        *VersionDescriptor++ = 0x06;
        *VersionDescriptor++ = 0x24;
    }

    if (AtaDeviceHasIeee1667(IdentifyData))
    {
        /* IEEE 1667 (no version claimed) */
        *VersionDescriptor++ = 0xFF;
        *VersionDescriptor++ = 0xC0;
    }

    if (IdentifyData->MajorRevision != 0xFFFF)
    {
        if (IdentifyData->MajorRevision & (1 << 8))
        {
            /* ATA/ATAPI-8 ATA8-ACS ATA/ATAPI Command Set (no version claimed) */
            *VersionDescriptor++ = 0x16;
            *VersionDescriptor++ = 0x23;
        }
        else if (IdentifyData->MajorRevision & (1 << 7))
        {
            if (IdentifyData->MinorRevision == 0x1D)
            {
                /* ATA/ATAPI-7 INCITS 397-2005 */
                *VersionDescriptor++ = 0x16;
                *VersionDescriptor++ = 0x1C;
            }
            else
            {
                /* ATA/ATAPI-7 (no version claimed) */
                *VersionDescriptor++ = 0x16;
                *VersionDescriptor++ = 0x00;
            }
        }
        else if (IdentifyData->MajorRevision & (1 << 6))
        {
            if (IdentifyData->MinorRevision == 0x22)
            {
                /* ATA/ATAPI-6 INCITS 361-2002 */
                *VersionDescriptor++ = 0x15;
                *VersionDescriptor++ = 0xFD;
            }
            else
            {
                /* ATA/ATAPI-6 (no version claimed) */
                *VersionDescriptor++ = 0x15;
                *VersionDescriptor++ = 0xE0;
            }
        }
    }

    InquiryData->AdditionalLength =
        ((ULONG_PTR)VersionDescriptor - (ULONG_PTR)InquiryData) -
        RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

    TRACE("VendorId: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, VendorId), InquiryData->VendorId);
    TRACE("ProductId: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, ProductId), InquiryData->ProductId);
    TRACE("ProductRevisionLevel: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel), InquiryData->ProductRevisionLevel);
}

static
CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaPdoCreateDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PATAPORT_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT Pdo;
    ULONG Alignment;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\IdeDeviceP999T9L9-FFF")];
    static ULONG PdoNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\IdeDeviceP%luT%luL%lu-%lx",
                                ChannelExtension->ChannelNumber,
                                AtaScsiAddress.TargetId,
                                AtaScsiAddress.Lun,
                                PdoNumber++);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(ChannelExtension->Common.Self->DriverObject,
                            sizeof(*DeviceExtension),
                            &DeviceName,
                            FILE_DEVICE_MASS_STORAGE,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create PDO 0x%lx\n", Status);
        return NULL;
    }

    INFO("Created device object %p '%wZ'\n", Pdo, &DeviceName);

    /* DMA buffers alignment */
    Alignment = ChannelExtension->Common.Self->AlignmentRequirement;
    Pdo->AlignmentRequirement = max(Alignment, FILE_WORD_ALIGNMENT);

    Pdo->Flags |= DO_DIRECT_IO;

    DeviceExtension = Pdo->DeviceExtension;

    RtlZeroMemory(DeviceExtension, sizeof(*DeviceExtension));
    DeviceExtension->Common.Self = Pdo;
    DeviceExtension->Common.Signature = ATAPORT_PDO_SIGNATURE;
    DeviceExtension->ChannelExtension = ChannelExtension;
    DeviceExtension->AtaScsiAddress = AtaScsiAddress;
    DeviceExtension->SectorSize = 512;

    KeInitializeSpinLock(&DeviceExtension->QueueLock);
    InitializeListHead(&DeviceExtension->RequestList);
    IoInitializeRemoveLock(&DeviceExtension->RemoveLock, IDEPORT_TAG, 0, 0);

    return DeviceExtension;
}

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_DEVICE_EXTENSION DeviceExtension, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;

    PAGED_CODE();

    ExAcquireFastMutex(&ChannelExtension->DeviceSyncMutex);

    for (Entry = ChannelExtension->DeviceList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        DeviceExtension = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if ((DeviceExtension->AtaScsiAddress.AsULONG == AtaScsiAddress.AsULONG) &&
            !DeviceExtension->ReportedMissing)
        {
            Result = DeviceExtension;
            break;
        }
    }

    ExReleaseFastMutex(&ChannelExtension->DeviceSyncMutex);

    return Result;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PATAPORT_DEVICE_EXTENSION CurrentDeviceExtension;
    PSINGLE_LIST_ENTRY Entry, PrevEntry;
    ATA_SCSI_ADDRESS Address = DeviceExtension->AtaScsiAddress;

    PAGED_CODE();

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

    /* The device list is ordered by SCSI address, smallest first */
    if (PrevEntry)
    {
        /* Insert before the current entry */
        DeviceExtension->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = &DeviceExtension->ListEntry;
    }
    else
    {
        /* Insert in the beginning */
        PushEntryList(&ChannelExtension->DeviceList, &DeviceExtension->ListEntry);
    }

    ExReleaseFastMutex(&ChannelExtension->DeviceSyncMutex);
}

static
PATAPORT_DEVICE_EXTENSION
CODE_SEG("PAGE")
AtaFdoCreateEnumDeviceExtension(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATAPORT_DEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    DeviceExtension = ExAllocatePoolZero(NonPagedPool, sizeof(*DeviceExtension), IDEPORT_TAG);
    if (!DeviceExtension)
        return NULL;

    DeviceExtension->Flags |= DEVICE_ENUM | DEVICE_PIO_ONLY;
    DeviceExtension->ChannelExtension = ChannelExtension;
    DeviceExtension->SectorSize = 512;

    KeInitializeSpinLock(&DeviceExtension->QueueLock);
    InitializeListHead(&DeviceExtension->RequestList);

    return DeviceExtension;
}

static
VOID
CODE_SEG("PAGE")
AtaFdoPrepareEnumDeviceExtension(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    static const CHAR Cdr260Model[] =  "NEC                 CD-ROM DRIVE";

    PAGED_CODE();

    if (AtaDeviceHasCdbInterrupt(&DeviceExtension->IdentifyPacketData))
    {
        DeviceExtension->Flags |= DEVICE_HAS_CDB_INTERRUPT;
    }
    else
    {
        DeviceExtension->Flags &= ~DEVICE_HAS_CDB_INTERRUPT;
    }

    if (RtlEqualMemory(DeviceExtension->IdentifyPacketData.ModelNumber,
                       Cdr260Model,
                       sizeof(Cdr260Model) - 1))
    {
        DeviceExtension->Flags |= DEVICE_IS_NEC_CDR260;
    }
    else
    {
        DeviceExtension->Flags &= ~DEVICE_IS_NEC_CDR260;
    }

    DeviceExtension->CdbSize = AtaDeviceCdbSizeInWords(&DeviceExtension->IdentifyPacketData);
}

CODE_SEG("PAGE")
NTSTATUS
AtaSendTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PATA_TASKFILE TaskFile,
    _In_ ATA_SRB_TYPE SrbType,
    _In_ BOOLEAN DoPoll,
    _In_ ULONG TimeoutInSeconds,
    _In_opt_ PVOID Buffer,
    _In_ ULONG Length)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeviceExtension->ChannelExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    KEVENT Event;

    PAGED_CODE();

    AtaScsiAddress = DeviceExtension->AtaScsiAddress;

    Srb = &ChannelExtension->InternalSrb[SrbType];
    Srb->PathId = AtaScsiAddress.PathId;
    Srb->TargetId = AtaScsiAddress.TargetId;
    Srb->Lun = AtaScsiAddress.Lun;
    Srb->DataBuffer = Buffer;
    Srb->DataTransferLength = Length;
    Srb->TimeOutValue = TimeoutInSeconds;
    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->SrbFlags = Buffer ? SRB_FLAGS_DATA_IN : 0;
    Srb->SenseInfoBuffer = NULL;
    Srb->SenseInfoBufferLength = 0;
    Srb->SrbExtension = (PVOID)SRB_FLAG_INTERNAL;
    if (DoPoll)
    {
        SRB_SET_FLAG(Srb, SRB_FLAG_POLL);
    }

    Irp = Srb->OriginalRequest;
    Irp->UserBuffer = TaskFile;
    Irp->UserEvent = &Event;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    AtaPdoStartSrb(DeviceExtension, Srb);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS)
        return STATUS_SUCCESS;

    return STATUS_IO_DEVICE_ERROR;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaSendInquiry(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    KEVENT Event;
    PCDB Cdb;
    PSENSE_DATA SenseData;

    PAGED_CODE();

    AtaScsiAddress = DeviceExtension->AtaScsiAddress;

    Srb = &ChannelExtension->InternalSrb[SRB_TYPE_ENUM];
    Srb->PathId = AtaScsiAddress.PathId;
    Srb->TargetId = AtaScsiAddress.TargetId;
    Srb->Lun = AtaScsiAddress.Lun;
    Srb->DataBuffer = &DeviceExtension->InquiryData;
    Srb->DataTransferLength = INQUIRYDATABUFFERSIZE;
    Srb->SrbExtension = (PVOID)(SRB_FLAG_INTERNAL | SRB_FLAG_PACKET_COMMAND);
    Srb->TimeOutValue = 10;
    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->SrbFlags = SRB_FLAGS_DATA_IN;
    Srb->CdbLength = 6;

    Cdb = (PCDB)&Srb->Cdb;
    RtlZeroMemory(Cdb, 12);
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = AtaScsiAddress.Lun;
    Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    /* Pass a sense buffer to identify broken Xbox CD/DVD drives */
    SenseData = &ChannelExtension->SenseData;
    RtlZeroMemory(SenseData, sizeof(*SenseData));
    Srb->SenseInfoBuffer = SenseData;
    Srb->SenseInfoBufferLength = sizeof(*SenseData);

    Irp = Srb->OriginalRequest;
    Irp->UserEvent = &Event;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    AtaPdoStartSrb(DeviceExtension, Srb);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    ASSERT(Srb->SrbStatus != SRB_STATUS_DATA_OVERRUN);

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS)
        return STATUS_SUCCESS;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
    {
        ERR("Inquiry failed, SK 0x%x, ASC 0x%x, ASCQ 0x%x\n",
            SenseData->SenseKey,
            SenseData->AdditionalSenseCode,
            SenseData->AdditionalSenseCodeQualifier);

        /*
         * The INQUIRY command is mandatory to be implemented by ATAPI devices,
         * but some Xbox drives violate this.
         */
        if ((Srb->Lun == 0) &&
            (SenseData->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST))
        {
            if (0) // TODO IsXboxDrive
            {
                /* Simulate inquiry data for ATAPI devices */
                AtaCreateStandardInquiryData(DeviceExtension, FALSE);
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
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PATA_TASKFILE TaskFile = &ChannelExtension->TaskFile[SRB_TYPE_ENUM];
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(TaskFile, sizeof(*TaskFile));
    TaskFile->Command = IDE_COMMAND_SET_FEATURE;
    TaskFile->Feature = IDE_FEATURE_PUIS_SPIN_UP;

    Status = AtaSendTaskFile(DeviceExtension, TaskFile, SRB_TYPE_ENUM, FALSE, 20, NULL, 0);

    TRACE("PUIS status 0x%lx\n", Status);
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaFdoIdentifyDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ PATAPORT_DEVICE_EXTENSION OldDeviceExtension)
{
    ATA_DEVICE_CLASS DeviceClass;
    ULONG DiscoveryOrder;
    BOOLEAN StartFromAtapi;
    NTSTATUS Status;

    PAGED_CODE();

    if (OldDeviceExtension)
    {
        StartFromAtapi = IS_ATAPI(OldDeviceExtension);
    }
    else
    {
        ULONG KeyValue;

        /* Query the last device type */
        AtaGetRegistryKey(ChannelExtension,
                          DeviceExtension,
                          AtapTargetToDeviceTypeKey[DeviceExtension->AtaScsiAddress.TargetId],
                          &KeyValue,
                          DEV_UNKNOWN);
        StartFromAtapi = (KeyValue == DEV_ATAPI);
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
        PATA_TASKFILE TaskFile = &ChannelExtension->TaskFile[SRB_TYPE_ENUM];

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));

        /* Send the identify command */
        if ((DeviceClass ^ DiscoveryOrder) == DEV_ATA)
            TaskFile->Command = IDE_COMMAND_IDENTIFY;
        else
            TaskFile->Command = IDE_COMMAND_ATAPI_IDENTIFY;

        /*
         * Disable interrupts for the identify command, use polling instead:
         *
         * - On some single device 1 configurations or non-existent IDE channels
         *   the status register stuck permanently at a value of 0,
         *   and we incorrectly assume that the device 0 is present.
         *   This will result in a taskfile timeout
         *   which must be avoided as it would cause hangs at boot time.
         *
         * - The NEC CDR-260 drive uses the Packet Command protocol (?) for this command,
         *   which causes a spurious (completion status) interrupt after reading the data buffer.
         */
        Status = AtaSendTaskFile(DeviceExtension,
                                 TaskFile,
                                 SRB_TYPE_ENUM,
                                 TRUE,
                                 15,
                                 &DeviceExtension->IdentifyDeviceData,
                                 sizeof(DeviceExtension->IdentifyDeviceData));
        if (NT_SUCCESS(Status))
            return DeviceClass ^ DiscoveryOrder;
    }

    return DEV_UNKNOWN;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp)
{
    PATAPORT_DEVICE_EXTENSION EnumDeviceExtension, DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG DeviceNumber, MaxDevices, Lun, MaxLun, MaxPdoCount;
    PSINGLE_LIST_ENTRY Entry;
    NTSTATUS Status;
#if DBG
    LARGE_INTEGER TimeStart, TimeFinish;
    ULONG QbrTimeMs;
#endif

    PAGED_CODE();

#if DBG
    KeQuerySystemTime(&TimeStart);
#endif

    /* Get the channel timing information */
    if (AtaAcpiGetTimingMode(ChannelExtension, &ChannelExtension->TimingMode))
        ChannelExtension->Flags |= CHANNEL_HAS_GTM;
    else
        ChannelExtension->Flags &= ~CHANNEL_HAS_GTM;

    /* Allocate a dummy PDO extension to service I/O requests during device discovery */
    EnumDeviceExtension = AtaFdoCreateEnumDeviceExtension(ChannelExtension);
    if (!EnumDeviceExtension)
    {
        ERR("Failed to allocate PDO extension\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MaxDevices = AtaFdoMaxDeviceCount(ChannelExtension);
    MaxLun = 1;

    /*
     * Preallocate the device relations structure we need for the QBR IRP,
     * because we don't want to be in a state where allocation fails.
     */
    MaxPdoCount = MaxDevices * ATA_MAX_LUN_COUNT;
    DeviceRelations = ExAllocatePoolUninitialized(PagedPool,
                                                  FIELD_OFFSET(DEVICE_RELATIONS,
                                                               Objects[MaxPdoCount]),
                                                  IDEPORT_TAG);
    if (!DeviceRelations)
    {
        ERR("Failed to allocate device relations\n");

        ExFreePoolWithTag(EnumDeviceExtension, IDEPORT_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Detect and enumerate ATA/ATAPI devices */
    for (DeviceNumber = 0; DeviceNumber < MaxDevices; ++DeviceNumber)
    {
        for (Lun = 0; Lun < MaxLun; ++Lun)
        {
            PATAPORT_DEVICE_EXTENSION OldDeviceExtension;
            ATA_DEVICE_CLASS DeviceClass;
            ULONG Signature, RetryCount;
            ATA_SCSI_ADDRESS AtaScsiAddress;

            TRACE("Scanning target %lu lun %lu\n", DeviceNumber, Lun);

            AtaScsiAddress = AtaMarshallScsiAddress(ChannelExtension->PathId, DeviceNumber, Lun);

            /* Try to find an existing device */
            OldDeviceExtension = AtaFdoFindDeviceByPath(ChannelExtension, AtaScsiAddress);

            /* Unrecoverable error, this device need to go away */
            if (OldDeviceExtension && (OldDeviceExtension->Flags & DEVICE_HARDWARE_ERROR))
            {
                OldDeviceExtension->ReportedMissing = TRUE;
                continue;
            }

            EnumDeviceExtension->AtaScsiAddress = AtaScsiAddress;
            EnumDeviceExtension->DeviceHead = ((AtaScsiAddress.TargetId & 1) << 4) |
                                              IDE_DRIVE_SELECT;
#if defined(_M_IX86)
            /* DeviceNumber is encoded in bits 3:0 for internal use only (PC-98 support) */
            EnumDeviceExtension->DeviceHead |= DeviceNumber;
#endif

            /* Determine the type of the target device */
            if (Lun == 0)
            {
                for (RetryCount = 0; RetryCount < 2; ++RetryCount)
                {
                    DeviceClass = AtaFdoIdentifyDevice(ChannelExtension,
                                                       EnumDeviceExtension,
                                                       AtaScsiAddress,
                                                       OldDeviceExtension);

                    /* The device was unable to return complete response data */
                    if ((DeviceClass != DEV_UNKNOWN) &&
                        AtaDeviceInPuisState(&EnumDeviceExtension->IdentifyDeviceData))
                    {
                        /* The drive needs to be spun up */
                        AtaFdoIdentifySpinUpPuisDevice(ChannelExtension, EnumDeviceExtension);
                        continue;
                    }

                    break;
                }
                INFO("Device Type %u at [tid %lu lun %lu]\n", DeviceClass, DeviceNumber, Lun);

                /* Scan all logical units to support rare multi-lun ATAPI devices */
                if (DeviceClass == DEV_ATAPI)
                    MaxLun = AtaDeviceMaxLun(&EnumDeviceExtension->IdentifyPacketData);
                else
                    MaxLun = 1;
            }
            else
            {
                DeviceClass = DEV_ATAPI;
            }

            /* Send the INQUIRY command for each logical unit */
            if (DeviceClass == DEV_ATAPI)
            {
                /* Prepare the device extension for the I/O request */
                AtaFdoPrepareEnumDeviceExtension(ChannelExtension, EnumDeviceExtension);

                Status = AtaSendInquiry(ChannelExtension, EnumDeviceExtension);
                if (!NT_SUCCESS(Status))
                {
                    DeviceClass = DEV_UNKNOWN;
                }
            }

            /* Save the new device type */
            if (Lun == 0)
            {
                AtaSetRegistryKey(ChannelExtension,
                                  EnumDeviceExtension,
                                  AtapTargetToDeviceTypeKey[DeviceNumber],
                                  DeviceClass);
            }

            /* No device present at this SCSI address */
            if (DeviceClass == DEV_UNKNOWN)
            {
                if (OldDeviceExtension)
                {
                    OldDeviceExtension->ReportedMissing = TRUE;
                }

                continue;
            }

            /* Check if the drive was replaced with another of the same type */
            Signature = AtaPdoBuildDeviceSignature(EnumDeviceExtension);
            if (OldDeviceExtension)
            {
                if (AtaPdoGetDeviceClass(OldDeviceExtension) != DeviceClass ||
                    OldDeviceExtension->Signature != Signature)
                {
                    OldDeviceExtension->ReportedMissing = TRUE;
                }
                else
                {
                    continue;
                }
            }

            /*
             * At this point, we assume that the drive is a new device,
             * allocate a PDO instance, and initialize it.
             */
            DeviceExtension = AtaPdoCreateDevice(ChannelExtension, AtaScsiAddress);
            if (!DeviceExtension)
            {
                /* We are out of memory, trying to continue process the QBR IRP anyway */
                continue;
            }

            DeviceExtension->Signature = Signature;

            /* Copy parameter information from the device */
            RtlCopyMemory(&DeviceExtension->IdentifyDeviceData,
                          &EnumDeviceExtension->IdentifyDeviceData,
                          sizeof(DeviceExtension->IdentifyDeviceData));

            if (DeviceClass == DEV_ATAPI)
            {
                /* Copy inquiry data from the device */
                RtlCopyMemory(&DeviceExtension->InquiryData,
                              &EnumDeviceExtension->InquiryData,
                              sizeof(DeviceExtension->InquiryData));

                DeviceExtension->Flags |= DEVICE_IS_ATAPI;

                /* Copy the transport flags */
                DeviceExtension->Flags |=
                    EnumDeviceExtension->Flags & (DEVICE_HAS_CDB_INTERRUPT |
                                                  DEVICE_IS_NEC_CDR260);

                DeviceExtension->CdbSize = EnumDeviceExtension->CdbSize;
            }

            DeviceExtension->DeviceHead = EnumDeviceExtension->DeviceHead;

            /* The spindle motor requires an inrush of power at start-up */
            if (!AtaDeviceIsSsd(&DeviceExtension->IdentifyDeviceData))
            {
                DeviceExtension->Common.Self->Flags |= DO_POWER_INRUSH;
            }
            DeviceExtension->Common.Self->Flags &= ~DO_DEVICE_INITIALIZING;

            AtaPdoFillIdentificationStrings(DeviceExtension);

            /* Simulate inquiry data for ATA devices */
            if (DeviceClass == DEV_ATA)
            {
                AtaCreateStandardInquiryData(DeviceExtension, TRUE);
            }

            ChannelExtension->Device[DeviceNumber] = DeviceExtension;

            /* Finish initializing the drive */
            Status = AtaPdoInit(ChannelExtension, DeviceExtension);
            if (!NT_SUCCESS(Status))
            {
                IoDeleteDevice(DeviceExtension->Common.Self);
                continue;
            }

            DeviceExtension->Flags |= DEVICE_PIO_ONLY;

            AtaFdoDeviceListInsert(ChannelExtension, DeviceExtension);
        }
    }

    ExFreePoolWithTag(EnumDeviceExtension, IDEPORT_TAG);

    ExAcquireFastMutex(&ChannelExtension->DeviceSyncMutex);

    /* Initialize the device relations structure */
    DeviceRelations->Count = 0;
    for (Entry = ChannelExtension->DeviceList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        DeviceExtension = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);
        if (DeviceExtension->ReportedMissing)
            continue;

        DeviceRelations->Objects[DeviceRelations->Count++] = DeviceExtension->Common.Self;
        ObReferenceObject(DeviceExtension->Common.Self);
    }
    ChannelExtension->DeviceCount = DeviceRelations->Count;
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    ExReleaseFastMutex(&ChannelExtension->DeviceSyncMutex);

#if DBG
    KeQuerySystemTime(&TimeFinish);

    QbrTimeMs = (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000;

    if (QbrTimeMs >= 5000)
        WARN("QBR request took %lu ms\n", QbrTimeMs);
    else
        INFO("QBR request took %lu ms\n", QbrTimeMs);
#endif

    return STATUS_SUCCESS;
}
