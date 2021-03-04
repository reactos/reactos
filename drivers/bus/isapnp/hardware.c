/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Hardware support code
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

static
inline
VOID
WriteAddress(
    _In_ USHORT Address)
{
    WRITE_PORT_UCHAR((PUCHAR)ISAPNP_ADDRESS, Address);
}

static
inline
VOID
WriteData(
    _In_ USHORT Data)
{
    WRITE_PORT_UCHAR((PUCHAR)ISAPNP_WRITE_DATA, Data);
}

static
inline
UCHAR
ReadData(
    _In_ PUCHAR ReadDataPort)
{
    return READ_PORT_UCHAR(ReadDataPort);
}

static
inline
VOID
WriteByte(
    _In_ USHORT Address,
    _In_ USHORT Value)
{
    WriteAddress(Address);
    WriteData(Value);
}

static
inline
UCHAR
ReadByte(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Address)
{
    WriteAddress(Address);
    return ReadData(ReadDataPort);
}

static
inline
USHORT
ReadWord(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Address)
{
    return ((ReadByte(ReadDataPort, Address) << 8) |
            (ReadByte(ReadDataPort, Address + 1)));
}

static
inline
VOID
SetReadDataPort(
    _In_ PUCHAR ReadDataPort)
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
    _In_ USHORT Csn)
{
    WriteByte(ISAPNP_WAKE, Csn);
}

static
inline
USHORT
ReadResourceData(
    _In_ PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_RESOURCEDATA);
}

static
inline
USHORT
ReadStatus(
    _In_ PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_STATUS);
}

static
inline
VOID
WriteCsn(
    _In_ USHORT Csn)
{
    WriteByte(ISAPNP_CARDSELECTNUMBER, Csn);
}

static
inline
VOID
WriteLogicalDeviceNumber(
    _In_ USHORT LogDev)
{
    WriteByte(ISAPNP_LOGICALDEVICENUMBER, LogDev);
}

static
inline
VOID
ActivateDevice(
    _In_ USHORT LogDev)
{
    WriteLogicalDeviceNumber(LogDev);
    WriteByte(ISAPNP_ACTIVATE, 1);
}

static
inline
VOID
DeactivateDevice(
    _In_ USHORT LogDev)
{
    WriteLogicalDeviceNumber(LogDev);
    WriteByte(ISAPNP_ACTIVATE, 0);
}

static
inline
USHORT
ReadIoBase(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Index)
{
    return ReadWord(ReadDataPort, ISAPNP_IOBASE(Index));
}

static
inline
USHORT
ReadIrqNo(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Index)
{
    return ReadByte(ReadDataPort, ISAPNP_IRQNO(Index));
}

static
inline
USHORT
ReadIrqType(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Index)
{
    return ReadByte(ReadDataPort, ISAPNP_IRQTYPE(Index));
}

static
inline
USHORT
ReadDmaChannel(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT Index)
{
    return ReadByte(ReadDataPort, ISAPNP_DMACHANNEL(Index));
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
    _In_ UCHAR Lfsr,
    _In_ UCHAR InputBit)
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
    _In_ PUCHAR ReadDataPort)
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
    _In_ PUCHAR ReadDataPort,
    _Out_writes_bytes_all_opt_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    USHORT i, Byte;

    for (i = 0; i < Length; i++)
    {
        Byte = PeekByte(ReadDataPort);
        if (Buffer)
            ((PUCHAR)Buffer)[i] = Byte;
    }
}

static
USHORT
IsaPnpChecksum(
    _In_ PISAPNP_IDENTIFIER Identifier)
{
    UCHAR i, j, Lfsr;

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < FIELD_OFFSET(ISAPNP_IDENTIFIER, Checksum); i++)
    {
        UCHAR Byte = ((PUCHAR)Identifier)[i];

        for (j = 0; j < RTL_BITS_OF(Byte); j++)
        {
            Lfsr = NextLFSR(Lfsr, Byte);
            Byte >>= 1;
        }
    }

    return Lfsr;
}

static
BOOLEAN
ReadTags(
    _In_ PUCHAR ReadDataPort,
    _In_ USHORT LogDev,
    _Inout_ PISAPNP_LOGICAL_DEVICE LogDevice)
{
    BOOLEAN res = FALSE;
    PVOID Buffer;
    USHORT Tag, TagLen, MaxLen;
    ULONG NumberOfIo = 0, NumberOfIrq = 0, NumberOfDma = 0;

    LogDev += 1;

    while (TRUE)
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
        if (Tag == ISAPNP_TAG_END)
            break;

        Buffer = NULL;
        if (Tag == ISAPNP_TAG_LOGDEVID)
        {
            MaxLen = sizeof(LogDevice->LogDevId);
            Buffer = &LogDevice->LogDevId;
            LogDev--;
        }
        else if (Tag == ISAPNP_TAG_IRQ && NumberOfIrq < ARRAYSIZE(LogDevice->Irq))
        {
            MaxLen = sizeof(LogDevice->Irq[NumberOfIrq].Description);
            Buffer = &LogDevice->Irq[NumberOfIrq].Description;
            NumberOfIrq++;
        }
        else if (Tag == ISAPNP_TAG_IOPORT && NumberOfIo < ARRAYSIZE(LogDevice->Io))
        {
            MaxLen = sizeof(LogDevice->Io[NumberOfIo].Description);
            Buffer = &LogDevice->Io[NumberOfIo].Description;
            NumberOfIo++;
        }
        else if (Tag == ISAPNP_TAG_DMA && NumberOfDma < ARRAYSIZE(LogDevice->Dma))
        {
            MaxLen = sizeof(LogDevice->Dma[NumberOfDma].Description);
            Buffer = &LogDevice->Dma[NumberOfDma].Description;
            NumberOfDma++;
        }
        else if (LogDev == 0)
        {
            DPRINT1("Found unknown tag 0x%x (len %d)\n", Tag, TagLen);
        }

        if (Buffer && LogDev == 0)
        {
            res = TRUE;
            if (MaxLen > TagLen)
            {
                Peek(ReadDataPort, Buffer, TagLen);
            }
            else
            {
                Peek(ReadDataPort, Buffer, MaxLen);
                Peek(ReadDataPort, NULL, TagLen - MaxLen);
            }
        }
        else
        {
            /* We don't want to read informations on this
             * logical device, or we don't know the tag. */
            Peek(ReadDataPort, NULL, TagLen);
        }
    };

    return res;
}

static
INT
TryIsolate(
    _In_ PUCHAR ReadDataPort)
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
    _In_ PISAPNP_LOGICAL_DEVICE IsaDevice,
    _In_ BOOLEAN Activate)
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
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDevice;
    ISAPNP_IDENTIFIER Identifier;
    USHORT Csn;
    USHORT LogDev;
    ULONG i;

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

            if (!ReadTags(FdoExt->ReadDataPort, LogDev, LogDevice))
                break;

            WriteLogicalDeviceNumber(LogDev);

            LogDevice->VendorId[0] = ((LogDevice->LogDevId.VendorId >> 2) & 0x1f) + 'A' - 1,
            LogDevice->VendorId[1] = (((LogDevice->LogDevId.VendorId & 0x3) << 3) | ((LogDevice->LogDevId.VendorId >> 13) & 0x7)) + 'A' - 1,
            LogDevice->VendorId[2] = ((LogDevice->LogDevId.VendorId >> 8) & 0x1f) + 'A' - 1,
            LogDevice->ProdId = RtlUshortByteSwap(LogDevice->LogDevId.ProdId);
            LogDevice->SerialNumber = Identifier.Serial;
            for (i = 0; i < ARRAYSIZE(LogDevice->Io); i++)
                LogDevice->Io[i].CurrentBase = ReadIoBase(FdoExt->ReadDataPort, i);
            for (i = 0; i < ARRAYSIZE(LogDevice->Irq); i++)
            {
                LogDevice->Irq[i].CurrentNo = ReadIrqNo(FdoExt->ReadDataPort, i);
                LogDevice->Irq[i].CurrentType = ReadIrqType(FdoExt->ReadDataPort, i);
            }
            for (i = 0; i < ARRAYSIZE(LogDevice->Dma); i++)
            {
                LogDevice->Dma[i].CurrentChannel = ReadDmaChannel(FdoExt->ReadDataPort, i);
            }

            DPRINT1("Detected ISA PnP device - VID: '%3s' PID: 0x%x SN: 0x%08x IoBase: 0x%x IRQ:0x%x\n",
                    LogDevice->VendorId, LogDevice->ProdId, LogDevice->SerialNumber, LogDevice->Io[0].CurrentBase, LogDevice->Irq[0].CurrentNo);

            WaitForKey();

            InsertTailList(&FdoExt->DeviceListHead, &LogDevice->DeviceLink);
            FdoExt->DeviceCount++;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwTryReadDataPort(
    _In_ PUCHAR ReadDataPort)
{
    return TryIsolate(ReadDataPort) > 0 ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
IsaHwActivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    DeviceActivation(LogicalDevice,
                     TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwDeactivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    DeviceActivation(LogicalDevice,
                     FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwFillDeviceList(
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    return ProbeIsaPnpBus(FdoExt);
}
