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
    for (i = 0; i < (Length / sizeof(USHORT)); i += sizeof(USHORT))
    {
        Destination[i] = Source[i + 1];
        Destination[i + 1] = Source[i];
    }

    return &Destination[i];
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
    _Inout_updates_bytes_(Length * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG Length)
{
    PUSHORT Word = Buffer;

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

CODE_SEG("TEST")
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
     * them available in the system (for example, to allow
     * the user to send any command to the drive through the pass-through interface).
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

    DevExt->Device.MaxQueuedSlotsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - DeviceQueueDepth);

    /* Don't set the bitmap to larger than the HBA can handle */
    DevExt->Device.MaxQueuedSlotsBitmap &= DevExt->Device.MaxRequestsBitmap;

    DevExt->Device.DeviceFlags |= DEVICE_NCQ;

    INFO("NCQ enabled, queue depth %lu/%lu\n",
         DeviceQueueDepth,
         ((DevExt->Device.ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1);
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
        /* ATAPI SuperDisk drives */
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
    PATAPORT_PORT_DATA PortData = DevExt->Device.PortData;

    PAGED_CODE();

    DevExt->Device.DeviceFlags &= ~DEVICE_UNINITIALIZED;

    AtaPdoFillIdentificationStrings(DevExt);

    if (PortData->PortFlags & PORT_FLAG_IS_EXTERNAL)
        DevExt->Device.DeviceFlags |= DEVICE_IS_PDO_REMOVABLE;

    if (PortData->PortFlags & PORT_FLAG_IS_PMP)
    {
        DevExt->Device.DeviceFlags |= DEVICE_IS_PMP_DEVICE;
    }

    if (IS_ATAPI(&DevExt->Device))
    {
        if (AtaDeviceIsSuperFloppy(DevExt))
            DevExt->Device.DeviceFlags |= DEVICE_IS_SUPER_FLOPPY;

        if (AtaDevIsTape(&DevExt->IdentifyPacketData))
            WARN("FIXME: Tape drive detected\n"); // TODO: Implement DSC polling
    }
    else
    {
        if (ChanExt->AhciCapabilities & AHCI_CAP_SNCQ)
            AtaDeviceEnableQueuedCommands(DevExt);

        AtaDeviceSetAddressingMode(DevExt);
        AtaCreateStandardInquiryData(DevExt);
    }

    if (DevExt->Device.AtaScsiAddress.Lun == 0)
    {
        AtaSetRegistryKey(ChanExt,
                          DevExt->Device.AtaScsiAddress.TargetId,
                          L"DeviceType",
                          DevExt->DeviceClass);

        AtaSetRegistryKey(ChanExt,
                          DevExt->Device.AtaScsiAddress.TargetId,
                          L"ScsiDeviceType",
                          DevExt->InquiryData.DeviceType);
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
        /* Distinguish between fixed and removable drives */
        DevExt->Common.Self->Characteristics |= FILE_REMOVABLE_MEDIA;
    }
    DevExt->Common.Self->Flags &= ~DO_DEVICE_INITIALIZING;
}

static
CODE_SEG("PAGE")
VOID
AtaPdoUpdateScsiAddress(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Device.ChanExt;

    PAGED_CODE();

    DevExt->Device.AtaScsiAddress = AtaScsiAddress;
    DevExt->Device.DeviceSelect = IDE_DRIVE_SELECT | AtaScsiAddress.Lun;

    if (IS_AHCI(&DevExt->Device))
    {
        ASSERT(AtaScsiAddress.TargetId < AHCI_MAX_PMP_DEVICES);

        DevExt->Device.LocalBuffer = ChanExt->PortInfo[AtaScsiAddress.PathId].LocalBuffer;
        DevExt->Device.PortData = &ChanExt->PortData[AtaScsiAddress.PathId];
    }
    else
    {
        ASSERT(AtaScsiAddress.TargetId < CHANNEL_PC98_MAX_DEVICES);

        DevExt->Device.LocalBuffer = ChanExt->PortData->Pata.LocalBuffer;
        DevExt->Device.PortData = ChanExt->PortData;

        /* Master/Slave select bit */
        DevExt->Device.DeviceSelect |= ((AtaScsiAddress.TargetId & 1) << 4);
    }
}

static
CODE_SEG("PAGE")
VOID
AtaFdoEnumeratePort(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG TargetId;
    BOOLEAN DiscoveredNewDevice = FALSE;

    PAGED_CODE();

    for (TargetId = 0; TargetId < ChanExt->MaxTargetId; ++TargetId)
    {
        ULONG Lun, MaxLun;

        if (!(PortData->Worker.TargetBitmap & (1 << TargetId)))
            continue;

        MaxLun = 1;

        for (Lun = 0; Lun < MaxLun; ++Lun)
        {
            PATAPORT_DEVICE_EXTENSION DevExt;
            ATA_SCSI_ADDRESS AtaScsiAddress;
Retry:
            AtaScsiAddress = AtaMarshallScsiAddress(PortData->PortNumber, TargetId, Lun);

            /* Try to find an existing device */
            DevExt = AtaFdoFindDeviceByPath(ChanExt, AtaScsiAddress);
            if (!DevExt)
            {
                DevExt = AtaPdoCreateDevice(ChanExt, AtaScsiAddress);
                if (!DevExt)
                {
                    /* We are out of memory, trying to continue process the QBR IRP anyway */
                    ERR("Failed to allocate PDO extension\n");
                    continue;
                }
                PortData->Worker.EnumDevExt = DevExt;

                AtaPdoUpdateScsiAddress(DevExt, AtaScsiAddress);

                /* Query the last known device type */
                AtaGetRegistryKey(PortData->ChanExt,
                                  DevExt->Device.AtaScsiAddress.TargetId,
                                  L"DeviceType",
                                  &DevExt->DeviceClass,
                                  DEV_NONE);
            }

            /* Determine the type of the device */
            KeClearEvent(&DevExt->Worker.EnumerationEvent);
            AtaDeviceQueueEvent(PortData,
                                DevExt,
                                (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
                                ? ACTION_ENUM_DEVICE_EXT : ACTION_ENUM_DEVICE_INT);
            KeWaitForSingleObject(&DevExt->Worker.EnumerationEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            switch (DevExt->Worker.EnumStatus)
            {
                /* No device present at this SCSI address */
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
                default:
                {
                    if (!(DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED))
                        goto Retry; // todo hack rewrite
                    break;
                }
            }

            if (Lun == 0)
            {
#if 0 // todo
                /*
                 * Scan all logical units to support rare multi-lun ATAPI devices.
                 * (For example: TEAC PD-518E and Nakamichi MJ-5.16)
                 */
                if (IS_ATAPI(&DevExt->Device))
                    MaxLun = AtaDevMaxLun(&DevExt->IdentifyPacketData);
                else
#endif
                    MaxLun = 1;
            }

            DiscoveredNewDevice = TRUE;

            AtaPdoInit(ChanExt, DevExt);
            AtaFdoDeviceListInsert(ChanExt, DevExt, TRUE);
        }
    }

    if (DiscoveredNewDevice)
    {
        KeClearEvent(&PortData->Worker.SetTimingsEvent);
        AtaDeviceQueueEvent(PortData, NULL, ACTION_DEVICE_TIMING);
        KeWaitForSingleObject(&PortData->Worker.SetTimingsEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }
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
                                  L"DeviceType",
                                  DEV_NONE);
            }

            DevExt->ReportedMissing = TRUE;
            continue;
        }

        if (AtaScsiAddress.Lun == 0)
        {
            AtaSetRegistryKey(ChanExt,
                              AtaScsiAddress.TargetId,
                              L"DeviceTimingModeSupported",
                              DevExt->TransferModeSupportedBitmap);

            AtaSetRegistryKey(ChanExt,
                              AtaScsiAddress.TargetId,
                              L"DeviceTimingMode",
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
    PATAPORT_ENUM_CONTEXT EnumContext = NULL;
    ULONG i, Size, PdoCount;
    ATA_SCSI_ADDRESS AtaScsiAddress;
#if DBG
    LARGE_INTEGER TimeStart, TimeFinish;
    ULONG EnumTimeMs;
#endif

    PAGED_CODE();

#if DBG
    KeQuerySystemTime(&TimeStart);
#endif

    /* Update the PATA channel timing information */
    if (IS_PCIIDE_EXT(ChanExt) && !(ChanExt->Flags & CHANNEL_HAS_GTM))
    {
        if (AtaAcpiGetTimingMode(ChanExt, &ChanExt->PortData->Pata.CurrentTimingMode))
            ChanExt->Flags |= CHANNEL_HAS_GTM;
    }

    Size = FIELD_OFFSET(ATAPORT_ENUM_CONTEXT, WaitBlocks[ChanExt->NumberOfPorts]);
    EnumContext = ExAllocatePoolUninitialized(PagedPool, Size, ATAPORT_TAG);
    if (!EnumContext)
    {
        ERR("Failed to allocate enum context\n");
        goto Cleanup;
    }

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
        if (!DevExt)
            break;

        DevExt->NotPresent = TRUE;
    }

    /* Enumerate SATA ports or IDE channels in parallel */
    for (i = 0; i < ChanExt->NumberOfPorts; ++i)
    {
        PATAPORT_PORT_DATA PortData = &ChanExt->PortData[i];

        EnumContext->WaitEvents[i] = &PortData->Worker.EnumerationEvent;
        KeClearEvent(&PortData->Worker.EnumerationEvent);

        AtaDeviceQueueEvent(PortData, NULL, ACTION_ENUM_PORT);
    }
    KeWaitForMultipleObjects(ChanExt->NumberOfPorts,
                             (PVOID)EnumContext->WaitEvents,
                             WaitAll,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL,
                             EnumContext->WaitBlocks);

    for (i = 0; i < ChanExt->NumberOfPorts; ++i)
    {
        AtaFdoEnumeratePort(ChanExt, &ChanExt->PortData[i]);
    }

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
    if (EnumContext)
        ExFreePoolWithTag(EnumContext, ATAPORT_TAG);

    if (DeviceRelations)
        ExFreePoolWithTag(DeviceRelations, ATAPORT_TAG);

    return STATUS_INSUFFICIENT_RESOURCES;
}
