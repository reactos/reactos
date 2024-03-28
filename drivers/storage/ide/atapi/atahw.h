/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
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
#define SRB_FLAG_DEVICE_CHECK      SRB_FLAG_INTERNAL

#define SRB_FLAGS_PASSTHROUGH      0x08000000

#define SRB_SET_FLAG(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb) \
    ((ULONG_PTR)(Srb)->SrbExtension)

#define QUEUE_ENTRY_FROM_IRP(Irp) \
    ((PREQUEST_QUEUE_ENTRY)&(((PIRP)(Irp))->Tail.Overlay.DriverContext[0]))

#define IRP_FROM_QUEUE_ENTRY(QueueEntry) \
    CONTAINING_RECORD(QueueEntry, IRP, Tail.Overlay.DriverContext[0])

#define ATA_IO_WAIT() \
    KeStallExecutionProcessor(1)

#define CMD_FLAG_NONE              0x00000000
#define CMD_FLAG_AWAIT_INTERRUPT   0x80000000
#define CMD_FLAG_REQUEST_MASK      0x00000003
#define CMD_FLAG_CDB               0x00000004
#define CMD_FLAG_READ_TASK_FILE    0x00000008
#define CMD_FLAG_DATA_IN           0x00000040
#define CMD_FLAG_DATA_OUT          0x00000080

#define CMD_FLAG_ATAPI_REQUEST     0x00000001
#define CMD_FLAG_ATA_REQUEST       0x00000002
#define CMD_FLAG_DMA_TRANSFER      0x00000003

C_ASSERT(CMD_FLAG_DATA_IN == SRB_FLAGS_DATA_IN);
C_ASSERT(CMD_FLAG_DATA_OUT == SRB_FLAGS_DATA_OUT);

/* Ignore the REL and Tag bits */
#define ATAPI_INTERRUPT_REASON_MASK                   0x03

#define ATAPI_INTERRUPT_REASON_INVALID                0x00
#define ATAPI_INTERRUPT_REASON_CMD_COMPLETION         0x03
#define ATAPI_INTERRUPT_REASON_DATA_OUT               0x08
#define ATAPI_INTERRUPT_REASON_AWAIT_CDB              0x09
#define ATAPI_INTERRUPT_REASON_DATA_IN                0x0A

#define ATA_TIME_BUSY_SELECT    2000 /* 20 ms */
#define ATA_TIME_BUSY_NORMAL    200000
#define ATA_TIME_BUSY_ISR       3
#define ATA_TIME_BUSY_POLL      5
#define ATA_TIME_DRQ_NORMAL     100000
#define ATA_TIME_DRQ_ISR        1000

#define IDE_DRIVE_SELECT    0xA0
#define IDE_HIGH_ORDER_BYTE          0x80

#define IDE_ERROR_WRITE_PROTECT      0x40

FORCEINLINE
UCHAR
ATA_READ(
  _In_ PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);;
}

FORCEINLINE
VOID
ATA_WRITE(
  _In_ PUCHAR Port,
  _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

#define ATA_WRITE_ULONG(Port, Value) \
    WRITE_PORT_ULONG(Port, Value)

FORCEINLINE
VOID
ATA_WRITE_BLOCK_16(
  _In_ PUSHORT Port,
  _In_ PUSHORT Buffer,
  _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_WRITE_BLOCK_32(
  _In_ PULONG Port,
  _In_ PULONG Buffer,
  _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_16(
  _In_ PUSHORT Port,
  _In_ PUSHORT Buffer,
  _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_32(
  _In_ PULONG Port,
  _In_ PULONG Buffer,
  _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

#if defined(_M_IX86)
FORCEINLINE
VOID
ATA_SELECT_BANK(
    _In_ UCHAR Number)
{
    ASSERT((Number & ~1) == 0);

    /* The 0x432 port is used to select the primary or secondary IDE channel */
    WRITE_PORT_UCHAR((PUCHAR)0x432, Number);
}
#endif

FORCEINLINE
VOID
ATA_SELECT_DEVICE(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR DeviceHead)
{
#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
    {
        ATA_SELECT_BANK((DeviceHead >> 1) & 1);
    }
#endif

    ATA_WRITE(ChannelExtension->Registers.DriveSelect, DeviceHead & 0xF8);
    ATA_IO_WAIT();
}

FORCEINLINE
VOID
ATA_WAIT_ON_BUSY(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PUCHAR Status,
    _In_ ULONG Timeout)
{
    ULONG i;

    if (!(*Status & IDE_STATUS_BUSY))
        return;

    for (i = 0; i < Timeout; ++i)
    {
        KeStallExecutionProcessor(10);

        *Status = ATA_READ(ChannelExtension->Registers.Status);
        if (!(*Status & IDE_STATUS_BUSY))
            break;

        if (*Status == 0xFF)
            break;
    }
}

FORCEINLINE
VOID
ATA_WAIT_FOR_DRQ(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PUCHAR Status,
    _In_ UCHAR Mask,
    _In_ ULONG Timeout)
{
    ULONG i;

    if (*Status & IDE_STATUS_DRQ)
        return;

    for (i = 0; i < Timeout; ++i)
    {
        KeStallExecutionProcessor(10);

        *Status = ATA_READ(ChannelExtension->Registers.Status);

        if (!(*Status & IDE_STATUS_BUSY))
        {
            if (*Status & (IDE_STATUS_DRQ | Mask))
                return;

            if (*Status == 0xFF)
                break;
        }
    }
}

FORCEINLINE
ATA_DEVICE_CLASS
AtaPdoGetDeviceClass(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    if (IS_ATAPI(DeviceExtension))
        return DEV_ATAPI;

    return DEV_ATA;
}

FORCEINLINE
BOOLEAN
AtaPdoSerialNumberValid(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    return (DeviceExtension->SerialNumber[0] != ANSI_NULL);
}

FORCEINLINE
ULONG
AtaFdoMaxDeviceCount(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
        return CHANNEL_PC98_MAX_DEVICES;
    else
#endif
        return CHANNEL_PCAT_MAX_DEVICES;
}

FORCEINLINE
UCHAR
AtaReadWriteCommand(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    ULONG CmdEntry;

    CmdEntry = (Request->Flags & REQUEST_FLAG_DATA_IN) ? 3 : 0;

    if (Request->Flags & REQUEST_FLAG_FUA)
    {
        CmdEntry += 6;
    }

    if (Request->Flags & REQUEST_FLAG_DMA_ENABLED)
    {
        CmdEntry += 2;
    }
    else
    {
        if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
        {
            CmdEntry += 1;
        }
    }

    return AtapAtaCommand[CmdEntry][(Request->Flags & REQUEST_FLAG_LBA48) ? 1 : 0];
}

FORCEINLINE
BOOLEAN
AtaAtapiUseDma(
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
AtaReadWriteUseLba48(
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
        /* Bits 10 of word 83 */
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
AtaHasForcedUnitAccessCommands(
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
