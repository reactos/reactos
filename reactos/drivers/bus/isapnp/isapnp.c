/* $Id: isapnp.c,v 1.1 2001/05/01 23:00:05 chorns Exp $
 *
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            isapnp.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTE:            Parts adapted from linux ISA PnP driver
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <isapnp.h>

#define NDEBUG
#include <debug.h>


#ifdef  ALLOC_PRAGMA

// Make the initialization routines discardable, so that they 
// don't waste space

#pragma  alloc_text(init, DriverEntry)

// Make the PASSIVE_LEVEL routines pageable, so that they don't
// waste nonpaged memory

#pragma  alloc_text(page, ACPIDispatchOpenClose)
#pragma  alloc_text(page, ACPIDispatchRead)
#pragma  alloc_text(page, ACPIDispatchWrite)

#endif  /*  ALLOC_PRAGMA  */


PUCHAR IsaPnPReadPort;


#define UCHAR2USHORT(v0, v1) \
  ((v1 << 8) | v0)

#define UCHAR2ULONG(v0, v1, v2, v3) \
  ((UCHAR2USHORT(v2, v3) << 16) | UCHAR2USHORT(v0, v1))


#ifndef NDEBUG

struct
{
  PCH Name;
} SmallTags[] = {
  {"Unknown Small Tag"},
  {"ISAPNP_SRIN_VERSION"},
  {"ISAPNP_SRIN_LDEVICE_ID"},
  {"ISAPNP_SRIN_CDEVICE_ID"},
  {"ISAPNP_SRIN_IRQ_FORMAT"},
  {"ISAPNP_SRIN_DMA_FORMAT"},
  {"ISAPNP_SRIN_START_DFUNCTION"},
  {"ISAPNP_SRIN_END_DFUNCTION"},
  {"ISAPNP_SRIN_IO_DESCRIPTOR"},
  {"ISAPNP_SRIN_FL_IO_DESCRIPOTOR"},
  {"Reserved Small Tag"},
  {"Reserved Small Tag"},
  {"Reserved Small Tag"},
  {"Reserved Small Tag"},
  {"ISAPNP_SRIN_VENDOR_DEFINED"},
  {"ISAPNP_SRIN_END_TAG"}
};

struct
{
  PCH Name;
} LargeTags[] = {
  {"Unknown Large Tag"},
  {"ISAPNP_LRIN_MEMORY_RANGE"},
  {"ISAPNP_LRIN_ID_STRING_ANSI"},
  {"ISAPNP_LRIN_ID_STRING_UNICODE"},
  {"ISAPNP_LRIN_VENDOR_DEFINED"},
  {"ISAPNP_LRIN_MEMORY_RANGE32"},
  {"ISAPNP_LRIN_FL_MEMORY_RANGE32"}
};

PCSZ TagName(ULONG Tag, BOOLEAN Small)
{
  if (Small && (Tag <= ISAPNP_SRIN_END_TAG)) {
    return SmallTags[Tag].Name;
  } else if (Tag <= ISAPNP_LRIN_FL_MEMORY_RANGE32){
    return LargeTags[Tag].Name;
  }

  return NULL;
}

#endif

static inline VOID WriteData(UCHAR Value)
{
  WRITE_PORT_UCHAR((PUCHAR)ISAPNP_WRITE_PORT, Value);
}

static inline VOID WriteAddress(UCHAR Value)
{
	WRITE_PORT_UCHAR((PUCHAR)ISAPNP_ADDRESS_PORT, Value);
	KeStallExecutionProcessor(20);
}

static inline UCHAR ReadData(VOID)
{
	return READ_PORT_UCHAR(IsaPnPReadPort);
}

UCHAR ReadUchar(UCHAR Index)
{
	WriteAddress(Index);
	return ReadData();
}

USHORT ReadUshort(UCHAR Index)
{
	USHORT Value;

	Value = ReadUchar(Index);
	Value = (Value << 8) + ReadUchar(Index + 1);
	return Value;
}

ULONG ReadUlong(UCHAR Index)
{
	ULONG Value;

	Value = ReadUchar(Index);
	Value = (Value << 8) + ReadUchar(Index + 1);
	Value = (Value << 8) + ReadUchar(Index + 2);
	Value = (Value << 8) + ReadUchar(Index + 3);
	return Value;
}

VOID WriteUchar(UCHAR Index, UCHAR Value)
{
	WriteAddress(Index);
	WriteData(Value);
}

VOID WriteUshort(UCHAR Index, USHORT Value)
{
	WriteUchar(Index, Value >> 8);
	WriteUchar(Index + 1, Value);
}

VOID WriteUlong(UCHAR Index, ULONG Value)
{
	WriteUchar(Index, Value >> 24);
	WriteUchar(Index + 1, Value >> 16);
	WriteUchar(Index + 2, Value >> 8);
	WriteUchar(Index + 3, Value);
}

static inline VOID SetReadDataPort(ULONG Port)
{
  IsaPnPReadPort = (PUCHAR)Port;
	WriteUchar(0x00, Port >> 2);
	KeStallExecutionProcessor(100);
}

static VOID SendKey(VOID)
{
  ULONG i;
  UCHAR msb;
	UCHAR code;

  /* FIXME: Is there something better? */
	KeStallExecutionProcessor(1000);
	WriteAddress(0x00);
	WriteAddress(0x00);

  code = 0x6a;
	WriteAddress(code);
	for (i = 1; i < 32; i++) {
		msb = ((code & 0x01) ^ ((code & 0x02) >> 1)) << 7;
		code = (code >> 1) | msb;
		WriteAddress(code);
	}
}

/* Place all PnP cards in wait-for-key state */
static VOID SendWait(VOID)
{
	WriteUchar(0x02, 0x02);
}

VOID SendWake(UCHAR csn)
{
	WriteUchar(ISAPNP_CARD_WAKECSN, csn);
}

VOID SelectLogicalDevice(UCHAR LogicalDevice)
{
	WriteUchar(ISAPNP_CARD_LOG_DEVICE_NUM, LogicalDevice);
}

VOID ActivateLogicalDevice(UCHAR LogicalDevice)
{
	SelectLogicalDevice(LogicalDevice);
	WriteUchar(ISAPNP_CONTROL_ACTIVATE, 0x1);
	KeStallExecutionProcessor(250);
}

VOID DeactivateLogicalDevice(UCHAR LogicalDevice)
{
	SelectLogicalDevice(LogicalDevice);
	WriteUchar(ISAPNP_CONTROL_ACTIVATE, 0x0);
	KeStallExecutionProcessor(500);
}

#define READ_DATA_PORT_STEP 32  /* Minimum is 4 */

static ULONG FindNextReadPort(VOID)
{
	ULONG Port;

  Port = (ULONG)IsaPnPReadPort;
	while (Port <= ISAPNP_MAX_READ_PORT) {
		/*
		 * We cannot use NE2000 probe spaces for
     * ISAPnP or we will lock up machines
		 */
		if ((Port < 0x280) || (Port > 0x380))
		{
			return Port;
		}
		Port += READ_DATA_PORT_STEP;
	}
	return 0;
}

static BOOLEAN IsolateReadDataPortSelect(VOID)
{
  ULONG Port;

	SendWait();
	SendKey();

	/* Control: reset CSN and conditionally everything else too */
	WriteUchar(0x02, 0x05);
	KeStallExecutionProcessor(2000);

	SendWait();
	SendKey();
	SendWake(0x00);

	Port = FindNextReadPort();
  if (Port == 0) {
		SendWait();
		return FALSE;
	}

	SetReadDataPort(Port);
	KeStallExecutionProcessor(1000);
  WriteAddress(0x01);
	KeStallExecutionProcessor(1000);
	return TRUE;
}

/*
 * Isolate (assign uniqued CSN) to all ISA PnP devices
 */
static ULONG IsolatePnPCards(VOID)
{
	UCHAR checksum = 0x6a;
	UCHAR chksum = 0x00;
	UCHAR bit = 0x00;
	ULONG data;
	ULONG csn = 0;
	ULONG i;
	ULONG iteration = 1;

  DPRINT("Called\n");

	IsaPnPReadPort = (PUCHAR)ISAPNP_MIN_READ_PORT;
  if (!IsolateReadDataPortSelect()) {
    DPRINT("Could not set read data port\n");
		return 0;
  }

	while (TRUE) {
		for (i = 1; i <= 64; i++) {
			data = ReadData() << 8;
			KeStallExecutionProcessor(250);
			data = data | ReadData();
			KeStallExecutionProcessor(250);
			if (data == 0x55aa)
				bit = 0x01;
			checksum = ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >> 1);
			bit = 0x00;
		}
		for (i = 65; i <= 72; i++) {
			data = ReadData() << 8;
			KeStallExecutionProcessor(250);
			data = data | ReadData();
			KeStallExecutionProcessor(250);
			if (data == 0x55aa)
				chksum |= (1 << (i - 65));
		}
		if ((checksum != 0x00) && (checksum == chksum)) {
			csn++;

			WriteUchar(0x06, csn);
			KeStallExecutionProcessor(250);
			iteration++;
			SendWake(0x00);
			SetReadDataPort((ULONG)IsaPnPReadPort);
			KeStallExecutionProcessor(1000);
			WriteAddress(0x01);
			KeStallExecutionProcessor(1000);
			goto next;
		}
		if (iteration == 1) {
			IsaPnPReadPort += READ_DATA_PORT_STEP;
      if (!IsolateReadDataPortSelect()) {
        DPRINT("Could not set read data port\n");
				return 0;
      }
		} else if (iteration > 1) {
			break;
		}
next:
		checksum = 0x6a;
		chksum = 0x00;
		bit = 0x00;
	}
	SendWait();
	return csn;
}


VOID Peek(PUCHAR Data, ULONG Count)
{
	ULONG i, j;
	UCHAR d = 0;

	for (i = 1; i <= Count; i++) {
		for (j = 0; j < 20; j++) {
			d = ReadUchar(0x05);
			if (d & 0x1)
				break;
			KeStallExecutionProcessor(100);
		}
		if (!(d & 0x1)) {
			if (Data != NULL)
				*Data++ = 0xff;
			continue;
		}
		d = ReadUchar(0x04); /* PRESDI */
		if (Data != NULL)
			*Data++ = d;
	}
}


/*
 * Skip specified number of bytes from stream
 */
static VOID Skip(ULONG Count)
{
	Peek(NULL, Count);
}


/*
 * Read one tag from stream
 */
static BOOLEAN ReadTag(PUCHAR Type,
  PUSHORT Size,
  PBOOLEAN Small)
{
	UCHAR tag, tmp[2];

	Peek(&tag, 1);
  if (tag == 0) {
    /* Invalid tag */
    DPRINT("Invalid tag with value 0\n");
#ifndef NDEBUG
    for (;;);
#endif
		return FALSE;
  }

	if (tag & ISAPNP_RESOURCE_ITEM_TYPE) {
    /* Large resource item */
		*Type = (tag & 0x7f);
		Peek(tmp, 2);
		*Size = UCHAR2USHORT(tmp[0], tmp[1]);
    *Small = FALSE;
#ifndef NDEBUG
    if (*Type > ISAPNP_LRIN_FL_MEMORY_RANGE32) {
      DPRINT("Invalid large tag with value 0x%X\n", *Type);
      for (;;);
    }
#endif
	} else {
    /* Small resource item */
		*Type = (tag >> 3) & 0x0f;
		*Size = tag & 0x07;
    *Small = TRUE;
#ifndef NDEBUG
    if (*Type > ISAPNP_SRIN_END_TAG) {
      DPRINT("Invalid small tag with value 0x%X\n", *Type);
      for (;;);
    }
#endif
  }

	DPRINT("Tag = 0x%X, Type = 0x%X, Size = %d (%s)\n",
    tag, *Type, *Size, TagName(*Type, *Small));

  /* Probably invalid data */
  if ((*Type == 0xff) && (*Size == 0xffff)) {
    DPRINT("Invalid data (Type 0x%X  Size 0x%X)\n", *Type, *Size);
    for (;;);
		return FALSE;
  }

	return TRUE;
}


/*
 * Parse ANSI name for ISA PnP logical device
 */
static NTSTATUS ParseAnsiName(PUNICODE_STRING Name, PUSHORT Size)
{
  ANSI_STRING AnsiString;
  UCHAR Buffer[256];
  USHORT size1;

  size1 = (*Size >= sizeof(Buffer)) ? (sizeof(Buffer) - 1) : *Size;

  Peek(Buffer, size1);
  Buffer[size1] = '\0';
  *Size -= size1;

  /* Clean whitespace from end of string */
  while ((size1 > 0) && (Buffer[--size1] == ' ')) 
    Buffer[size1] = '\0';

  DPRINT("ANSI name: %s\n", Buffer);

  RtlInitAnsiString(&AnsiString, (PCSZ)&Buffer);
  return RtlAnsiStringToUnicodeString(Name, &AnsiString, TRUE);
}


/*
 * Add a resource list to the
 * resource lists of a logical device
 */
static NTSTATUS AddResourceList(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Priority,
  PISAPNP_CONFIGURATION_LIST *NewList)
{
  PISAPNP_CONFIGURATION_LIST List;
  NTSTATUS Status;

  DPRINT("Adding resource list for logical device %d on card %d (Priority %d)\n",
        LogicalDevice->Number,
        LogicalDevice->Card->CardId,
        Priority);

  List = (PISAPNP_CONFIGURATION_LIST)
      ExAllocatePool(PagedPool, sizeof(ISAPNP_CONFIGURATION_LIST));
  if (!List)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(List, sizeof(ISAPNP_CONFIGURATION_LIST));

  List->Priority = Priority;

  InitializeListHead(&List->ListHead);

  InsertTailList(&LogicalDevice->Configuration, &List->ListEntry);

  *NewList = List;

  return STATUS_SUCCESS;
}


/*
 * Add a resource entry to the
 * resource list of a logical device
 */
static NTSTATUS AddResourceDescriptor(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Priority,
  ULONG Option,
  PISAPNP_DESCRIPTOR *Descriptor)
{
  PLIST_ENTRY CurrentEntry;
  PISAPNP_CONFIGURATION_LIST List;
  PISAPNP_DESCRIPTOR d;
  NTSTATUS Status;

  DPRINT("Adding resource descriptor for logical device %d on card %d (%d of %d)\n",
        LogicalDevice->Number,
        LogicalDevice->Card->CardId,
        LogicalDevice->CurrentDescriptorCount,
        LogicalDevice->DescriptorCount);

  d = (PISAPNP_DESCRIPTOR)
    ExAllocatePool(PagedPool, sizeof(ISAPNP_DESCRIPTOR));
  if (!d)
    return Status;

  RtlZeroMemory(d, sizeof(ISAPNP_DESCRIPTOR));

  d->Descriptor.Option = Option;

  *Descriptor = d;

  CurrentEntry = LogicalDevice->Configuration.Flink;
  while (CurrentEntry != &LogicalDevice->Configuration) {
    List = CONTAINING_RECORD(
      CurrentEntry, ISAPNP_CONFIGURATION_LIST, ListEntry);

    if (List->Priority == Priority) {

      LogicalDevice->ConfigurationSize += sizeof(IO_RESOURCE_DESCRIPTOR);
      InsertTailList(&List->ListHead, &d->ListEntry);
      LogicalDevice->CurrentDescriptorCount++;
      if (LogicalDevice->DescriptorCount <
        LogicalDevice->CurrentDescriptorCount) {
        LogicalDevice->DescriptorCount =
          LogicalDevice->CurrentDescriptorCount;
      }

      return STATUS_SUCCESS;
    }
    CurrentEntry = CurrentEntry->Flink;
  }

  Status = AddResourceList(LogicalDevice, Priority, &List);
  if (NT_SUCCESS(Status)) {
    LogicalDevice->ConfigurationSize += sizeof(IO_RESOURCE_LIST);
    LogicalDevice->CurrentDescriptorCount = 0;
    InsertTailList(&List->ListHead, &d->ListEntry);
  }

  return Status;
}


/*
 * Add IRQ resource to resources list
 */
static NTSTATUS AddIrqResource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[3];
  ULONG irq, i, last;
  BOOLEAN found;
  NTSTATUS Status;

	Peek(tmp, Size);

	irq = UCHAR2USHORT(tmp[0], tmp[0]);

  DPRINT("IRQ bitmask: 0x%X\n", irq);

  found = FALSE;
  for (i = 0; i < 16; i++) {
    if (!found && (irq & (1 << i))) {
      last = i;
      found = TRUE;
    }

    if ((found && !(irq & (1 << i))) || (irq & (1 << i) && (i == 15))) {
      Status = AddResourceDescriptor(LogicalDevice,
        Priority, Option, &Descriptor);
      if (!NT_SUCCESS(Status))
        return Status;
      Descriptor->Descriptor.Type = CmResourceTypeInterrupt;
      Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
      Descriptor->Descriptor.u.Interrupt.MinimumVector = last;

      if ((irq & (1 << i)) && (i == 15))
        Descriptor->Descriptor.u.Interrupt.MaximumVector = i;
      else
        Descriptor->Descriptor.u.Interrupt.MaximumVector = i - 1;

      DPRINT("Found IRQ range %d - %d for logical device %d on card %d\n",
        Descriptor->Descriptor.u.Interrupt.MinimumVector,
        Descriptor->Descriptor.u.Interrupt.MaximumVector,
        LogicalDevice->Number,
        LogicalDevice->Card->CardId);

      found = FALSE;
    }
  }

  return STATUS_SUCCESS;
}

/*
 * Add DMA resource to resources list
 */
static NTSTATUS AddDmaResource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[2];
  ULONG dma, flags, i, last;
  BOOLEAN found;
  NTSTATUS Status;

	Peek(tmp, Size);

	dma = tmp[0];
  flags = tmp[1];

  DPRINT("DMA bitmask: 0x%X\n", dma);

  found = FALSE;
  for (i = 0; i < 8; i++) {
    if (!found && (dma & (1 << i))) {
      last = i;
      found = TRUE;
    }

    if ((found && !(dma & (1 << i))) || (dma & (1 << i) && (i == 15))) {
      Status = AddResourceDescriptor(LogicalDevice,
        Priority, Option, &Descriptor);
      if (!NT_SUCCESS(Status))
        return Status;
      Descriptor->Descriptor.Type = CmResourceTypeDma;
      Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
      Descriptor->Descriptor.u.Dma.MinimumChannel = last;

      if ((dma & (1 << i)) && (i == 15))
        Descriptor->Descriptor.u.Dma.MaximumChannel = i;
      else
        Descriptor->Descriptor.u.Dma.MaximumChannel = i - 1;

      /* FIXME: Parse flags */

      DPRINT("Found DMA range %d - %d for logical device %d on card %d\n",
        Descriptor->Descriptor.u.Dma.MinimumChannel,
        Descriptor->Descriptor.u.Dma.MaximumChannel,
        LogicalDevice->Number,
        LogicalDevice->Card->CardId);

      found = FALSE;
    }
  }

  return STATUS_SUCCESS;
}

/*
 * Add port resource to resources list
 */
static NTSTATUS AddIOPortResource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
#if 0
  DPRINT("I/O port: size 0x%X\n", Size);
  Skip(Size);
#else
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[7];
  NTSTATUS Status;

	Peek(tmp, Size);

  Status = AddResourceDescriptor(LogicalDevice,
    Priority, Option, &Descriptor);
  if (!NT_SUCCESS(Status))
    return Status;
  Descriptor->Descriptor.Type = CmResourceTypePort;
  Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
  Descriptor->Descriptor.u.Port.Length = tmp[6];
  /* FIXME: Parse flags */
  Descriptor->Descriptor.u.Port.Alignment = 0;
  Descriptor->Descriptor.u.Port.MinimumAddress.QuadPart = UCHAR2USHORT(tmp[1], tmp[2]);
  Descriptor->Descriptor.u.Port.MaximumAddress.QuadPart = UCHAR2USHORT(tmp[4], tmp[4]);

  DPRINT("Found I/O port range 0x%X - 0x%X for logical device %d on card %d\n",
    Descriptor->Descriptor.u.Port.MinimumAddress,
    Descriptor->Descriptor.u.Port.MaximumAddress,
    LogicalDevice->Number,
    LogicalDevice->Card->CardId);
#endif
  return STATUS_SUCCESS;
}

/*
 * Add fixed port resource to resources list
 */
static NTSTATUS AddFixedIOPortResource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
#if 0
  DPRINT("Fixed I/O port: size 0x%X\n", Size);
  Skip(Size);
#else
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[3];
  NTSTATUS Status;

	Peek(tmp, Size);

  Status = AddResourceDescriptor(LogicalDevice,
    Priority, Option, &Descriptor);
  if (!NT_SUCCESS(Status))
    return Status;
  Descriptor->Descriptor.Type = CmResourceTypePort;
  Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
  Descriptor->Descriptor.u.Port.Length = tmp[2];
  Descriptor->Descriptor.u.Port.Alignment = 0;
  Descriptor->Descriptor.u.Port.MinimumAddress.QuadPart = UCHAR2USHORT(tmp[0], tmp[1]);
  Descriptor->Descriptor.u.Port.MaximumAddress.QuadPart = UCHAR2USHORT(tmp[0], tmp[1]);

  DPRINT("Found fixed I/O port range 0x%X - 0x%X for logical device %d on card %d\n",
    Descriptor->Descriptor.u.Port.MinimumAddress,
    Descriptor->Descriptor.u.Port.MaximumAddress,
    LogicalDevice->Number,
    LogicalDevice->Card->CardId);
#endif
  return STATUS_SUCCESS;
}

/*
 * Add memory resource to resources list
 */
static NTSTATUS AddMemoryResource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
#if 0
  DPRINT("Memory range: size 0x%X\n", Size);
  Skip(Size);
#else
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[9];
  NTSTATUS Status;

	Peek(tmp, Size);

  Status = AddResourceDescriptor(LogicalDevice,
    Priority, Option, &Descriptor);
  if (!NT_SUCCESS(Status))
    return Status;
  Descriptor->Descriptor.Type = CmResourceTypeMemory;
  Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
  Descriptor->Descriptor.u.Memory.Length = UCHAR2USHORT(tmp[7], tmp[8]) << 8;
  Descriptor->Descriptor.u.Memory.Alignment = UCHAR2USHORT(tmp[5], tmp[6]);
  Descriptor->Descriptor.u.Memory.MinimumAddress.QuadPart = UCHAR2USHORT(tmp[1], tmp[2]) << 8;
  Descriptor->Descriptor.u.Memory.MaximumAddress.QuadPart = UCHAR2USHORT(tmp[3], tmp[4]) << 8;

  DPRINT("Found memory range 0x%X - 0x%X for logical device %d on card %d\n",
    Descriptor->Descriptor.u.Memory.MinimumAddress,
    Descriptor->Descriptor.u.Memory.MaximumAddress,
    LogicalDevice->Number,
    LogicalDevice->Card->CardId);
#endif
  return STATUS_SUCCESS;
}

/*
 * Add 32-bit memory resource to resources list
 */
static NTSTATUS AddMemory32Resource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
#if 0
  DPRINT("Memory32 range: size 0x%X\n", Size);
  Skip(Size);
#else
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[17];
  NTSTATUS Status;

	Peek(tmp, Size);

  Status = AddResourceDescriptor(LogicalDevice,
    Priority, Option, &Descriptor);
  if (!NT_SUCCESS(Status))
    return Status;
  Descriptor->Descriptor.Type = CmResourceTypeMemory;
  Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
  Descriptor->Descriptor.u.Memory.Length =
    UCHAR2ULONG(tmp[13], tmp[14], tmp[15], tmp[16]);
  Descriptor->Descriptor.u.Memory.Alignment =
    UCHAR2ULONG(tmp[9], tmp[10], tmp[11], tmp[12]);
  Descriptor->Descriptor.u.Memory.MinimumAddress.QuadPart =
    UCHAR2ULONG(tmp[1], tmp[2], tmp[3], tmp[4]);
  Descriptor->Descriptor.u.Memory.MaximumAddress.QuadPart =
    UCHAR2ULONG(tmp[5], tmp[6], tmp[7], tmp[8]);

  DPRINT("Found memory32 range 0x%X - 0x%X for logical device %d on card %d\n",
    Descriptor->Descriptor.u.Memory.MinimumAddress,
    Descriptor->Descriptor.u.Memory.MaximumAddress,
    LogicalDevice->Number,
    LogicalDevice->Card->CardId);
#endif
  return STATUS_SUCCESS;
}

/*
 * Add 32-bit fixed memory resource to resources list
 */
static NTSTATUS AddFixedMemory32Resource(
  PISAPNP_LOGICAL_DEVICE LogicalDevice,
  ULONG Size,
  ULONG Priority,
  ULONG Option)
{
#if 0
  DPRINT("Memory32 range: size 0x%X\n", Size);
  Skip(Size);
#else
  PISAPNP_DESCRIPTOR Descriptor;
	UCHAR tmp[17];
  NTSTATUS Status;

	Peek(tmp, Size);

  Status = AddResourceDescriptor(LogicalDevice,
    Priority, Option, &Descriptor);
  if (!NT_SUCCESS(Status))
    return Status;
  Descriptor->Descriptor.Type = CmResourceTypeMemory;
  Descriptor->Descriptor.ShareDisposition = CmResourceShareDeviceExclusive;
  Descriptor->Descriptor.u.Memory.Length =
    UCHAR2ULONG(tmp[9], tmp[10], tmp[11], tmp[12]);
  Descriptor->Descriptor.u.Memory.Alignment =
    UCHAR2ULONG(tmp[5], tmp[6], tmp[7], tmp[8]);
  Descriptor->Descriptor.u.Memory.MinimumAddress.QuadPart =
    UCHAR2ULONG(tmp[1], tmp[2], tmp[3], tmp[4]);
  Descriptor->Descriptor.u.Memory.MaximumAddress.QuadPart =
    UCHAR2ULONG(tmp[1], tmp[2], tmp[3], tmp[4]);

  DPRINT("Found fixed memory32 range 0x%X - 0x%X for logical device %d on card %d\n",
    Descriptor->Descriptor.u.Memory.MinimumAddress,
    Descriptor->Descriptor.u.Memory.MaximumAddress,
    LogicalDevice->Number,
    LogicalDevice->Card->CardId);
#endif
  return STATUS_SUCCESS;
}


/*
 * Parse logical device tag
 */
static PISAPNP_LOGICAL_DEVICE ParseLogicalDevice(
  PISAPNP_DEVICE_EXTENSION DeviceExtension,
  PISAPNP_CARD Card,
  ULONG Size,
  USHORT Number)
{
	UCHAR tmp[6];
	PISAPNP_LOGICAL_DEVICE LogicalDevice;

  DPRINT("Card %d  Number %d\n", Card->CardId, Number);

  Peek(tmp, Size);

  LogicalDevice = (PISAPNP_LOGICAL_DEVICE)ExAllocatePool(
    PagedPool, sizeof(ISAPNP_LOGICAL_DEVICE));
	if (!LogicalDevice)
		return NULL;

  RtlZeroMemory(LogicalDevice, sizeof(ISAPNP_LOGICAL_DEVICE));

  LogicalDevice->Number = Number;
	LogicalDevice->VendorId = UCHAR2USHORT(tmp[0], tmp[1]);
	LogicalDevice->DeviceId = UCHAR2USHORT(tmp[2], tmp[3]);
	LogicalDevice->Regs = tmp[4];
	LogicalDevice->Card = Card;
	if (Size > 5)
		LogicalDevice->Regs |= tmp[5] << 8;

  InitializeListHead(&LogicalDevice->Configuration);

  ExInterlockedInsertTailList(&Card->LogicalDevices,
    &LogicalDevice->CardListEntry,
    &Card->LogicalDevicesLock);

  ExInterlockedInsertTailList(&DeviceExtension->DeviceListHead,
    &LogicalDevice->DeviceListEntry,
    &DeviceExtension->GlobalListLock);

  DeviceExtension->DeviceListCount++;

	return LogicalDevice;
}


/*
 * Parse resource map for logical device
 */
static BOOLEAN CreateLogicalDevice(PISAPNP_DEVICE_EXTENSION DeviceExtension,
  PISAPNP_CARD Card, USHORT Size)
{
	ULONG number = 0, skip = 0, compat = 0;
	UCHAR type, tmp[17];
  PISAPNP_LOGICAL_DEVICE LogicalDevice;
  BOOLEAN Small;
  ULONG Priority = 0;
  ULONG Option = IO_RESOURCE_REQUIRED;

  DPRINT("Card %d  Size %d\n", Card->CardId, Size);

  LogicalDevice = ParseLogicalDevice(DeviceExtension, Card, Size, number++);
	if (!LogicalDevice)
		return FALSE;

  while (TRUE) {
		if (!ReadTag(&type, &Size, &Small))
			return FALSE;

		if (skip && !(Small && (type == ISAPNP_SRIN_LDEVICE_ID)
      || (type == ISAPNP_SRIN_END_TAG)))
			goto skip;

    if (Small) {
  		switch (type) {
	  	case ISAPNP_SRIN_LDEVICE_ID:
        if ((Size >= 5) && (Size <= 6)) {
          LogicalDevice = ParseLogicalDevice(
            DeviceExtension, Card, Size, number++);
	        if (!LogicalDevice)
  	        return FALSE;
  				Size = 0;
	  			skip = 0;
		  	} else {
			  	skip = 1;
			  }
        Priority = 0;
        Option = IO_RESOURCE_REQUIRED;
			  compat = 0;
			  break;

		  case ISAPNP_SRIN_CDEVICE_ID:
			  if ((Size == 4) && (compat < MAX_COMPATIBLE_ID)) {
				  Peek(tmp, 4);
				  LogicalDevice->CVendorId[compat] = UCHAR2USHORT(tmp[0], tmp[1]); 
				  LogicalDevice->CDeviceId[compat] = UCHAR2USHORT(tmp[2], tmp[3]);
				  compat++;
				  Size = 0;
			  }
			  break;

		  case ISAPNP_SRIN_IRQ_FORMAT:
  			if ((Size < 2) || (Size > 3))
				  goto skip;
			  AddIrqResource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_SRIN_DMA_FORMAT:
  			if (Size != 2)
				  goto skip;
        AddDmaResource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_SRIN_START_DFUNCTION:
  			if (Size > 1)
				  goto skip;

        if (Size > 0) {
  				Peek(tmp, Size);
          Priority = tmp[0];
				  Size = 0;
          /* FIXME: Maybe use IO_RESOURCE_PREFERRED for some */
          Option = IO_RESOURCE_ALTERNATIVE;
        } else {
          Priority = 0;
          Option = IO_RESOURCE_ALTERNATIVE;
        }

        DPRINT("   Start priority %d   \n", Priority);

        LogicalDevice->CurrentDescriptorCount = 0;

			  break;

  		case ISAPNP_SRIN_END_DFUNCTION:

        DPRINT("   End priority %d   \n", Priority);

	  		if (Size != 0)
		  		goto skip;
        Priority = 0;
        Option = IO_RESOURCE_REQUIRED;
        LogicalDevice->CurrentDescriptorCount = 0;
			  break;

  		case ISAPNP_SRIN_IO_DESCRIPTOR:
	  		if (Size != 7)
		  		goto skip;
			  AddIOPortResource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_SRIN_FL_IO_DESCRIPOTOR:
			  if (Size != 3)
  				goto skip;
			  AddFixedIOPortResource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_SRIN_VENDOR_DEFINED:
  			break;

		  case ISAPNP_SRIN_END_TAG:
			  if (Size > 0)
  				Skip(Size);
        return FALSE;

		  default:
  			DPRINT("Ignoring small tag of type 0x%X for logical device %d on card %d\n",
          type, LogicalDevice->Number, Card->CardId);
      }
    } else {
      switch (type) {
		  case ISAPNP_LRIN_MEMORY_RANGE:
			  if (Size != 9)
  				goto skip;
			  AddMemoryResource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_LRIN_ID_STRING_ANSI:
        ParseAnsiName(&LogicalDevice->Name, &Size);
			  break;

		  case ISAPNP_LRIN_ID_STRING_UNICODE:
  			break;

	  	case ISAPNP_LRIN_VENDOR_DEFINED:
			  break;

		  case ISAPNP_LRIN_MEMORY_RANGE32:
  			if (Size != 17)
				  goto skip;
			  AddMemory32Resource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  case ISAPNP_LRIN_FL_MEMORY_RANGE32:
  			if (Size != 17)
				  goto skip;
			  AddFixedMemory32Resource(LogicalDevice, Size, Priority, Option);
			  Size = 0;
			  break;

		  default:
  			DPRINT("Ignoring large tag of type 0x%X for logical device %d on card %d\n",
          type, LogicalDevice->Number, Card->CardId);
		  }
    }
skip:
  if (Size > 0)
	  Skip(Size);
	}

	return TRUE;
}


/*
 * Parse resource map for ISA PnP card
 */
static BOOLEAN ParseResourceMap(PISAPNP_DEVICE_EXTENSION DeviceExtension,
  PISAPNP_CARD Card)
{
	UCHAR type, tmp[17];
	USHORT size;
  BOOLEAN Small;

  DPRINT("Card %d\n", Card->CardId);

	while (TRUE) {
		if (!ReadTag(&type, &size, &Small))
			return FALSE;

    if (Small) {
		  switch (type) {
		  case ISAPNP_SRIN_VERSION:
  			if (size != 2)
	  			goto skip;
		  	Peek(tmp, 2);
			  Card->PNPVersion = tmp[0];
			  Card->ProductVersion = tmp[1];
			  size = 0;
			  break;

      case ISAPNP_SRIN_LDEVICE_ID:
  			if ((size >= 5) && (size <= 6)) {
				  if (!CreateLogicalDevice(DeviceExtension, Card, size))
  					return FALSE;
				  size = 0;
			  }
			  break;
  
      case ISAPNP_SRIN_CDEVICE_ID:
        /* FIXME: Parse compatible IDs */
        break;

		  case ISAPNP_SRIN_END_TAG:
			  if (size > 0)
  				Skip(size);
			  return TRUE;

		  default:
  			DPRINT("Ignoring small tag Type 0x%X for Card %d\n", type, Card->CardId);
		  }
    } else {
		  switch (type) {
		  case ISAPNP_LRIN_ID_STRING_ANSI:
  			ParseAnsiName(&Card->Name, &size);
			  break;

      default:
  			DPRINT("Ignoring large tag Type 0x%X for Card %d\n",
          type, Card->CardId);
		  }
    }
skip:
  if (size > 0)
    Skip(size);
  }

  return TRUE;
}


/*
 * Compute ISA PnP checksum for first eight bytes
 */
static UCHAR Checksum(PUCHAR data)
{
	ULONG i, j;
	UCHAR checksum = 0x6a, bit, b;

	for (i = 0; i < 8; i++) {
		b = data[i];
		for (j = 0; j < 8; j++) {
			bit = 0;
			if (b & (1 << j))
				bit = 1;
			checksum = ((((checksum ^ (checksum >> 1)) &
        0x01) ^ bit) << 7) | (checksum >> 1);
		}
	}
	return checksum;
}


/*
 * Build a resource list for a logical ISA PnP device
 */
static NTSTATUS BuildResourceList(PISAPNP_LOGICAL_DEVICE LogicalDevice,
  PIO_RESOURCE_LIST DestinationList,
  ULONG Priority)
{
  PLIST_ENTRY CurrentEntry, Entry;
  PISAPNP_CONFIGURATION_LIST List;
  PISAPNP_DESCRIPTOR Descriptor;
  NTSTATUS Status;
  ULONG i;

  if (IsListEmpty(&LogicalDevice->Configuration))
    return STATUS_NOT_FOUND;

  CurrentEntry = LogicalDevice->Configuration.Flink;
  while (CurrentEntry != &LogicalDevice->Configuration) {
    List = CONTAINING_RECORD(
      CurrentEntry, ISAPNP_CONFIGURATION_LIST, ListEntry);

    if (List->Priority == Priority) {

      DPRINT("Logical device %d  DestinationList 0x%X\n",
        LogicalDevice->Number,
        DestinationList);

      DestinationList->Version = 1;
      DestinationList->Revision = 1;
      DestinationList->Count = LogicalDevice->DescriptorCount;

      i = 0;
      Entry = List->ListHead.Flink;
      while (Entry != &List->ListHead) {
        Descriptor = CONTAINING_RECORD(
          Entry, ISAPNP_DESCRIPTOR, ListEntry);

        DPRINT("Logical device %d  Destination 0x%X(%d)\n",
          LogicalDevice->Number,
          &DestinationList->Descriptors[i],
          i);

        RtlCopyMemory(&DestinationList->Descriptors[i],
          &Descriptor->Descriptor,
          sizeof(IO_RESOURCE_DESCRIPTOR));

        i++;

        Entry = Entry->Flink;
      }

      RemoveEntryList(&List->ListEntry);

      ExFreePool(List);

      return STATUS_SUCCESS;
    }

    CurrentEntry = CurrentEntry->Flink;
  }

  return STATUS_UNSUCCESSFUL;
}


/*
 * Build resource lists for a logical ISA PnP device
 */
static NTSTATUS BuildResourceLists(PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
  ULONG ListSize;
  ULONG Priority;
  ULONG SingleListSize;
  PIO_RESOURCE_LIST p;
  NTSTATUS Status;

  ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
    - sizeof(IO_RESOURCE_LIST)
    + LogicalDevice->ConfigurationSize;

  DPRINT("Logical device %d  ListSize 0x%X  ConfigurationSize 0x%X  DescriptorCount %d\n",
    LogicalDevice->Number, ListSize,
    LogicalDevice->ConfigurationSize,
    LogicalDevice->DescriptorCount);

  LogicalDevice->ResourceLists =
    (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(
      PagedPool, ListSize);
	if (!LogicalDevice->ResourceLists)
		return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(LogicalDevice->ResourceLists, ListSize);

  SingleListSize = sizeof(IO_RESOURCE_LIST) +
    (LogicalDevice->DescriptorCount - 1) *
    sizeof(IO_RESOURCE_DESCRIPTOR);

  DPRINT("SingleListSize %d\n", SingleListSize);

  Priority = 0;
  p = &LogicalDevice->ResourceLists->List[0];
  do {
    Status = BuildResourceList(LogicalDevice, p, Priority);
    if (NT_SUCCESS(Status)) {
      p = (PIO_RESOURCE_LIST)((ULONG)p + SingleListSize);
      Priority++;
    }
  } while (Status != STATUS_NOT_FOUND);

  LogicalDevice->ResourceLists->ListSize = ListSize;
  LogicalDevice->ResourceLists->AlternativeLists = Priority + 1;

  return STATUS_SUCCESS;
}


/*
 * Build resource lists for a ISA PnP card
 */
static NTSTATUS BuildResourceListsForCard(PISAPNP_CARD Card)
{
  PISAPNP_LOGICAL_DEVICE LogicalDevice;
  PLIST_ENTRY CurrentEntry;
  NTSTATUS Status;

  CurrentEntry = Card->LogicalDevices.Flink;
  while (CurrentEntry != &Card->LogicalDevices) {
    LogicalDevice = CONTAINING_RECORD(
      CurrentEntry, ISAPNP_LOGICAL_DEVICE, CardListEntry);
    Status = BuildResourceLists(LogicalDevice);
    if (!NT_SUCCESS(Status))
      return Status;
    CurrentEntry = CurrentEntry->Flink;
  }

  return STATUS_SUCCESS;
}


/*
 * Build resource lists for all present ISA PnP cards
 */
static NTSTATUS BuildResourceListsForAll(
  PISAPNP_DEVICE_EXTENSION DeviceExtension)
{
  PLIST_ENTRY CurrentEntry;
  PISAPNP_CARD Card;
  NTSTATUS Status;

  CurrentEntry = DeviceExtension->CardListHead.Flink;
  while (CurrentEntry != &DeviceExtension->CardListHead) {
    Card = CONTAINING_RECORD(
      CurrentEntry, ISAPNP_CARD, ListEntry);
    Status = BuildResourceListsForCard(Card);
    if (!NT_SUCCESS(Status))
      return Status;
    CurrentEntry = CurrentEntry->Flink;
  }

  return STATUS_SUCCESS;
}


/*
 * Build device list for all present ISA PnP cards
 */
static NTSTATUS BuildDeviceList(PISAPNP_DEVICE_EXTENSION DeviceExtension)
{
	ULONG csn;
	UCHAR header[9], checksum;
  PISAPNP_CARD Card;
  NTSTATUS Status;

  DPRINT("Called\n");

	SendWait();
	SendKey();
	for (csn = 1; csn <= 10; csn++) {
		SendWake(csn);
		Peek(header, 9);
		checksum = Checksum(header);

    if (checksum == 0x00 || checksum != header[8])  /* Invalid CSN */
			continue;

		DPRINT("VENDOR: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			header[0], header[1], header[2], header[3],
			header[4], header[5], header[6], header[7], header[8]);

    Card = (PISAPNP_CARD)ExAllocatePool(
      PagedPool, sizeof(ISAPNP_CARD));
    if (!Card)
      return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Card, sizeof(ISAPNP_CARD));

		Card->CardId = csn;
		Card->VendorId = (header[1] << 8) | header[0];
		Card->DeviceId = (header[3] << 8) | header[2];
		Card->Serial = (header[7] << 24) | (header[6] << 16) | (header[5] << 8) | header[4];

    InitializeListHead(&Card->LogicalDevices);
    KeInitializeSpinLock(&Card->LogicalDevicesLock);

    ParseResourceMap(DeviceExtension, Card);

    ExInterlockedInsertTailList(&DeviceExtension->CardListHead,
      &Card->ListEntry,
      &DeviceExtension->GlobalListLock);
	}

  return STATUS_SUCCESS;
}


NTSTATUS
ISAPNPQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PISAPNP_DEVICE_EXTENSION DeviceExtension;
  PISAPNP_LOGICAL_DEVICE LogicalDevice;
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  NTSTATUS Status;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PISAPNP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (Irp->IoStatus.Information) {
    /* FIXME: Another bus driver has already created a DEVICE_RELATIONS 
              structure so we must merge this structure with our own */
  }

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (DeviceExtension->DeviceListCount - 1);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = DeviceExtension->DeviceListCount;

  i = 0;
  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead) {
    LogicalDevice = CONTAINING_RECORD(
      CurrentEntry, ISAPNP_LOGICAL_DEVICE, DeviceListEntry);

    if (!LogicalDevice->Pdo) {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(DeviceObject->DriverObject, 0,
        NULL, FILE_DEVICE_CONTROLLER, 0, FALSE, &LogicalDevice->Pdo);
      if (!NT_SUCCESS(Status)) {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        ExFreePool(Relations);
        return Status;
      }

      LogicalDevice->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(LogicalDevice->Pdo);

    Relations->Objects[i] = LogicalDevice->Pdo;

    i++;

    CurrentEntry = CurrentEntry->Flink;
  }

  Irp->IoStatus.Information = (ULONG)Relations;

  return Status;
}


NTSTATUS
ISAPNPQueryDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PISAPNP_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PISAPNP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->State == dsStopped)
    return STATUS_UNSUCCESSFUL;

  switch (IrpSp->Parameters.QueryDeviceRelations.Type) {
    case BusRelations:
      Status = ISAPNPQueryBusRelations(DeviceObject, Irp, IrpSp);
      break;

    default:
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


NTSTATUS
ISAPNPStartDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PISAPNP_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;
  ULONG NumCards;

  DPRINT("Called\n");

  DeviceExtension = (PISAPNP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->State == dsStarted)
    return STATUS_SUCCESS;

  NumCards = IsolatePnPCards();

  DPRINT("Number of ISA PnP cards found: %d\n", NumCards);

  Status = BuildDeviceList(DeviceExtension);
  if (!NT_SUCCESS(Status)) {
    DPRINT("BuildDeviceList() failed with status 0x%X\n", Status);
    return Status;
  }

  Status = BuildResourceListsForAll(DeviceExtension);
  if (!NT_SUCCESS(Status)) {
    DPRINT("BuildResourceListsForAll() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension->State = dsStarted;

  return STATUS_SUCCESS;
}


NTSTATUS
ISAPNPStopDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PISAPNP_DEVICE_EXTENSION DeviceExtension;

  DPRINT("Called\n");

  DeviceExtension = (PISAPNP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->State != dsStopped) {
    /* FIXME: Stop device */
    DeviceExtension->State = dsStopped;
  }

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
ISAPNPDispatchOpenClose(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  DPRINT("Called\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
ISAPNPDispatchReadWrite(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PIRP Irp)
{
  DPRINT("Called\n");

  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
STDCALL
ISAPNPDispatchDeviceControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  Irp->IoStatus.Information = 0;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
STDCALL
ISAPNPControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = ISAPNPQueryDeviceRelations(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_START_DEVICE:
    Status = ISAPNPStartDevice(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_STOP_DEVICE:
    Status = ISAPNPStopDevice(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
ISAPNPAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PISAPNP_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, sizeof(ISAPNP_DEVICE_EXTENSION),
    NULL, FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SECURE_OPEN, TRUE, &Fdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension = (PISAPNP_DEVICE_EXTENSION)Fdo->DeviceExtension;

  DeviceExtension->Pdo = PhysicalDeviceObject;

  DeviceExtension->Ldo =
    IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

  InitializeListHead(&DeviceExtension->CardListHead);
  InitializeListHead(&DeviceExtension->DeviceListHead);
  DeviceExtension->DeviceListCount = 0;
  KeInitializeSpinLock(&DeviceExtension->GlobalListLock);

  DeviceExtension->State = dsStopped;

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Done AddDevice\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
  IN PDRIVER_OBJECT DriverObject, 
  IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("ISA Plug and Play Bus Driver\n");

  DriverObject->MajorFunction[IRP_MJ_CREATE] = ISAPNPDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = ISAPNPDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = ISAPNPDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = ISAPNPDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ISAPNPDispatchDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = ISAPNPControl;
  DriverObject->DriverExtension->AddDevice = ISAPNPAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
