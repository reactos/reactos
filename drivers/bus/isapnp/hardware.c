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
WriteAddress(USHORT Address)
{
  WRITE_PORT_UCHAR((PUCHAR)ISAPNP_ADDRESS, Address);
}

static
inline
VOID
WriteData(USHORT Data)
{
  WRITE_PORT_UCHAR((PUCHAR)ISAPNP_WRITE_DATA, Data);
}

static
inline
UCHAR
ReadData(PUCHAR ReadDataPort)
{
  return READ_PORT_UCHAR(ReadDataPort);
}

static
inline
VOID
WriteByte(USHORT Address, USHORT Value)
{
  WriteAddress(Address);
  WriteData(Value);
}

static
inline
UCHAR
ReadByte(PUCHAR ReadDataPort, USHORT Address)
{
  WriteAddress(Address);
  return ReadData(ReadDataPort);
}

static
inline
USHORT
ReadWord(PUCHAR ReadDataPort, USHORT Address)
{
  return ((ReadByte(ReadDataPort, Address) << 8) |
          (ReadByte(ReadDataPort, Address + 1)));
}

static
inline
VOID
SetReadDataPort(PUCHAR ReadDataPort)
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
Wake(USHORT Csn)
{
  WriteByte(ISAPNP_WAKE, Csn);
}

static
inline
USHORT
ReadResourceData(PUCHAR ReadDataPort)
{
  return ReadByte(ReadDataPort, ISAPNP_RESOURCEDATA);
}

static
inline
USHORT
ReadStatus(PUCHAR ReadDataPort)
{
  return ReadByte(ReadDataPort, ISAPNP_STATUS);
}

static
inline
VOID
WriteCsn(USHORT Csn)
{
  WriteByte(ISAPNP_CARDSELECTNUMBER, Csn);
}

static
inline
VOID
WriteLogicalDeviceNumber(USHORT LogDev)
{
  WriteByte(ISAPNP_LOGICALDEVICENUMBER, LogDev);
}

static
inline
VOID
ActivateDevice(USHORT LogDev)
{
  WriteLogicalDeviceNumber(LogDev);
  WriteByte(ISAPNP_ACTIVATE, 1);
}

static
inline
VOID
DeactivateDevice(USHORT LogDev)
{
  WriteLogicalDeviceNumber(LogDev);
  WriteByte(ISAPNP_ACTIVATE, 0);
}

static
inline
USHORT
ReadIoBase(PUCHAR ReadDataPort, USHORT Index)
{
  return ReadWord(ReadDataPort, ISAPNP_IOBASE(Index));
}

static
inline
USHORT
ReadIrqNo(PUCHAR ReadDataPort, USHORT Index)
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
USHORT
NextLFSR(USHORT Lfsr, USHORT InputBit)
{
  ULONG NextLfsr = Lfsr >> 1;

  NextLfsr |= (((Lfsr ^ NextLfsr) ^ InputBit)) << 7;

  return NextLfsr;
}

static
VOID
SendKey(VOID)
{
  USHORT i, Lfsr;

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
PeekByte(PUCHAR ReadDataPort)
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
Peek(PUCHAR ReadDataPort, PVOID Buffer, ULONG Length)
{
  USHORT i, byte;

  for (i = 0; i < Length; i++)
  {
    byte = PeekByte(ReadDataPort);
    if (Buffer)
       *((PUCHAR)Buffer + i) = byte;
  }
}

static
USHORT
IsaPnpChecksum(PISAPNP_IDENTIFIER Identifier)
{
  USHORT i,j, Lfsr, Byte;

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
FindTag(PUCHAR ReadDataPort, USHORT WantedTag, PVOID Buffer, ULONG Length)
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
      Peek(ReadDataPort, NULL, Length);
    }
  } while (Tag != ISAPNP_TAG_END);

  return FALSE;
}

static
BOOLEAN
FindLogDevId(PUCHAR ReadDataPort, USHORT LogDev, PISAPNP_LOGDEVID LogDeviceId)
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
TryIsolate(PUCHAR ReadDataPort)
{
  ISAPNP_IDENTIFIER Identifier;
  USHORT i, j;
  BOOLEAN Seen55aa, SeenLife;
  INT Csn = 0;
  USHORT Byte, Data;

  DPRINT("Setting read data port: 0x%x\n", ReadDataPort);

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
    DPRINT("Found %d cards at read port 0x%x\n", Csn, ReadDataPort);
  }

  return Csn;
}

static
PUCHAR
Isolate(VOID)
{
  PUCHAR ReadPort;

  for (ReadPort = (PUCHAR)ISAPNP_READ_PORT_START;
       (ULONG_PTR)ReadPort <= ISAPNP_READ_PORT_MAX;
       ReadPort += ISAPNP_READ_PORT_STEP)
  {
    /* Avoid the NE2000 probe space */
    if ((ULONG_PTR)ReadPort >= 0x280 &&
        (ULONG_PTR)ReadPort <= 0x380)
        continue;

    if (TryIsolate(ReadPort) > 0)
        return ReadPort;
  }

  return 0;
}

VOID
DeviceActivation(PISAPNP_LOGICAL_DEVICE IsaDevice,
                 BOOLEAN Activate)
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
ProbeIsaPnpBus(PISAPNP_FDO_EXTENSION FdoExt)
{
  PISAPNP_LOGICAL_DEVICE LogDevice;
  ISAPNP_IDENTIFIER Identifier;
  ISAPNP_LOGDEVID LogDevId;
  USHORT Csn;
  USHORT LogDev;
  PDEVICE_OBJECT Pdo;
  NTSTATUS Status;

  ASSERT(FdoExt->ReadDataPort);

  for (Csn = 1; Csn <= 0xFF; Csn++)
  {
    for (LogDev = 0; LogDev <= 0xFF; LogDev++)
    {
      Status = IoCreateDevice(FdoExt->Common.Self->DriverObject,
                              sizeof(ISAPNP_LOGICAL_DEVICE),
                              NULL,
                              FILE_DEVICE_CONTROLLER,
                              FILE_DEVICE_SECURE_OPEN,
                              FALSE,
                              &Pdo);
      if (!NT_SUCCESS(Status))
          return Status;

      Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      LogDevice = Pdo->DeviceExtension;

      RtlZeroMemory(LogDevice, sizeof(ISAPNP_LOGICAL_DEVICE));

      LogDevice->Common.Self = Pdo;
      LogDevice->Common.IsFdo = FALSE;
      LogDevice->Common.State = dsStopped;

      LogDevice->CSN = Csn;
      LogDevice->LDN = LogDev;

      WaitForKey();
      SendKey();
      Wake(Csn);

      Peek(FdoExt->ReadDataPort, &Identifier, sizeof(Identifier));

      if (Identifier.VendorId & 0x80)
      {
          IoDeleteDevice(LogDevice->Common.Self);
          return STATUS_SUCCESS;
      }

      if (!FindLogDevId(FdoExt->ReadDataPort, LogDev, &LogDevId))
          break;

      WriteLogicalDeviceNumber(LogDev);

      LogDevice->VendorId = LogDevId.VendorId;
      LogDevice->ProdId = LogDevId.ProdId;
      LogDevice->IoAddr = ReadIoBase(FdoExt->ReadDataPort, 0);
      LogDevice->IrqNo = ReadIrqNo(FdoExt->ReadDataPort, 0);

      DPRINT1("Detected ISA PnP device - VID: 0x%x PID: 0x%x IoBase: 0x%x IRQ:0x%x\n",
               LogDevice->VendorId, LogDevice->ProdId, LogDevice->IoAddr, LogDevice->IrqNo);

      WaitForKey();

      Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

      InsertTailList(&FdoExt->DeviceListHead, &LogDevice->ListEntry);
      FdoExt->DeviceCount++;
    }
  }

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaHwDetectReadDataPort(
  IN PISAPNP_FDO_EXTENSION FdoExt)
{
  FdoExt->ReadDataPort = Isolate();
  if (!FdoExt->ReadDataPort)
  {
      DPRINT1("No read data port found\n");
      return STATUS_UNSUCCESSFUL;
  }

  DPRINT1("Detected read data port at 0x%x\n", FdoExt->ReadDataPort);

  return STATUS_SUCCESS;
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
