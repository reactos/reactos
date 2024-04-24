/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     EEPROM manipulation and parsing
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

#define SROM_READ(Adapter, Data)  \
    do { \
        *Data = DC_READ((Adapter), DcCsr9_SerialInterface); \
        NdisStallExecution(10); \
    } while (0)

#define SROM_WRITE(Adapter, Value)  \
    do { \
        DC_WRITE((Adapter), DcCsr9_SerialInterface, Value); \
        NdisStallExecution(10); \
    } while (0)

extern DC_PG_DATA DC_SROM_REPAIR_ENTRY SRompRepairData[];

LIST_ENTRY SRompAdapterList;

_Interlocked_
static volatile LONG SRompAdapterLock = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static
CODE_SEG("PAGE")
VOID
SRomAcquireListMutex(VOID)
{
    PAGED_CODE();

    while (_InterlockedCompareExchange(&SRompAdapterLock, 1, 0))
    {
        NdisMSleep(10);
    }
}

static inline
CODE_SEG("PAGE")
VOID
SRomReleaseListMutex(VOID)
{
    PAGED_CODE();

    _InterlockedDecrement(&SRompAdapterLock);
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomIsAdapterInList(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN SearchForMaster,
    _Out_opt_ PDC_SROM_ENTRY* FoundEntry)
{
    PLIST_ENTRY PrevEntry;
    PDC_SROM_ENTRY SRomEntry;

    PAGED_CODE();

    /* Loop the adapter list backwards */
    for (PrevEntry = (&SRompAdapterList)->Blink;
         PrevEntry != &SRompAdapterList;
         PrevEntry = PrevEntry->Blink)
    {
        SRomEntry = CONTAINING_RECORD(PrevEntry, DC_SROM_ENTRY, ListEntry);

        if ((SRomEntry->ChipType == Adapter->ChipType) &&
            (SRomEntry->BusNumber == Adapter->BusNumber) &&
            (!SearchForMaster || (SRomEntry->DeviceNumber == Adapter->DeviceNumber)))
        {
            if (FoundEntry)
                *FoundEntry = SRomEntry;

            return TRUE;
        }
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomRegisterMasterAdapter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_SROM_ENTRY SRomEntry)
{
    BOOLEAN Success;

    PAGED_CODE();

    SRomAcquireListMutex();

    /* Check if board is already registered */
    if (SRomIsAdapterInList(Adapter, TRUE, NULL))
    {
        Success = FALSE;
        goto Exit;
    }

    Adapter->SRomEntry = SRomEntry;

    SRomEntry->ChipType = Adapter->ChipType;
    SRomEntry->BusNumber = Adapter->BusNumber;
    SRomEntry->DeviceNumber = Adapter->DeviceNumber;
    SRomEntry->InterruptLevel = Adapter->InterruptLevel;
    SRomEntry->InterruptVector = Adapter->InterruptVector;

    /* Register the port */
    SRomEntry->DeviceBitmap |= 1 << Adapter->DeviceNumber;

    /*
     * On some multiport boards only the first port contains an EEPROM.
     * We put their references to the global adapter list.
     */
    InsertTailList(&SRompAdapterList, &SRomEntry->ListEntry);
    Success = TRUE;

Exit:
    SRomReleaseListMutex();

    return Success;
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomFindMasterAdapter(
    _In_ PDC21X4_ADAPTER Adapter,
    _Out_ PDC_SROM_ENTRY* FoundEntry)
{
    PDC_SROM_ENTRY SRomEntry;
    ULONG i;
    BOOLEAN Found;

    PAGED_CODE();

    SRomAcquireListMutex();

    if (!SRomIsAdapterInList(Adapter, FALSE, &SRomEntry))
    {
        Found = FALSE;
        goto Exit;
    }

    Adapter->SRomEntry = SRomEntry;

    /* Register the port */
    SRomEntry->DeviceBitmap |= 1 << Adapter->DeviceNumber;

    /*
     * Determine the port index that should be used in order to
     * (possibly) update the base MAC address.
     */
    for (i = 0; i < PCI_MAX_DEVICES; ++i)
    {
        if (i == Adapter->DeviceNumber)
            break;

        if (SRomEntry->DeviceBitmap & (1 << i))
            ++Adapter->ControllerIndex;
    }

    /*
     * On a multiport board there can be up to 4 ports
     * connected through a 21050 or 21152 PCI-to-PCI Bridge.
     * These boards share a single IRQ line between all of the chips.
     * Some BIOSes incorrectly assign different IRQs to the different ports.
     */
    Adapter->InterruptLevel = SRomEntry->InterruptLevel;
    Adapter->InterruptVector = SRomEntry->InterruptVector;

    WARN("EEPROM is missing on controller %u, using image from the master at %u:%u\n",
         Adapter->DeviceNumber,
         SRomEntry->BusNumber,
         SRomEntry->DeviceNumber);

    *FoundEntry = SRomEntry;
    Found = TRUE;

Exit:
    SRomReleaseListMutex();

    return Found;
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomIsEmpty(
    _In_reads_bytes_(Length) const VOID* Buffer,
    _In_ ULONG Length)
{
    const UCHAR* Data = Buffer;
    const UCHAR FirstByte = Data[0];
    ULONG i;

    PAGED_CODE();

    for (i = 1; i < Length; ++i)
    {
        if (FirstByte != Data[i])
            return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
SRomNWayAdvertise(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG MediaCode)
{
    PAGED_CODE();

    switch (MediaCode)
    {
        case MEDIA_10T:
            Adapter->SymAdvertising |= MII_ADV_10T_HD;
            break;
        case MEDIA_10T_FD:
            Adapter->SymAdvertising |= MII_ADV_10T_FD;
            break;
        case MEDIA_100TX_HD:
            Adapter->SymAdvertising |= MII_ADV_100T_HD;
            break;
        case MEDIA_100TX_FD:
            Adapter->SymAdvertising |= MII_ADV_100T_FD;
            break;
        case MEDIA_100T4:
            Adapter->SymAdvertising |= MII_ADV_100T4;
            break;

        default:
            break;
    }
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockGpr(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData)
{
    PDC_MEDIA Media;
    ULONG MediaCode, OpMode;
    USHORT Command;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 4))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    MediaCode = SRomGetMediaCode(*BlockData++);
    if (MediaCode > SROM_MEDIA_MAX)
    {
        WARN("Unknown media code %u\n", MediaCode);
        return NDIS_STATUS_SUCCESS;
    }
    Adapter->MediaBitmap |= 1 << MediaCode;

    Media = &Adapter->Media[MediaCode];

    Media->GpioData = *BlockData++;

    Command = DcRetrieveWord(BlockData);

    OpMode = Media->OpMode;
    OpMode &= ~SROM_OPMODE_MASK;
    OpMode |= SRomCommandToOpMode(Command);
    Media->OpMode = OpMode;

    if (SRomMediaHasActivityIndicator(Command))
    {
        Media->LinkMask = SRomMediaGetSenseMask(Command);
    }
    if (SRomMediaActivityIsActiveLow(Command))
    {
        Media->Polarity = 0xFFFFFFFF;
    }

    INFO("GPR #%u %s, Command %04lx, Data %02lx\n",
         MediaCode,
         MediaNumber2Str(Adapter, MediaCode),
         Command,
         Media->GpioData);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockMii(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData,
    _In_ BOOLEAN IsOldVersion)
{
    PDC_MII_MEDIA Media;
    ULONG i, Bytes, Offset;
    UCHAR PhyNumber, StreamLength, InterruptInfo;
    USHORT Capabilities, Fdx, Ttm;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 2))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    PhyNumber = *BlockData++;

    /*
     * Even though the SROM specification allows several
     * PHY devices to be connected to the same chip on a board,
     * most if not all boards never use more than 1 MII PHY device.
     */
    if (Adapter->Features & DC_HAS_MII)
    {
        WARN("Unsupported PHY %u\n", PhyNumber);
        return NDIS_STATUS_SUCCESS;
    }

    Media = &Adapter->MiiMedia;

    /*
     * PHY selection sequence
     */

    StreamLength = *BlockData++;
    if (StreamLength > SROM_MAX_STREAM_REGS)
    {
        WARN("Too much registers %u\n", StreamLength);
        return NDIS_STATUS_SUCCESS;
    }

    Bytes = StreamLength;
    if (!IsOldVersion)
    {
        /* In words */
        Bytes *= 2;
    }
    if ((BlockData + Bytes) > (SRomEnd - 1))
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }

    Media->SetupStreamLength = StreamLength;

    /* Check if we already have the GPIO direction data */
    if (Media->SetupStream[0] != 0)
    {
        Offset = 1;
        ++Media->SetupStreamLength;
    }
    else
    {
        Offset = 0;
    }

    for (i = 0; i < StreamLength; ++i)
    {
        if (IsOldVersion)
        {
            Media->SetupStream[i + Offset] = *BlockData++;
        }
        else
        {
            Media->SetupStream[i + Offset] = DcRetrieveWord(BlockData);
            BlockData += sizeof(USHORT);
        }
    }

    /*
     * PHY reset sequence
     */

    if (BlockData > (SRomEnd - 1))
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }

    StreamLength = *BlockData++;
    if (StreamLength > SROM_MAX_STREAM_REGS)
    {
        WARN("Too much registers %u\n", StreamLength);
        return NDIS_STATUS_SUCCESS;
    }

    Bytes = StreamLength;
    if (!IsOldVersion)
    {
        /* In words */
        Bytes *= 2;
    }
    if ((BlockData + Bytes) > (SRomEnd - 1))
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }

    Media->ResetStreamLength = StreamLength;

    for (i = 0; i < StreamLength; ++i)
    {
        if (IsOldVersion)
        {
            Media->ResetStream[i] = *BlockData++;
        }
        else
        {
            Media->ResetStream[i] = DcRetrieveWord(BlockData);
            BlockData += sizeof(USHORT);
        }
    }

    /*
     * MII data
     */

    Bytes = 4 * sizeof(USHORT);
    if (!IsOldVersion)
    {
        Bytes += 1;
    }
    if (BlockData > (SRomEnd - Bytes))
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }

    Capabilities = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Media->Advertising = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Fdx = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Ttm = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    InterruptInfo = IsOldVersion ? 0 : *BlockData;

    Adapter->Features |= DC_HAS_MII;

    INFO("MII #%u, Caps %04lx, Adv %04lx, Fdx %04lx, Ttm %04lx, Int %02x\n",
         PhyNumber,
         Capabilities,
         Media->Advertising,
         Fdx,
         Ttm,
         InterruptInfo);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockSia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData)
{
    PDC_MEDIA Media;
    UCHAR BlockStart;
    ULONG MediaCode;
    BOOLEAN HasExtendedData;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 1))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    BlockStart = *BlockData++;

    HasExtendedData = SRomBlockHasExtendedData(BlockStart);
    if (BlockData > (SRomEnd - (HasExtendedData ? 10 : 4)))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    MediaCode = SRomGetMediaCode(BlockStart);
    if (MediaCode > SROM_MEDIA_MAX && MediaCode != SROM_MEDIA_HMR)
    {
        WARN("Unknown media code %u\n", MediaCode);
        return NDIS_STATUS_SUCCESS;
    }

    /* TODO: There were a few 21143-based boards with HMR media */
    if ((MediaCode == SROM_MEDIA_HMR) && (Adapter->ChipType != DC21145))
    {
        ERR("FIXME: 21143 HMR is not supported yet\n");
        return NDIS_STATUS_SUCCESS;
    }

    /* Map the code to our internal value */
    if (MediaCode == SROM_MEDIA_HMR)
    {
        MediaCode = MEDIA_HMR;
    }

    Adapter->MediaBitmap |= 1 << MediaCode;

    Media = &Adapter->Media[MediaCode];

    if (HasExtendedData)
    {
        Media->Csr13 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);

        Media->Csr14 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);

        Media->Csr15 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);
    }

    Media->GpioCtrl = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Media->GpioData = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    SRomNWayAdvertise(Adapter, MediaCode);

    INFO("SIA #%u %s, %sCSR13 %04lx CSR14 %04lx CSR15 %04lx, "
         "Ctrl %04lx, Data %04lx\n",
         MediaCode,
         MediaNumber2Str(Adapter, MediaCode),
         HasExtendedData ? "EXT " : "",
         Media->Csr13,
         Media->Csr14,
         Media->Csr15,
         Media->GpioCtrl,
         Media->GpioData);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockSym(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData)
{
    PDC_MEDIA Media;
    ULONG MediaCode, OpMode;
    USHORT Command;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 7))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    MediaCode = SRomGetMediaCode(*BlockData++);
    if (MediaCode > SROM_MEDIA_MAX)
    {
        WARN("Unknown media code %u\n", MediaCode);
        return NDIS_STATUS_SUCCESS;
    }
    Adapter->MediaBitmap |= 1 << MediaCode;

    Media = &Adapter->Media[MediaCode];

    Media->GpioCtrl = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Media->GpioData = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    Command = DcRetrieveWord(BlockData);
    BlockData += sizeof(USHORT);

    OpMode = Media->OpMode;
    OpMode &= ~SROM_OPMODE_MASK;
    OpMode |= SRomCommandToOpMode(Command);
    Media->OpMode = OpMode;

    SRomNWayAdvertise(Adapter, MediaCode);

    INFO("SYM #%u %s, Command %04lx, Ctrl %04lx, Data %04lx\n",
         MediaCode,
         MediaNumber2Str(Adapter, MediaCode),
         Command,
         Media->GpioCtrl,
         Media->GpioData);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockReset(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData)
{
    UCHAR i, StreamLength;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 1))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    StreamLength = *BlockData++;
    if (StreamLength > SROM_MAX_STREAM_REGS)
    {
        WARN("Too much registers %u\n", StreamLength);
        return NDIS_STATUS_SUCCESS;
    }

    if ((BlockData + StreamLength * 2) > (SRomEnd - 1))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    Adapter->ResetStreamLength = StreamLength;

    for (i = 0; i < StreamLength; ++i)
    {
        Adapter->ResetStream[i] = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);
    }

    INFO("RESET, length %u\n", StreamLength);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomDecodeBlockHmr(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData,
    _In_ UCHAR BlockLength)
{
    ULONG Offset, ExtraData, i;

    PAGED_CODE();

    if (BlockData > (SRomEnd - (2 + 6)))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    Adapter->AnalogControl = DcRetrieveWord(BlockData) << 16;
    BlockData += sizeof(USHORT);

    Adapter->HpnaRegister[HPNA_CONTROL_LOW] = *BlockData++;
    Adapter->HpnaRegister[HPNA_CONTROL_HIGH] = *BlockData++;
    Adapter->HpnaRegister[HPNA_NOISE] = *BlockData++;
    Adapter->HpnaRegister[HPNA_NOISE_FLOOR] = *BlockData++;
    Adapter->HpnaRegister[HPNA_NOISE_CEILING] = *BlockData++;
    Adapter->HpnaRegister[HPNA_NOISE_ATTACK] = *BlockData++;
    Adapter->HpnaInitBitmap |= ((1 << HPNA_CONTROL_LOW) |
                                (1 << HPNA_CONTROL_HIGH) |
                                (1 << HPNA_NOISE) |
                                (1 << HPNA_NOISE_FLOOR) |
                                (1 << HPNA_NOISE_CEILING) |
                                (1 << HPNA_NOISE_ATTACK));

    Offset = 2 /* Length and type fields */ + 2 /* Analog ctrl */ + 6; /* Regs */
    ExtraData = (BlockLength - Offset);

    if ((BlockData + ExtraData) > (SRomEnd - 1))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < ExtraData / sizeof(USHORT); ++i)
    {
        UCHAR RegAddress = SRomHmrRegAddress(*BlockData++);
        UCHAR RegValue = *BlockData++;

        Adapter->HpnaRegister[RegAddress] = RegValue;
        Adapter->HpnaInitBitmap |= 1 << RegAddress;
    }

#if DBG
    INFO_VERB("Analog Ctrl %04lx\n", Adapter->AnalogControl);

    for (i = 0; i < RTL_NUMBER_OF(Adapter->HpnaRegister); ++i)
    {
        if (Adapter->HpnaInitBitmap & (1 << i))
        {
            INFO_VERB("HR Reg %02x = %02x\n", i, Adapter->HpnaRegister[i]);
        }
    }

    if (ExtraData % sizeof(USHORT))
    {
        INFO_VERB("HR Data = %02x\n", *BlockData);
    }
#endif

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomParseExtendedBlock(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData,
    _Out_ PULONG BlockSize)
{
    NDIS_STATUS Status;
    ULONG Length, Type;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 2))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    Length = SRomGetExtendedBlockLength(*BlockData++);
    Type = *BlockData++;

    *BlockSize = Length;

    switch (Type)
    {
        case SROM_BLOCK_TYPE_GPR:
            Status = SRomDecodeBlockGpr(Adapter, SRomEnd, BlockData);
            break;
        case SROM_BLOCK_TYPE_MII_1:
        case SROM_BLOCK_TYPE_MII_2:
            Status = SRomDecodeBlockMii(Adapter,
                                        SRomEnd,
                                        BlockData,
                                        (Type == SROM_BLOCK_TYPE_MII_1));
            break;
        case SROM_BLOCK_TYPE_SIA:
            Status = SRomDecodeBlockSia(Adapter, SRomEnd, BlockData);
            break;
        case SROM_BLOCK_TYPE_SYM:
            Status = SRomDecodeBlockSym(Adapter, SRomEnd, BlockData);
            break;
        case SROM_BLOCK_TYPE_RESET:
            Status = SRomDecodeBlockReset(Adapter, SRomEnd, BlockData);
            break;
        case SROM_BLOCK_TYPE_HOMERUN:
            Status = SRomDecodeBlockHmr(Adapter, SRomEnd, BlockData, Length);
            break;

        /* Skip over the unused or unknown blocks */
        default:
            WARN("Unknown block type %u, length %u\n", Type, Length);
        case SROM_BLOCK_TYPE_PHY_SHUTDOWN:
            Status = NDIS_STATUS_SUCCESS;
            break;
    }

    return Status;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomParse21041Block(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRomEnd,
    _In_ PUCHAR BlockData,
    _Out_ PULONG BlockSize)
{
    PDC_MEDIA Media;
    UCHAR BlockStart;
    ULONG MediaCode;
    BOOLEAN HasExtendedData;

    PAGED_CODE();

    if (BlockData > (SRomEnd - 1))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    BlockStart = *BlockData++;

    HasExtendedData = SRomBlockHasExtendedData(BlockStart);
    if (BlockData > (SRomEnd - (HasExtendedData ? 7 : 1)))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    *BlockSize = HasExtendedData ? 7 : 1;

    MediaCode = SRomGetMediaCode(BlockStart);
    if (MediaCode > SROM_MEDIA_MAX)
    {
        WARN("Unknown media code %u\n", MediaCode);
        return NDIS_STATUS_SUCCESS;
    }
    Adapter->MediaBitmap |= 1 << MediaCode;

    Media = &Adapter->Media[MediaCode];

    if (HasExtendedData)
    {
        Media->Csr13 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);

        Media->Csr14 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);

        Media->Csr15 = DcRetrieveWord(BlockData);
        BlockData += sizeof(USHORT);
    }

    INFO("SIA #%u %s, %sCSR13 %04lx CSR14 %04lx CSR15 %04lx\n",
         MediaCode,
         MediaNumber2Str(Adapter, MediaCode),
         HasExtendedData ? "EXT " : "",
         Media->Csr13,
         Media->Csr14,
         Media->Csr15);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomChecksumValid(
    _In_ PUCHAR SRom)
{
    USHORT Checksum;

    PAGED_CODE();

    Checksum = ~DcEthernetCrc(SRom, SROM_CHECKSUM_V1);
    if (Checksum == DcRetrieveWord(&SRom[SROM_CHECKSUM_V1]))
        return TRUE;

    Checksum = ~DcEthernetCrc(SRom, SROM_CHECKSUM_V2);
    if (Checksum == DcRetrieveWord(&SRom[SROM_CHECKSUM_V2]))
        return TRUE;

    return FALSE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AddressRomChecksumValid(
    _In_reads_bytes_(EAR_SIZE) PVOID AddressRom)
{
    const UCHAR* Octet = AddressRom;
    ULONG64 TestPatterm;
    ULONG Checksum, i;

    PAGED_CODE();

    NdisMoveMemory(&TestPatterm, &Octet[24], 8);
    if (TestPatterm != EAR_TEST_PATTERN)
        return FALSE;

    for (i = 0; i < 8; ++i)
    {
        if (Octet[i] != Octet[15 - i])
            return FALSE;
    }

    Checksum = (Octet[0] << 10) + (Octet[2] << 9) + (Octet[4] << 8) +
               (Octet[1] << 2) + (Octet[3] << 1) + Octet[5];
    Checksum %= 0xFFFF;

    return ((USHORT)Checksum == ((Octet[6] << 8) | Octet[7]));
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomReadMacAddress(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRom,
    _Out_opt_ PULONG AddressOffset)
{
    ULONG MacOffset;

    PAGED_CODE();

    /* Check if we have a board with an old EAR format */
    if (NdisEqualMemory(SRom, &SRom[16], 8))
    {
        /* Validate the EAR checksum */
        if (!AddressRomChecksumValid(SRom))
        {
            ERR("EAR has an invalid checksum\n");
            return FALSE;
        }

        MacOffset = 0;
        goto ReadMac;
    }

    /* Check for a new SROM format */
    if (Adapter->ChipType != DC21040)
    {
        /* Validate the SROM checksum */
        if (SRomChecksumValid(SRom))
        {
            MacOffset = SROM_MAC_ADDRESS;
            goto ReadMac;
        }
    }

    /* Sanity check */
    if (*(PULONG)SRom == 0xFFFFFFFF || *(PULONG)SRom == 0)
        return FALSE;

    WARN("Legacy/unknown board found\n");
    MacOffset = 0;

ReadMac:
    if (AddressOffset)
        *AddressOffset = MacOffset;

    NdisMoveMemory(Adapter->PermanentMacAddress,
                   &SRom[MacOffset],
                   ETH_LENGTH_OF_ADDRESS);

    return TRUE;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomParseHeader(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRom,
    _Out_ PUCHAR* InfoLeaf,
    _Out_ PUCHAR* SRomEnd)
{
    ULONG i, MacOffset, LeafOffset;

    PAGED_CODE();

    if (!SRomReadMacAddress(Adapter, SRom, &MacOffset))
    {
        ERR("Unable to read the MAC address\n");
        return NDIS_STATUS_FAILURE;
    }

    /* Assign our own fake info leaf */
    if (MacOffset != SROM_MAC_ADDRESS)
    {
        for (i = 0; SRompRepairData[i].InfoLeaf; ++i)
        {
            /* Check for a MAC match */
            if (NdisEqualMemory(SRompRepairData[i].InfoLeaf, &Adapter->PermanentMacAddress, 3))
            {
                /* This check is used to distinguish Accton EN1207 from Maxtech */
                if ((Adapter->PermanentMacAddress[0] == 0xE8) && (SRom[0x1A] == 0x55))
                    ++i;

                break;
            }
        }
        if (!SRompRepairData[i].InfoLeaf)
        {
            ERR("Non-standard SROM format, OUI %02x:%02x:%02x\n",
                Adapter->PermanentMacAddress[0],
                Adapter->PermanentMacAddress[1],
                Adapter->PermanentMacAddress[2]);

            return NDIS_STATUS_ADAPTER_NOT_FOUND;
        }

        *InfoLeaf = &SRompRepairData[i].InfoLeaf[3];
        *SRomEnd = *InfoLeaf + SRompRepairData[i].Length;

        /* Update the base address on multiport boards */
        Adapter->PermanentMacAddress[5] += Adapter->ControllerIndex;

#if DBG
        WARN("Non-standard SROM format, using '%s' info leaf\n", SRompRepairData[i].Name);
#endif
        return STATUS_SUCCESS;
    }

    /* Check if the SROM chip is shared between multiple controllers on a multiport board */
    if (SRom[SROM_CONTROLLER_COUNT] > 1)
    {
        INFO("Multiport board, controller number %u (%u/%u)\n",
             Adapter->DeviceNumber,
             Adapter->ControllerIndex + 1,
             SRom[SROM_CONTROLLER_COUNT]);

        for (i = 0; i < SRom[SROM_CONTROLLER_COUNT]; ++i)
        {
            if (SROM_DEVICE_NUMBER(i) >= EE_SIZE)
                return NDIS_STATUS_BUFFER_OVERFLOW;

            if (SRom[SROM_DEVICE_NUMBER(i)] == Adapter->DeviceNumber)
                break;
        }
        if (i == SRom[SROM_CONTROLLER_COUNT])
        {
            ERR("Controller %u was not found in the SROM\n", Adapter->DeviceNumber);
            return NDIS_STATUS_ADAPTER_NOT_FOUND;
        }

        if (SROM_LEAF_OFFSET(i) >= EE_SIZE)
            return NDIS_STATUS_BUFFER_OVERFLOW;

        /* Update the base address */
        Adapter->PermanentMacAddress[5] += i;
    }
    else
    {
        i = 0;
    }

    /* Controller info block offset */
    LeafOffset = DcRetrieveWord(SRom + SROM_LEAF_OFFSET(i));
    if (LeafOffset > (EE_SIZE - sizeof(DC_SROM_COMPACT_BLOCK)))
        return NDIS_STATUS_BUFFER_OVERFLOW;

    /* Controller info leaf */
    *InfoLeaf = &SRom[LeafOffset];

    *SRomEnd = SRom + EE_SIZE;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomParse(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PUCHAR SRom)
{
    ULONG Index, BlockCount, BlockSize, DefaultMedia;
    NDIS_STATUS Status;
    USHORT GpioCtrl;
    PUCHAR Data, SRomEnd;

    PAGED_CODE();

    INFO("SROM Version %u, Controller count %u\n",
         SRom[SROM_VERSION],
         SRom[SROM_CONTROLLER_COUNT]);

    Status = SRomParseHeader(Adapter, SRom, &Data, &SRomEnd);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    DefaultMedia = DcRetrieveWord(Data);
    Data += sizeof(USHORT);

    INFO("Default Media:  %04lx\n", DefaultMedia);

    /* Direction of the GPIO pins */
    if (Adapter->ChipType == DC21140)
    {
        GpioCtrl = *Data++;

        INFO("GPIO Direction: %04lx\n", GpioCtrl);

        GpioCtrl |= DC_GPIO_CONTROL;

        for (Index = 0; Index < MEDIA_LIST_MAX; ++Index)
        {
            Adapter->Media[Index].GpioCtrl = GpioCtrl;
        }

        /* Control word for block type 1 */
        Adapter->MiiMedia.SetupStream[0] = GpioCtrl;
    }

    BlockCount = *Data++;

    INFO("Block Count:    %u\n", BlockCount);

    if (BlockCount == 0 || BlockCount == 0xFF)
    {
        WARN("No media information found\n");
        return NDIS_STATUS_SUCCESS;
    }

    /* Analyze and decode blocks */
    for (Index = 0; Index < BlockCount; ++Index)
    {
        if (Adapter->ChipType == DC21041)
        {
            Status = SRomParse21041Block(Adapter, SRomEnd, Data, &BlockSize);
        }
        else
        {
            if (Data > (SRomEnd - 1))
                return NDIS_STATUS_BUFFER_OVERFLOW;

            if (SRomIsBlockExtended(*Data))
            {
                Status = SRomParseExtendedBlock(Adapter, SRomEnd, Data, &BlockSize);
            }
            else
            {
                Status = SRomDecodeBlockGpr(Adapter, SRomEnd, Data);
                BlockSize = 4;
            }
        }
        if (Status != NDIS_STATUS_SUCCESS)
            return Status;

        Data += BlockSize;
    }

    if ((Adapter->MediaBitmap == 0) && !(Adapter->Features & DC_HAS_MII))
    {
        WARN("No media information found\n");
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
SRomShiftOut(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Sequence,
    _In_ ULONG BitCount)
{
    LONG i;

    PAGED_CODE();

    for (i = BitCount - 1; i >= 0; --i)
    {
        ULONG DataIn = ((Sequence >> i) & 1) << DC_SERIAL_EE_DI_SHIFT;

        SROM_WRITE(Adapter, DataIn | DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS);
        SROM_WRITE(Adapter, DataIn | DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS |
                            DC_SERIAL_EE_SK);
        SROM_WRITE(Adapter, DataIn | DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS);
    }
}

static
CODE_SEG("PAGE")
USHORT
SRomShiftIn(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i, Csr;
    USHORT SerialData;

    PAGED_CODE();

    /* Shift the data out of the EEPROM */
    SerialData = 0;
    for (i = 0; i < RTL_BITS_OF(USHORT); ++i)
    {
        SROM_WRITE(Adapter, DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS | DC_SERIAL_EE_SK);

        SROM_READ(Adapter, &Csr);
        SerialData = (SerialData << 1) | ((Csr >> DC_SERIAL_EE_DO_SHIFT) & 1);

        SROM_WRITE(Adapter, DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS);
    }

    /* End the read cycle */
    SROM_WRITE(Adapter, DC_SERIAL_EE_RD | DC_SERIAL_EE_SR);

    return SerialData;
}

static
CODE_SEG("PAGE")
ULONG
SRomDetectAddressBusWidth(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG Csr, BusWidth;

    PAGED_CODE();

    /* Assume the SROM is a 1kB ROM, send the read command and zero address (6 bits) */
    SRomShiftOut(Adapter, EEPROM_CMD_READ << 6, EEPROM_CMD_LENGTH + 6);

    /* Check the preceding dummy zero bit */
    Csr = DC_READ(Adapter, DcCsr9_SerialInterface);
    if (Csr & DC_SERIAL_EE_DO)
    {
        /* 4kB EEPROM */
        BusWidth = 8;

        /* Send the remaining part of the address */
        SRomShiftOut(Adapter, 0, 8 - 6);

        /* The preceding dummy bit must be zero */
        Csr = DC_READ(Adapter, DcCsr9_SerialInterface);
        if (Csr & DC_SERIAL_EE_DO)
            return 0;
    }
    else
    {
        /* 1kB EEPROM */
        BusWidth = 6;
    }

    /* Complete the read cycle */
    (VOID)SRomShiftIn(Adapter);

    return BusWidth;
}

static
CODE_SEG("PAGE")
BOOLEAN
SRomReadSRom(
    _In_ PDC21X4_ADAPTER Adapter,
    _Out_writes_all_(EE_SIZE) PVOID SRom)
{
    PUSHORT SRomWord = SRom;
    BOOLEAN Success = TRUE;
    ULONG BusWidth, Address;

    PAGED_CODE();

    /* Select the device */
    SROM_WRITE(Adapter, DC_SERIAL_EE_RD | DC_SERIAL_EE_SR);
    SROM_WRITE(Adapter, DC_SERIAL_EE_RD | DC_SERIAL_EE_SR | DC_SERIAL_EE_CS);

    BusWidth = SRomDetectAddressBusWidth(Adapter);
    if (BusWidth == 0)
    {
        Success = FALSE;
        goto Done;
    }
    INFO("SROM Bus width: %u\n", BusWidth);

    /* Read the SROM contents once */
    for (Address = 0; Address < (EE_SIZE / sizeof(USHORT)); ++Address)
    {
        /* Send the command and address */
        SRomShiftOut(Adapter,
                     (EEPROM_CMD_READ << BusWidth) | Address,
                     EEPROM_CMD_LENGTH + BusWidth);

        /* Read the data */
        SRomWord[Address] = SRomShiftIn(Adapter);
    }

Done:
    /* End chip select */
    DC_WRITE(Adapter, DcCsr9_SerialInterface, 0);

    return Success;
}

#if DBG
static
CODE_SEG("PAGE")
VOID
SRomDumpContents(
    _In_reads_bytes_(Length) const VOID* Buffer,
    _In_ ULONG Length)
{
    ULONG Offset, Count, i;
    const UCHAR* Data = Buffer;

    PAGED_CODE();

    DbgPrint("SROM data:\n");

    Offset = 0;
    while (Offset < Length)
    {
        DbgPrint("%04x:\t", Offset);

        Count = min(Length - Offset, 16);
        for (i = 0; i < Count; ++i, ++Offset)
        {
            DbgPrint("0x%02x, ", Data[Offset], (i == 7) ? '-' : ' ');
        }

        DbgPrint("\n");
    }
}
#endif // DBG

static
CODE_SEG("PAGE")
NDIS_STATUS
SRomRead(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_SROM_ENTRY SRomEntry;
    NDIS_STATUS Status;
    BOOLEAN ReleaseImage;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&SRomEntry,
                                       FIELD_OFFSET(DC_SROM_ENTRY, SRomImage[EE_SIZE]),
                                       DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return NDIS_STATUS_RESOURCES;
    NdisZeroMemory(SRomEntry, FIELD_OFFSET(DC_SROM_ENTRY, SRomImage));

    ReleaseImage = FALSE;

    if (SRomReadSRom(Adapter, SRomEntry->SRomImage))
    {
        if (!SRomRegisterMasterAdapter(Adapter, SRomEntry))
            ReleaseImage = TRUE;
    }
    else
    {
        NdisFreeMemory(SRomEntry, 0, 0);

        if (!SRomFindMasterAdapter(Adapter, &SRomEntry))
        {
            ERR("Failed to retrieve the SROM contents\n");
            return NDIS_STATUS_FAILURE;
        }
    }

    Status = SRomParse(Adapter, SRomEntry->SRomImage);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Failed to parse SROM\n");
    }

#if DBG
    if (Status != NDIS_STATUS_SUCCESS)
        SRomDumpContents(SRomEntry->SRomImage, EE_SIZE);
#endif

    if (ReleaseImage)
        NdisFreeMemory(SRomEntry, 0, 0);

    return Status;
}

static
CODE_SEG("PAGE")
BOOLEAN
AddressRomReadData(
    _In_ PDC21X4_ADAPTER Adapter,
    _Out_writes_all_(EAR_SIZE) PUCHAR AddressRom)
{
    ULONG Data, i, j;

    PAGED_CODE();

    /* Reset the ROM pointer */
    DC_WRITE(Adapter, DcCsr9_SerialInterface, 0);

    for (i = 0; i < EAR_SIZE; ++i)
    {
        for (j = 10000; j > 0; --j)
        {
            NdisStallExecution(1);
            Data = DC_READ(Adapter, DcCsr9_SerialInterface);

            if (!(Data & DC_SERIAL_EAR_DN))
                break;
        }
        AddressRom[i] = Data & DC_SERIAL_EAR_DT;
    }

    if (SRomIsEmpty(AddressRom, EAR_SIZE))
        return FALSE;

    return TRUE;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
AddressRomRead(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_SROM_ENTRY SRomEntry;
    NDIS_STATUS Status;
    BOOLEAN ReleaseImage;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&SRomEntry,
                                       FIELD_OFFSET(DC_SROM_ENTRY, SRomImage[EAR_SIZE]),
                                       DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return NDIS_STATUS_RESOURCES;
    NdisZeroMemory(SRomEntry, FIELD_OFFSET(DC_SROM_ENTRY, SRomImage));

    ReleaseImage = FALSE;

    if (AddressRomReadData(Adapter, SRomEntry->SRomImage))
    {
        if (!SRomRegisterMasterAdapter(Adapter, SRomEntry))
            ReleaseImage = TRUE;
    }
    else
    {
        NdisFreeMemory(SRomEntry, 0, 0);

        if (!SRomFindMasterAdapter(Adapter, &SRomEntry))
        {
            ERR("Failed to retrieve the EAR contents\n");
            return NDIS_STATUS_FAILURE;
        }
    }

    if (!SRomReadMacAddress(Adapter, SRomEntry->SRomImage, NULL))
    {
        ERR("Unable to read the MAC address\n");
        Status = NDIS_STATUS_FAILURE;
    }

    /* Update the base address on multiport boards */
    Adapter->PermanentMacAddress[5] += Adapter->ControllerIndex;

#if DBG
    if (Status != NDIS_STATUS_SUCCESS)
        SRomDumpContents(SRomEntry->SRomImage, EAR_SIZE);
#endif

    if (ReleaseImage)
        NdisFreeMemory(SRomEntry, 0, 0);

    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

CODE_SEG("PAGE")
VOID
DcFreeEeprom(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_SROM_ENTRY SRomEntry;

    PAGED_CODE();

    SRomEntry = Adapter->SRomEntry;
    if (!SRomEntry)
        return;

    SRomAcquireListMutex();

    /* Unregister the port */
    SRomEntry->DeviceBitmap &= ~(1 << Adapter->DeviceNumber);

    /*
     * Free the SROM as soon as the last registered port has removed.
     * We can't free it in an unload handler
     * as the bus numbers can be changed by a resource rebalance.
     */
    if (SRomEntry->DeviceBitmap == 0)
    {
        INFO("Freeing SROM %p at %u:%u\n",
             SRomEntry,
             SRomEntry->BusNumber,
             SRomEntry->DeviceNumber);

        RemoveEntryList(&SRomEntry->ListEntry);

        NdisFreeMemory(SRomEntry, 0, 0);
    }

    SRomReleaseListMutex();
}

CODE_SEG("PAGE")
NDIS_STATUS
DcReadEeprom(
    _In_ PDC21X4_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    PAGED_CODE();

    if (Adapter->ChipType == DC21040)
    {
        /* Ethernet Address ROM */
        Status = AddressRomRead(Adapter);
    }
    else
    {
        /* MicroWire Compatible Serial EEPROM */
        Status = SRomRead(Adapter);
    }

    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    INFO("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         Adapter->PermanentMacAddress[0],
         Adapter->PermanentMacAddress[1],
         Adapter->PermanentMacAddress[2],
         Adapter->PermanentMacAddress[3],
         Adapter->PermanentMacAddress[4],
         Adapter->PermanentMacAddress[5]);

    if (ETH_IS_BROADCAST(Adapter->PermanentMacAddress) ||
        ETH_IS_EMPTY(Adapter->PermanentMacAddress) ||
        ETH_IS_MULTICAST(Adapter->PermanentMacAddress))
    {
        ERR("Invalid permanent MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
            Adapter->PermanentMacAddress[0],
            Adapter->PermanentMacAddress[1],
            Adapter->PermanentMacAddress[2],
            Adapter->PermanentMacAddress[3],
            Adapter->PermanentMacAddress[4],
            Adapter->PermanentMacAddress[5]);

        NdisWriteErrorLogEntry(Adapter->AdapterHandle, NDIS_ERROR_CODE_NETWORK_ADDRESS, 0);

        return NDIS_STATUS_INVALID_ADDRESS;
    }

    return NDIS_STATUS_SUCCESS;
}
