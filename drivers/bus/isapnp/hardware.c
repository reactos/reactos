/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            hardware.c
 * PURPOSE:         Hardware support code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <isapnp.h>
#include <isapnphw.h>

#define NDEBUG
#include <debug.h>

static
inline
VOID
WriteAddress(
    IN USHORT Address)
{
    WRITE_PORT_UCHAR((PUCHAR)ISAPNP_ADDRESS, Address);
}

static
inline
VOID
WriteData(
    IN USHORT Data)
{
    WRITE_PORT_UCHAR((PUCHAR)ISAPNP_WRITE_DATA, Data);
}

static
inline
UCHAR
ReadData(
    IN PUCHAR ReadDataPort)
{
    return READ_PORT_UCHAR(ReadDataPort);
}

static
inline
VOID
WriteByte(
    IN USHORT Address,
    IN USHORT Value)
{
    WriteAddress(Address);
    WriteData(Value);
}

static
inline
UCHAR
ReadByte(
    IN PUCHAR ReadDataPort,
    IN USHORT Address)
{
    WriteAddress(Address);
    return ReadData(ReadDataPort);
}

static
inline
USHORT
ReadWord(
    IN PUCHAR ReadDataPort,
    IN USHORT Address)
{
    return ((ReadByte(ReadDataPort, Address) << 8) |
            (ReadByte(ReadDataPort, Address + 1)));
}

static
inline
VOID
SetReadDataPort(
    IN PUCHAR ReadDataPort)
{
    WriteByte(ISAPNP_READPORT, ((ULONG_PTR)ReadDataPort >> 2));
}

static
inline
VOID
EnterIsolationState(VOID)
{
    WriteAddress(ISAPNP_SERIALISOLATION);
}

static
inline
VOID
WaitForKey(VOID)
{
    WriteByte(ISAPNP_CONFIGCONTROL, ISAPNP_CONFIG_WAIT_FOR_KEY);
}

static
inline
VOID
ResetCsn(VOID)
{
    WriteByte(ISAPNP_CONFIGCONTROL, ISAPNP_CONFIG_RESET_CSN);
}

static
inline
VOID
Wake(
    IN USHORT Csn)
{
    WriteByte(ISAPNP_WAKE, Csn);
}

static
inline
USHORT
ReadResourceData(
    IN PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_RESOURCEDATA);
}

static
inline
USHORT
ReadStatus(
    IN PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_STATUS);
}

static
inline
VOID
WriteCsn(
    IN USHORT Csn)
{
    WriteByte(ISAPNP_CARDSELECTNUMBER, Csn);
}

static
inline
VOID
WriteLogicalDeviceNumber(
    IN USHORT LogDev)
{
    WriteByte(ISAPNP_LOGICALDEVICENUMBER, LogDev);
}

static
inline
VOID
ActivateDevice(
    IN USHORT LogDev)
{
    WriteLogicalDeviceNumber(LogDev);
    WriteByte(ISAPNP_ACTIVATE, 1);
}

static
inline
VOID
DeactivateDevice(
    IN USHORT LogDev)
{
    WriteLogicalDeviceNumber(LogDev);
    WriteByte(ISAPNP_ACTIVATE, 0);
}

static
inline
USHORT
ReadIoBase(
    IN PUCHAR ReadDataPort,
    IN USHORT Index)
{
    return ReadWord(ReadDataPort, ISAPNP_IOBASE(Index));
}

static
inline
USHORT
ReadIrqNo(
    IN PUCHAR ReadDataPort,
    IN USHORT Index)
{
    return ReadByte(ReadDataPort, ISAPNP_IRQNO(Index));
}

static
inline
VOID
HwDelay(VOID)
{
    KeStallExecutionProcessor(1000);
}

static
inline
UCHAR
NextLFSR(
    IN UCHAR Lfsr,
    IN UCHAR InputBit)
{
    UCHAR NextLfsr = Lfsr >> 1;

    NextLfsr |= (((Lfsr ^ NextLfsr) ^ InputBit)) << 7;

    return NextLfsr;
}

static
VOID
SendKey(VOID)
{
    UCHAR i, Lfsr;

    HwDelay();
    WriteAddress(0x00);
    WriteAddress(0x00);

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < 32; i++)
    {
        WriteAddress(Lfsr);
        Lfsr = NextLFSR(Lfsr, 0);
    }
}

static
USHORT
PeekByte(
    IN PUCHAR ReadDataPort)
{
    USHORT i;

    for (i = 0; i < 20; i++)
    {
        if (ReadStatus(ReadDataPort) & 0x01)
            return ReadResourceData(ReadDataPort);

        HwDelay();
    }

    return 0xFF;
}

static
VOID
Peek(
    IN PUCHAR ReadDataPort,
    IN OUT PVOID Buffer,
    IN ULONG Length)
{
    USHORT i, Byte;

    for (i = 0; i < Length; i++)
    {
        Byte = PeekByte(ReadDataPort);
        if (Buffer)
            *((PUCHAR)Buffer + i) = Byte;
    }
}

static
USHORT
IsaPnpChecksum(
    IN PISAPNP_IDENTIFIER Identifier)
{
    UCHAR i, j, Lfsr, Byte;

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < 8; i++)
    {
        Byte = *(((PUCHAR)Identifier) + i);
        for (j = 0; j < 8; j++)
        {
            Lfsr = NextLFSR(Lfsr, Byte);
            Byte >>= 1;
        }
    }

    return Lfsr;
}

static
BOOLEAN
FindTag(
    IN PUCHAR ReadDataPort,
    IN USHORT WantedTag,
    IN OUT PVOID Buffer,
    IN ULONG Length)
{
    USHORT Tag, TagLen;

    do
    {
        Tag = PeekByte(ReadDataPort);
        if (ISAPNP_IS_SMALL_TAG(Tag))
        {
            TagLen = ISAPNP_SMALL_TAG_LEN(Tag);
            Tag = ISAPNP_SMALL_TAG_NAME(Tag);
        }
        else
        {
            TagLen = PeekByte(ReadDataPort) + (PeekByte(ReadDataPort) << 8);
            Tag = ISAPNP_LARGE_TAG_NAME(Tag);
        }

        if (Tag == WantedTag)
        {
            if (Length > TagLen)
                Length = TagLen;

            Peek(ReadDataPort, Buffer, Length);

            return TRUE;
        }
        else
        {
            Peek(ReadDataPort, NULL, TagLen);
        }
    } while (Tag != ISAPNP_TAG_END);

    return FALSE;
}

static
BOOLEAN
FindLogDevId(
    IN PUCHAR ReadDataPort,
    IN USHORT LogDev,
    IN OUT PISAPNP_LOGDEVID LogDeviceId)
{
    USHORT i;

    for (i = 0; i <= LogDev; i++)
    {
        if (!FindTag(ReadDataPort, ISAPNP_TAG_LOGDEVID, LogDeviceId, sizeof(*LogDeviceId)))
            return FALSE;
    }

    return TRUE;
}

static
INT
TryIsolate(
    IN PUCHAR ReadDataPort)
{
    ISAPNP_IDENTIFIER Identifier;
    USHORT i, j;
    BOOLEAN Seen55aa, SeenLife;
    INT Csn = 0;
    USHORT Byte, Data;

    DPRINT("Setting read data port: 0x%p\n", ReadDataPort);

    WaitForKey();
    SendKey();

    ResetCsn();
    HwDelay();
    HwDelay();

    WaitForKey();
    SendKey();
    Wake(0x00);

    SetReadDataPort(ReadDataPort);
    HwDelay();

    while (TRUE)
    {
        EnterIsolationState();
        HwDelay();

        RtlZeroMemory(&Identifier, sizeof(Identifier));

        Seen55aa = SeenLife = FALSE;
        for (i = 0; i < 9; i++)
        {
            Byte = 0;
            for (j = 0; j < 8; j++)
            {
                Data = ReadData(ReadDataPort);
                HwDelay();
                Data = ((Data << 8) | ReadData(ReadDataPort));
                HwDelay();
                Byte >>= 1;

                if (Data != 0xFFFF)
                {
                    SeenLife = TRUE;
                    if (Data == 0x55AA)
                    {
                        Byte |= 0x80;
                        Seen55aa = TRUE;
                    }
                }
            }
            *(((PUCHAR)&Identifier) + i) = Byte;
        }

        if (!Seen55aa)
        {
            if (Csn)
            {
                DPRINT("Found no more cards\n");
            }
            else
            {
                if (SeenLife)
                {
                    DPRINT("Saw life but no cards, trying new read port\n");
                    Csn = -1;
                }
                else
                {
                    DPRINT("Saw no sign of life, abandoning isolation\n");
                }
            }
            break;
        }

        if (Identifier.Checksum != IsaPnpChecksum(&Identifier))
        {
            DPRINT("Bad checksum, trying next read data port\n");
            Csn = -1;
            break;
        }

        Csn++;

        WriteCsn(Csn);
        HwDelay();

        Wake(0x00);
        HwDelay();
    }

    WaitForKey();

    if (Csn > 0)
    {
        DPRINT("Found %d cards at read port 0x%p\n", Csn, ReadDataPort);
    }

    return Csn;
}

VOID
DeviceActivation(
    IN PISAPNP_LOGICAL_DEVICE IsaDevice,
    IN BOOLEAN Activate)
{
    WaitForKey();
    SendKey();
    Wake(IsaDevice->CSN);

    if (Activate)
        ActivateDevice(IsaDevice->LDN);
    else
        DeactivateDevice(IsaDevice->LDN);

    HwDelay();

    WaitForKey();
}

NTSTATUS
ProbeIsaPnpBus(
    IN PISAPNP_FDO_EXTENSION FdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDevice;
    ISAPNP_IDENTIFIER Identifier;
    ISAPNP_LOGDEVID LogDevId;
    USHORT Csn;
    USHORT LogDev;

    ASSERT(FdoExt->ReadDataPort);

    for (Csn = 1; Csn <= 0xFF; Csn++)
    {
        for (LogDev = 0; LogDev <= 0xFF; LogDev++)
        {
            LogDevice = ExAllocatePool(NonPagedPool, sizeof(ISAPNP_LOGICAL_DEVICE));
            if (!LogDevice)
                return STATUS_NO_MEMORY;

            RtlZeroMemory(LogDevice, sizeof(ISAPNP_LOGICAL_DEVICE));

            LogDevice->CSN = Csn;
            LogDevice->LDN = LogDev;

            WaitForKey();
            SendKey();
            Wake(Csn);

            Peek(FdoExt->ReadDataPort, &Identifier, sizeof(Identifier));

            if (Identifier.VendorId & 0x80)
            {
                ExFreePool(LogDevice);
                return STATUS_SUCCESS;
            }

            if (!FindLogDevId(FdoExt->ReadDataPort, LogDev, &LogDevId))
                break;

            WriteLogicalDeviceNumber(LogDev);

            LogDevice->VendorId[0] = ((LogDevId.VendorId >> 2) & 0x1f) + 'A' - 1,
            LogDevice->VendorId[1] = (((LogDevId.VendorId & 0x3) << 3) | ((LogDevId.VendorId >> 13) & 0x7)) + 'A' - 1,
            LogDevice->VendorId[2] = ((LogDevId.VendorId >> 8) & 0x1f) + 'A' - 1,
            LogDevice->ProdId = RtlUshortByteSwap(LogDevId.ProdId);
            LogDevice->SerialNumber = Identifier.Serial;
            LogDevice->IoAddr = ReadIoBase(FdoExt->ReadDataPort, 0);
            LogDevice->IrqNo = ReadIrqNo(FdoExt->ReadDataPort, 0);

            DPRINT1("Detected ISA PnP device - VID: '%3s' PID: 0x%x SN: 0x%08x IoBase: 0x%x IRQ:0x%x\n",
                    LogDevice->VendorId, LogDevice->ProdId, LogDevice->SerialNumber, LogDevice->IoAddr, LogDevice->IrqNo);

            WaitForKey();

            InsertTailList(&FdoExt->DeviceListHead, &LogDevice->ListEntry);
            FdoExt->DeviceCount++;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwTryReadDataPort(
  IN PUCHAR ReadDataPort)
{
    return TryIsolate(ReadDataPort) > 0 ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
IsaHwActivateDevice(
  IN PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
  DeviceActivation(LogicalDevice,
                   TRUE);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwDeactivateDevice(
    IN PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    DeviceActivation(LogicalDevice,
                     FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwFillDeviceList(
    IN PISAPNP_FDO_EXTENSION FdoExt)
{
    return ProbeIsaPnpBus(FdoExt);
}
