/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA bus enumeration
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

PUCHAR
AtaCopyIdStringUnsafe(
    _Out_writes_bytes_all_(Length) PUCHAR Destination,
    _In_reads_bytes_(Length) PUCHAR Source,
    _In_ ULONG Length)
{
    ULONG i;

    ASSUME((Length >= sizeof(USHORT)) && (Length % sizeof(USHORT)) == 0);

    /* Copy the ATA string and swap it */
    for (i = 0; i < Length; i += sizeof(USHORT))
    {
        Destination[i] = Source[i + 1];
        Destination[i + 1] = Source[i];
    }

    return &Destination[i - 1];
}

PCHAR
AtaCopyIdStringSafe(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter)
{
    PCHAR Dest = Destination;

    PAGED_CODE();

    while (MaxLength != 0)
    {
        const UCHAR Char = *Source;

        /* Only characters from space to tilde are allowed in an ID */
        if (Char > ' ' && Char <= '~' && Char != ',')
            *Dest = Char;
        else
            *Dest = DefaultCharacter;

        ++Source;
        ++Dest;
        --MaxLength;
    }

    return Dest;
}

VOID
AtaSwapIdString(
    _Inout_updates_bytes_(WordCount * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG WordCount)
{
    PUSHORT Word = Buffer;

    /* The buffer should be USHORT aligned for ARM compatibility */
    ASSERT(((ULONG_PTR)Buffer & 1) == 0);

    while (WordCount--)
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
    ULONG i;
    NTSTATUS Status;
    size_t Remaining;

    PAGED_CODE();

    /*
     * For ATAPI devices inquiry data is preferred over identify data.
     * ATA strings are *not* byte-swapped for early ATAPI drives (NEC CDR-260, etc.).
     */
    if (IS_ATAPI(&DevExt->Device))
    {
        /* Combine VendorId and ProductId, separated by a space */
        End = AtaCopyIdStringSafe(DevExt->FriendlyName,
                                  DevExt->InquiryData.VendorId,
                                  RTL_FIELD_SIZE(INQUIRYDATA, VendorId),
                                  ' ');
        End = AtaTrimIdString(DevExt->FriendlyName, End);
        *End++ = ' ';
        End = AtaCopyIdStringSafe(End,
                                  DevExt->InquiryData.ProductId,
                                  RTL_FIELD_SIZE(INQUIRYDATA, ProductId),
                                  ' ');
        AtaTrimIdString(DevExt->FriendlyName, End);

        /* Copy ProductRevisionLevel */
        End = AtaCopyIdStringSafe(DevExt->RevisionNumber,
                                  DevExt->InquiryData.ProductRevisionLevel,
                                  RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel),
                                  ' ');
        AtaTrimIdString(DevExt->RevisionNumber, End);
    }
    else
    {
        /* Copy ModelNumber from a byte-swapped ATA string */
        End = AtaCopyIdStringSafe(DevExt->FriendlyName,
                                  IdentifyData->ModelNumber,
                                  ATAPORT_FN_FIELD,
                                  ' ');
        AtaSwapIdString(DevExt->FriendlyName, ATAPORT_FN_FIELD / 2);
        AtaTrimIdString(DevExt->FriendlyName, End);

        /* Copy FirmwareRevision from a byte-swapped ATA string */
        End = AtaCopyIdStringSafe(DevExt->RevisionNumber,
                                  IdentifyData->FirmwareRevision,
                                  ATAPORT_RN_FIELD,
                                  ' ');
        AtaSwapIdString(DevExt->RevisionNumber, ATAPORT_RN_FIELD / 2);
        AtaTrimIdString(DevExt->RevisionNumber, End);
    }

    End = DevExt->SerialNumber;
    Remaining = sizeof(DevExt->SerialNumber);

    /* Format the serial number */
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

    INFO("FriendlyName: '%s'\n", DevExt->FriendlyName);
    INFO("RevisionNumber: '%s'\n", DevExt->RevisionNumber);
    INFO("SerialNumber: '%s'\n", DevExt->SerialNumber);
}

VOID
AtaDeviceSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG64 TotalSectors;

    DevExt->Device.DeviceFlags &= ~(DEVICE_LBA_MODE | DEVICE_LBA48 | DEVICE_HAS_FUA);

    /* Using LBA addressing mode */
    if (AtaDevHasLbaTranslation(IdentifyData))
    {
        DevExt->Device.DeviceFlags |= DEVICE_LBA_MODE;

        if (AtaDevHas48BitAddressFeature(IdentifyData))
        {
            /* Using LBA48 addressing mode */
            TotalSectors = AtaDevUserAddressableSectors48Bit(IdentifyData);
            ASSERT(TotalSectors <= ATA_MAX_LBA_48);

            DevExt->Device.DeviceFlags |= DEVICE_LBA48;

            if (AtaDevHasForceUnitAccessCommands(IdentifyData))
                DevExt->Device.DeviceFlags |= DEVICE_HAS_FUA;
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
        DevExt->Device.Cylinders = Cylinders;
        DevExt->Device.Heads = Heads;
        DevExt->Device.SectorsPerTrack = SectorsPerTrack;

        TotalSectors = UInt32x32To64(Cylinders, Heads) * SectorsPerTrack;
    }

    /*
     * The sector count can be 0 on faulty devices. It's better to keep
     * them available in the system to allow
     * the user to send any command to the drive through the pass-through interface.
     */
    if (TotalSectors == 0)
    {
        ERR("Unknown geometry\n");

        /* Fix up sector count for the READ CAPACITY command */
        TotalSectors = 1;

        /* Avoid dividing by zero in READ/WRITE commands */
        DevExt->Device.SectorsPerTrack = 1;
        DevExt->Device.Heads = 1;
    }

    DevExt->Device.TotalSectors = TotalSectors;

    DevExt->Device.SectorSize = AtaDevBytesPerLogicalSector(IdentifyData);
    ASSERT(DevExt->Device.SectorSize >= ATA_MIN_SECTOR_SIZE);
    DevExt->Device.SectorSize = max(DevExt->Device.SectorSize, ATA_MIN_SECTOR_SIZE);

    INFO("Total sectors %I64u of size %lu, CHS %u:%u:%u\n",
         DevExt->Device.TotalSectors,
         DevExt->Device.SectorSize,
         DevExt->Device.Cylinders,
         DevExt->Device.Heads,
         DevExt->Device.SectorsPerTrack);
}

static
CODE_SEG("PAGE")
VOID
AtaDeviceEnableQueuedCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG DeviceQueueDepth;

    PAGED_CODE();

    DeviceQueueDepth = AtaDevQueueDepth(&DevExt->IdentifyDeviceData);
    if (DeviceQueueDepth == 0)
        return;

    /* Do not set the queue depth to larger than the HBA can handle */
    DeviceQueueDepth = min(DeviceQueueDepth, DevExt->Device.PortData->QueueDepth);

    DevExt->Device.TransportFlags |= DeviceQueueDepth << DEVICE_QUEUE_DEPTH_SHIFT;

    DevExt->Device.DeviceFlags |= DEVICE_NCQ;

    INFO("NCQ enabled, queue depth %lu/%lu\n",
         DeviceQueueDepth,
         DevExt->Device.PortData->QueueDepth);
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaDeviceIsSuperFloppy(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PAGED_CODE();

    if (DevExt->InquiryData.DeviceType == DIRECT_ACCESS_DEVICE)
    {
        /* Look for ATAPI SuperDisk drives. For example, 'MATSHITA LS-120 COSM  03' */
        return (strstr(DevExt->FriendlyName, " LS-120") ||
                strstr(DevExt->FriendlyName, " LS-240"));
    }

    return FALSE;
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

    if (IS_ATAPI(&DevExt->Device))
    {
        if (AtaDeviceIsSuperFloppy(DevExt))
            DevExt->Device.DeviceFlags |= DEVICE_IS_SUPER_FLOPPY;

        if (AtaDevIsTape(&DevExt->IdentifyPacketData))
        {
            /* TODO: Should we really need to implement DSC polling support? */
            WARN("Tape drive detected '%s'\n", DevExt->FriendlyName);
        }
    }
    else
    {
        if (ChanExt->PortData.PortFlags & PORT_FLAG_NCQ)
            AtaDeviceEnableQueuedCommands(DevExt);

        AtaDeviceSetAddressingMode(DevExt);
        AtaCreateStandardInquiryData(DevExt);
    }

    if (DevExt->Device.AtaScsiAddress.Lun == 0)
    {
        AtaSetRegistryKey(ChanExt,
                          DevExt->Device.AtaScsiAddress.TargetId,
                          DD_ATA_REG_ATA_DEVICE_TYPE,
                          DevExt->DeviceType);

        AtaSetRegistryKey(ChanExt,
                          DevExt->Device.AtaScsiAddress.TargetId,
                          DD_ATA_REG_SCSI_DEVICE_TYPE,
                          DevExt->InquiryData.DeviceType);

        AtaGetRegistryKey(ChanExt,
                          DevExt->Device.AtaScsiAddress.TargetId,
                          DD_ATA_REG_XFER_MODE_ALLOWED,
                          &DevExt->TransferModeUserAllowedMask,
                          MAXULONG);
    }

    /* Will be unlocked upon PnP START IRP */
    AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PNP);

    if (!AtaDevIsSsd(&DevExt->IdentifyDeviceData))
    {
        /* The spindle motor requires an inrush of power at start-up */
        DevExt->Common.Self->Flags |= DO_POWER_INRUSH;
    }
    if (DevExt->InquiryData.RemovableMedia)
    {
        /* Distinguish between fixed and removable drives (e.g. CFA media) */
        DevExt->Common.Self->Characteristics |= FILE_REMOVABLE_MEDIA;
    }
    DevExt->Common.Self->Flags &= ~DO_DEVICE_INITIALIZING;

    DevExt->Device.DeviceFlags &= ~DEVICE_UNINITIALIZED;
}

static
CODE_SEG("PAGE")
ULONG
AtaFdoQueryDeviceCount(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PAGED_CODE();

    KeClearEvent(&PortData->Worker.EnumerationEvent);
    AtaDeviceQueueEvent(PortData, NULL, ACTION_ENUM_PORT);
    KeWaitForSingleObject(&PortData->Worker.EnumerationEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    return PortData->Worker.DeviceCount;
}

static
CODE_SEG("PAGE")
ATA_DEVICE_STATUS
AtaFdoQueryDeviceStatus(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action)
{
    PAGED_CODE();

    KeClearEvent(&DevExt->Worker.EnumerationEvent);
    AtaDeviceQueueEvent(PortData, DevExt, Action);
    KeWaitForSingleObject(&DevExt->Worker.EnumerationEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    return DevExt->Worker.EnumStatus;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoEnumeratePort(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;
    ULONG i, DeviceCount;
    BOOLEAN DiscoveredNewDevice = FALSE;

    PAGED_CODE();

    DeviceCount = AtaFdoQueryDeviceCount(PortData);

    for (i = 0; i < DeviceCount; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        ATA_SCSI_ADDRESS AtaScsiAddress;
        ATA_DEVICE_STATUS EnumStatus;

        AtaScsiAddress = AtaMarshallScsiAddress(PortData->PortNumber, i, 0);

        /* Try to find an existing device */
        DevExt = AtaFdoFindDeviceByPath(ChanExt, AtaScsiAddress, NULL);
        if (!DevExt)
        {
            DevExt = AtaPdoCreateDevice(ChanExt, AtaScsiAddress);
            if (!DevExt)
            {
                /* We are out of memory, trying to continue process the QBR IRP anyway */
                ERR("Failed to allocate PDO extension\n");
                continue;
            }

            /* Query the last known device type */
            AtaGetRegistryKey(ChanExt,
                              i,
                              DD_ATA_REG_ATA_DEVICE_TYPE,
                              &DevExt->DeviceType,
                              DEV_NONE);

            PortData->Worker.EnumDevExt = DevExt;
            EnumStatus = AtaFdoQueryDeviceStatus(PortData, DevExt, ACTION_ENUM_DEVICE_NEW);
        }
        else
        {
            EnumStatus = AtaFdoQueryDeviceStatus(PortData, DevExt, ACTION_ENUM_DEVICE);
        }

        /* Determine the type of the device */
        switch (EnumStatus)
        {
            /* No device present at this SCSI address */
            case DEV_STATUS_FAILED:
            case DEV_STATUS_NO_DEVICE:
            {
                if (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
                    AtaPdoFreeDevice(DevExt);
                continue;
            }

            /* It's the same device still */
            case DEV_STATUS_SAME_DEVICE:
            {
                ASSERT(!(DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED));

                DevExt->NotPresent = FALSE;
                continue;
            }

            /* At this point, we assume that the drive is a new device */
            case DEV_STATUS_NEW_DEVICE:
            {
                /* Device type has changed because of a hot-plug event */
                if (!(DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED))
                {
                    /* Create a new PDO for the newly hotplugged device */
                    --i; // Retry
                    continue;
                }
                break;
            }

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }

        AtaPdoInit(ChanExt, DevExt);
        AtaFdoDeviceListInsert(ChanExt, DevExt, TRUE);

        DiscoveredNewDevice = TRUE;
    }

    /* Prepare the channel for a new device */
    if (DiscoveredNewDevice)
        AtaDeviceQueueEvent(PortData, NULL, ACTION_PORT_TIMING);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoInitializeDeviceRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PDEVICE_RELATIONS DeviceRelations)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    DeviceRelations->Count = 0;

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
        if (!DevExt)
            break;

        if (DevExt->NotPresent)
        {
            if (AtaScsiAddress.Lun == 0)
            {
                AtaSetRegistryKey(ChanExt,
                                  AtaScsiAddress.TargetId,
                                  DD_ATA_REG_ATA_DEVICE_TYPE,
                                  DEV_NONE);
            }

            DevExt->ReportedMissing = TRUE;
            continue;
        }

        if (AtaScsiAddress.Lun == 0)
        {
            AtaSetRegistryKey(ChanExt, AtaScsiAddress.TargetId,
                              DD_ATA_REG_XFER_MODE_SUPPORTED,
                              DevExt->TransferModeSupportedBitmap);

            AtaSetRegistryKey(ChanExt,
                              AtaScsiAddress.TargetId,
                              DD_ATA_REG_XFER_MODE_SELECTED,
                              DevExt->TransferModeSelectedBitmap);
        }

        DeviceRelations->Objects[DeviceRelations->Count++] = DevExt->Common.Self;
        ObReferenceObject(DevExt->Common.Self);
    }
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations = NULL;
    ULONG Size, PdoCount;
    ATA_SCSI_ADDRESS AtaScsiAddress;
#if DBG
    LARGE_INTEGER TimeStart, TimeFinish;
    ULONG EnumTimeMs;
#endif

    PAGED_CODE();

#if DBG
    KeQuerySystemTime(&TimeStart);
#endif

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
        if (!DevExt)
            break;

        DevExt->NotPresent = TRUE;

        if (DevExt->Device.AtaScsiAddress.Lun == 0)
        {
            AtaGetRegistryKey(ChanExt,
                              DevExt->Device.AtaScsiAddress.TargetId,
                              DD_ATA_REG_XFER_MODE_ALLOWED,
                              &DevExt->TransferModeUserAllowedMask,
                              MAXULONG);
        }
    }

    AtaFdoEnumeratePort(ChanExt);

    PdoCount = 0;
    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
        if (!DevExt)
            break;

        if (!DevExt->NotPresent)
            ++PdoCount;
    }

    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects[PdoCount]);
    DeviceRelations = ExAllocatePoolUninitialized(PagedPool, Size, ATAPORT_TAG);
    if (!DeviceRelations)
    {
        ERR("Failed to allocate device relations\n");
        goto Cleanup;
    }

    AtaFdoInitializeDeviceRelations(ChanExt, DeviceRelations);

#if DBG
    KeQuerySystemTime(&TimeFinish);
    EnumTimeMs = (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000;
    if (EnumTimeMs >= 5000)
    {
        WARN("%lu: QBR request took %lu ms, %lu devices\n",
             ChanExt->ScsiPortNumber,
             EnumTimeMs,
             DeviceRelations->Count);
    }
    else
    {
        INFO("%lu: QBR request took %lu ms, %lu devices\n",
             ChanExt->ScsiPortNumber,
             EnumTimeMs,
             DeviceRelations->Count);
    }
#endif

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;

Cleanup:
    if (DeviceRelations)
        ExFreePoolWithTag(DeviceRelations, ATAPORT_TAG);

    return STATUS_INSUFFICIENT_RESOURCES;
}
