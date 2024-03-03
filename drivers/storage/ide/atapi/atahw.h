/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#define ATA_MAX_LBA_28 0x0FFFFFFFULL

/* Master/Slave devices */
#define CHANNEL_PCAT_MAX_DEVICES     2

/* Master/Slave devices for Bank 0 and Bank 1 */
#define CHANNEL_PC98_MAX_DEVICES     4

#define SRB_FLAG_INTERNAL          0x00000001
#define SRB_FLAG_PACKET_COMMAND    0x00000002
#define SRB_FLAG_POLL              0x00000008
#define SRB_FLAG_RETRY             0x00000010
#define SRB_FLAG_DEVICE_CHECK      0x00000020

#define SRB_SET_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb) \
    ((ULONG_PTR)(Srb)->SrbExtension)

#define QUEUE_ENTRY_FROM_IRP(Irp) \
    ((PREQUEST_QUEUE_ENTRY)&(((PIRP)(Irp))->Tail.Overlay.DriverContext[0]))

#define IRP_FROM_QUEUE_ENTRY(QueueEntry) \
    CONTAINING_RECORD(QueueEntry, IRP, Tail.Overlay.DriverContext[0])

#define LIST_ENTRY_FROM_IRP(Irp) \
    ((PLIST_ENTRY)&(Irp)->Tail.Overlay.DriverContext[0])

#define IRP_FROM_LIST_ENTRY(ListEntry) \
    (CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]))

#define IDE_DRIVE_SELECT_SLAVE       0x10
#define IDE_DRIVE_SELECT             0xA0
#define IDE_HIGH_ORDER_BYTE          0x80

#define IDE_ERROR_WRITE_PROTECT      0x40

#define IDE_FEATURE_PIO         0x00
#define IDE_FEATURE_DMA         0x01
#define IDE_FEATURE_DMADIR      0x04

#define IDE_DEVICE_FUA_NCQ      0x80

FORCEINLINE
ATA_DEVICE_CLASS
AtaPdoGetDeviceClass(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    if (IS_ATAPI(DevExt))
        return DEV_ATAPI;

    return DEV_ATA;
}

FORCEINLINE
BOOLEAN
AtaPdoSerialNumberValid(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    return (DevExt->SerialNumber[0] != ANSI_NULL);
}

FORCEINLINE
ULONG
AtaFdoMaxDeviceCount(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    if (IS_AHCI(ChanExt))
        return AHCI_MAX_PORTS;
#if defined(_M_IX86)
    else if (ChanExt->Flags & CHANNEL_CBUS_IDE)
        return CHANNEL_PC98_MAX_DEVICES;
#endif
    else
        return CHANNEL_PCAT_MAX_DEVICES;
}

FORCEINLINE
UCHAR
AtaReadWriteCommand(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG CmdEntry = (Request->Flags & REQUEST_FLAG_LBA48) ? 0 : 6;

    if (Request->Flags & REQUEST_FLAG_FUA)
    {
        CmdEntry += 3;
    }

    if (Request->Flags & REQUEST_FLAG_DMA_ENABLED)
    {
        CmdEntry += 2;
    }
    else if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
    {
        CmdEntry += 1;
    }

    return AtapAtaCommand[CmdEntry][(Request->Flags & REQUEST_FLAG_DATA_IN) ? 1 : 0];
}

FORCEINLINE
BOOLEAN
AtaPacketCommandUseDma(
    _In_ UCHAR OpCode)
{
    return (OpCode == SCSIOP_READ6 ||
            OpCode == SCSIOP_WRITE6 ||
            OpCode == SCSIOP_READ ||
            OpCode == SCSIOP_WRITE ||
            OpCode == SCSIOP_READ12 ||
            OpCode == SCSIOP_WRITE12 ||
            OpCode == SCSIOP_READ16 ||
            OpCode == SCSIOP_WRITE16);
}

FORCEINLINE
BOOLEAN
AtaCommandUseLba48(
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount)
{
    /* Use the 48-bit command when reasonable */
    return (((SectorNumber + SectorCount) >= ATA_MAX_LBA_28) || (SectorCount > 0x100));
}

FORCEINLINE
BOOLEAN
AtaDeviceHasCdbInterrupt(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 5:6 of word 0 */
    return (IdentifyPacketData->GeneralConfiguration.DrqDelay == 1);
}

FORCEINLINE
UCHAR
AtaDeviceCdbSizeInWords(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 0:2 of word 0 */
    return (IdentifyPacketData->GeneralConfiguration.PacketType != 0) ? 8 : 6;
}

FORCEINLINE
BOOLEAN
AtaDeviceMaxLun(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 0:2 of word 126 */
    USHORT LastLunIdentifier = IdentifyPacketData->ReservedWord126 & 7;

    /*
     * Make sure this field has no value that represents all bits set.
     * We perform additional validation because
     * most ATAPI devices ignore the LUN field in the CDB and respond to each LUN.
     */
    if (LastLunIdentifier != 7)
        return LastLunIdentifier + 1;

    return 1;
}

FORCEINLINE
BOOLEAN
AtaDeviceDmaDirectionRequired(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bit 15 of word 62 */
    if (IdentifyPacketData->DMADIR.DMADIRBitRequired)
    {
        return !(IdentifyPacketData->MultiWordDMASupport & 0x7) &&          // Bits 0:2 of word 63
               !IdentifyPacketData->Capabilities.DmaSupported &&            // Bit 8 of word 49
               !IdentifyPacketData->Capabilities.InterleavedDmaSupported && // Bit 15 of word 49
               !(IdentifyPacketData->UltraDMAActive & 0x7F);                // Bits 0:6 of word 88
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceSerialNumberValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Some ancient IDE drives return the empty serial number */
    return (IdentifyData->SerialNumber[0] != ' ' &&
            IdentifyData->SerialNumber[0] != ANSI_NULL);
}

FORCEINLINE
BOOLEAN
AtaHas48BitAddressFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
   if (IdentifyData->CommandSetSupport.WordValid83 == 1)
   {
        /* Bit 10 of word 83 */
        return IdentifyData->CommandSetSupport.BigLba;
   }

   return FALSE;
}

FORCEINLINE
BOOLEAN
AtaCurrentGeometryValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    return ((IdentifyData->TranslationFieldsValid & 1) &&
            (IdentifyData->NumberOfCurrentCylinders != 0) &&
            (IdentifyData->NumberOfCurrentCylinders <= 63) &&
            (IdentifyData->NumberOfCurrentHeads != 0) &&
            (IdentifyData->NumberOfCurrentHeads <= 16) &&
            (IdentifyData->CurrentSectorsPerTrack != 0));
}

FORCEINLINE
UCHAR
AtaMaximumSectorsPerDrq(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    UCHAR MultiSectorMax;

    /* The word 47 should be a power of 2 */
    MultiSectorMax = IdentifyData->MaximumBlockTransfer;

    if ((MultiSectorMax > 0) && ((MultiSectorMax & (MultiSectorMax - 1)) == 0))
        return MultiSectorMax;

    return 0;
}

FORCEINLINE
ULONG
AtaBytesPerLogicalSector(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 106: 15 = 0, 14 = 1, 12 = 1 */
    if (IdentifyData->PhysicalLogicalSectorSize.Reserved1 == 1 &&
        IdentifyData->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words)
    {
        /* Words 116-117 */
        return (((ULONG)IdentifyData->WordsPerLogicalSector[1] << 16) |
                IdentifyData->WordsPerLogicalSector[0]) * sizeof(USHORT);
    }

    /* 256 words = 512 bytes */
    return 256 * sizeof(USHORT);
}

FORCEINLINE
ULONG
AtaLogicalSectorsPerPhysicalSector(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PULONG Exponent)
{
    /* Word 106: 15 = 0, 14 = 1, 13 = 1 */
    if (IdentifyData->PhysicalLogicalSectorSize.Reserved1 == 1 &&
        IdentifyData->PhysicalLogicalSectorSize.MultipleLogicalSectorsPerPhysicalSector)
    {
        /* Bits 0:3 of word 106 */
        *Exponent = IdentifyData->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector;

        return 1 << *Exponent;
    }

    *Exponent = 0;
    return 1 << 0;
}

FORCEINLINE
ULONG
AtaLogicalSectorAlignment(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 209: 15 = 0, 14 = 1 */
    if (IdentifyData->BlockAlignment.Word209Supported &&
        IdentifyData->BlockAlignment.Reserved0 == 0)
    {
        /* Bits 0:13 of word 209 */
        return IdentifyData->BlockAlignment.AlignmentOfLogicalWithinPhysical;
    }

    return 0;
}

FORCEINLINE
BOOLEAN
AtaDeviceInPuisState(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 2 of word 0 */
    if (IdentifyData->GeneralConfiguration.ResponseIncomplete)
    {
        /* Word 2 */
        if (IdentifyData->SpecificConfiguration == 0x37C8 ||
            IdentifyData->SpecificConfiguration == 0x738C)
        {
            return TRUE;
        }
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceIsRemovable(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 7 of word 0 */
    return IdentifyData->GeneralConfiguration.RemovableMedia;
}

FORCEINLINE
ULONG
AtaDeviceQueueDepth(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 8 of word 76 */
    if (IdentifyData->SerialAtaCapabilities.NCQ)
    {
        /* Bits 0:4 of word 75 */
        return IdentifyData->QueueDepth + 1;
    }

    return 0;
}

FORCEINLINE
BOOLEAN
AtaDeviceHasRemovableMediaStatusNotificationFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 127: bit 0 = 1, bit 1 = 0 */
    return (IdentifyData->MsnSupport == 1);
}

FORCEINLINE
BOOLEAN
AtaDeviceHasIeee1667(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    // TODO: verify word 48

    /* Bit 0 of word 48 */
    if (IdentifyData->TrustedComputing.FeatureSupported)
    {
        /* Bit 7 of word 69 */
        return IdentifyData->AdditionalSupported.IEEE1667;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceHasWorldWideName(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 87: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetActive.Reserved4 == 1)
    {
        /* Bit 8 of word 87 */
        return IdentifyData->CommandSetActive.WWN64Bit;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceIsZonedDevice(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bits 0:1 of word 69 */
    return (IdentifyData->AdditionalSupported.ZonedCapabilities != 0);
}

FORCEINLINE
BOOLEAN
AtaDeviceHasSmartFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 0 of word 82 */
    return IdentifyData->CommandSetSupport.SmartCommands;
}

FORCEINLINE
BOOLEAN
AtaDeviceIsVolatileWriteCacheEnabled(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 5 of word 82 and bit 5 of word 85 */
        return (IdentifyData->CommandSetSupport.WriteCache &&
                IdentifyData->CommandSetActive.WriteCache);
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceIsReadLookAHeadEnabled(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 6 of word 82 and bit 6 of word 85 */
        return (IdentifyData->CommandSetSupport.LookAhead &&
                IdentifyData->CommandSetActive.LookAhead);
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDeviceIsRotatingDevice(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 217 */
    return (IdentifyData->NominalMediaRotationRate >= 0x0401 &&
            IdentifyData->NominalMediaRotationRate <= 0xFFFE);
}

FORCEINLINE
BOOLEAN
AtaDeviceIsSsd(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 217 */
    return (IdentifyData->NominalMediaRotationRate == 1);
}

FORCEINLINE
BOOLEAN
AtaHasTrimFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 0 of word 169 */
    return IdentifyData->DataSetManagementFeature.SupportsTrim;
}

FORCEINLINE
BOOLEAN
AtaHasRzatFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 5 of word 69 */
    return IdentifyData->AdditionalSupported.ReadZeroAfterTrimSupported;
}

FORCEINLINE
BOOLEAN
AtaHasDratFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 14 of word 69 */
    return IdentifyData->AdditionalSupported.DeterministicReadAfterTrimSupported;
}

FORCEINLINE
BOOLEAN
AtaHasForceUnitAccessCommands(
  _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 84: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 6 of word 84 */
        return IdentifyData->CommandSetSupport.WriteFua;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaStandybyTimerPeriodsValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 13 of word 49 */
    return IdentifyData->Capabilities.StandybyTimerSupport;
}
