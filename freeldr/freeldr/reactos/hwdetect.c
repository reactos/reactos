/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <rtl.h>
#include <mm.h>
#include <portio.h>
#include "registry.h"
#include "hwdetect.h"
#include <debug.h>

/* ***** BEGIN I/O ***** */

#define MILLISEC     (10)
#define PRECISION    (8)

static unsigned int delay_count = 1;

static VOID
__KeStallExecutionProcessor(U32 Loops)
{
  register unsigned int i;
  for (i = 0; i < Loops; i++);
}

VOID KeStallExecutionProcessor(U32 Microseconds)
{
  __KeStallExecutionProcessor((delay_count * Microseconds) / 1000);
}

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)

static U32
Read8254Timer(VOID)
{
	U32 Count;

	WRITE_PORT_UCHAR((PU8)0x43, 0x00);
	Count = READ_PORT_UCHAR((PU8)0x40);
	Count |= READ_PORT_UCHAR((PU8)0x40) << 8;
	return Count;
}

static VOID
WaitFor8254Wraparound(VOID)
{
	U32 CurCount;
  U32 PrevCount = ~0;
	S32 Delta;

	CurCount = Read8254Timer();

	do {
    PrevCount = CurCount;
    CurCount = Read8254Timer();
    Delta = CurCount - PrevCount;

    /*
     * This limit for delta seems arbitrary, but it isn't, it's
     * slightly above the level of error a buggy Mercury/Neptune
     * chipset timer can cause.
     */
	} while (Delta < 300);
}

VOID
HalpCalibrateStallExecution(VOID)
{
  U32 i;
  U32 calib_bit;
  U32 CurCount;

  /* Initialise timer interrupt with MILLISECOND ms interval        */
  WRITE_PORT_UCHAR((PU8)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
  WRITE_PORT_UCHAR((PU8)0x40, LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PU8)0x40, LATCH >> 8); /* MSB */
  
  /* Stage 1:  Coarse calibration                                   */
  
  WaitFor8254Wraparound();
  
  delay_count = 1;
  
  do {
    delay_count <<= 1;                  /* Next delay count to try */

    WaitFor8254Wraparound();
  
    __KeStallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
  } while (CurCount > LATCH / 2);
  
  delay_count >>= 1;              /* Get bottom value for delay     */
  
  /* Stage 2:  Fine calibration                                     */
  
  calib_bit = delay_count;        /* Which bit are we going to test */
  
  for(i=0;i<PRECISION;i++) {
    calib_bit >>= 1;             /* Next bit to calibrate          */
    if(!calib_bit) break;        /* If we have done all bits, stop */
  
    delay_count |= calib_bit;        /* Set the bit in delay_count */
  
    WaitFor8254Wraparound();
  
    __KeStallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
    if (CurCount <= LATCH / 2)   /* If a tick has passed, turn the */
      delay_count &= ~calib_bit; /* calibrated bit back off        */
  }
  
  /* We're finished:  Do the finishing touches                      */
  
  delay_count /= (MILLISEC / 2);   /* Calculate delay_count for 1ms */
}



/* ***** BEGIN Bus ***** */

/* access type 1 macros */
#define CONFIG_CMD(bus, dev_fn, where) \
	(0x80000000 | (((U32)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))

/* access type 2 macros */
#define IOADDR(dev_fn, where) \
	(0xC000 | (((dev_fn) & 0x1F) << 8) | (where))
#define FUNC(dev_fn) \
	((((dev_fn) & 0xE0) >> 4) | 0xf0)

static U32 PciBusConfigType = 0;  /* undetermined config type */

static BOOLEAN
ReadPciConfigUchar(U8 Bus,
  U8 Slot,
  U8 Offset,
  PU8 Value)
{
  switch (PciBusConfigType)
    {
      case 1:
        WRITE_PORT_ULONG((PU32)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
        *Value = READ_PORT_UCHAR((PU8)0xCFC + (Offset & 3));
        return TRUE;
      
      case 2:
        WRITE_PORT_UCHAR((PU8)0xCF8, FUNC(Slot));
        WRITE_PORT_UCHAR((PU8)0xCFA, Bus);
        *Value = READ_PORT_UCHAR((PU8)(IOADDR(Slot, Offset)));
        WRITE_PORT_UCHAR((PU8)0xCF8, 0);
        return TRUE;
    }
  return FALSE;
}

static BOOLEAN
ReadPciConfigUshort(U8 Bus,
  U8 Slot,
  U8 Offset,
  PU16 Value)
{
  if ((Offset & 1) != 0)
    {
      return FALSE;
    }
  
  switch (PciBusConfigType)
    {
      case 1:
        WRITE_PORT_ULONG((PU32)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
        *Value = READ_PORT_USHORT((PU16)0xCFC + (Offset & 2));
        return TRUE;

      case 2:
        WRITE_PORT_UCHAR((PU8)0xCF8, FUNC(Slot));
        WRITE_PORT_UCHAR((PU8)0xCFA, Bus);
        *Value = READ_PORT_USHORT((PU16)(IOADDR(Slot, Offset)));
        WRITE_PORT_UCHAR((PU8)0xCF8, 0);
        return TRUE;
    }
  return FALSE;
}

static BOOLEAN
ReadPciConfigUlong(U8 Bus,
  U8 Slot,
  U8 Offset,
  PU32 Value)
{
  if ((Offset & 3) != 0)
    {
      return FALSE;
    }
  
  switch (PciBusConfigType)
    {
      case 1:
        WRITE_PORT_ULONG((PU32)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
        *Value = READ_PORT_ULONG((PU32)0xCFC);
        return TRUE;
      
      case 2:
        WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
        WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
        *Value = READ_PORT_ULONG((PU32)(IOADDR(Slot, Offset)));
        WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
        return TRUE;
    }
  return FALSE;
}

static U32
GetPciBusConfigType(VOID)
{
  U32 Value;

  WRITE_PORT_UCHAR((PU8)0xCFB, 0x01);
  Value = READ_PORT_ULONG((PU32)0xCF8);
  WRITE_PORT_ULONG((PU32)0xCF8, 0x80000000);
  if (READ_PORT_ULONG((PU32)0xCF8) == 0x80000000)
    {
      WRITE_PORT_ULONG((PU32)0xCF8, Value);
      DbgPrint((DPRINT_HWDETECT, "Pci bus type 1 found\n"));
      return 1;
    }
  WRITE_PORT_ULONG((PU32)0xCF8, Value);
  
  WRITE_PORT_UCHAR((PU8)0xCFB, 0x00);
  WRITE_PORT_UCHAR((PU8)0xCF8, 0x00);
  WRITE_PORT_UCHAR((PU8)0xCFA, 0x00);
  if (READ_PORT_UCHAR((PU8)0xCF8) == 0x00 &&
    READ_PORT_UCHAR((PU8)0xCFB) == 0x00)
    {
      DbgPrint((DPRINT_HWDETECT, "Pci bus type 2 found\n"));
      return 2;
    }

  DbgPrint((DPRINT_HWDETECT, "No pci bus found\n"));
  return 0;
}

static U32
HalpGetPciData(U32 BusNumber,
  U32 SlotNumber,
  PVOID Buffer,
  U32 Offset,
  U32 Length)
{
  PVOID Ptr = Buffer;
  U32 Address = Offset;
  U32 Len = Length;
  U32 Vendor;
  UCHAR HeaderType;
  
  if ((Length == 0) || (PciBusConfigType == 0))
    return 0;
  
  ReadPciConfigUlong(BusNumber,
    SlotNumber & 0x1F,
    0x00,
    &Vendor);
  /* some broken boards return 0 if a slot is empty: */
  if (Vendor == 0xFFFFFFFF || Vendor == 0)
    {
      if (BusNumber == 0 && Offset == 0 && Length >= 2)
        {
          *(PU16)Buffer = PCI_INVALID_VENDORID;
          return 2;
        }
      return 0;
    }
  
  /* 0E=PCI_HEADER_TYPE */
  ReadPciConfigUchar(BusNumber,
    SlotNumber & 0x1F,
    0x0E,
    &HeaderType);
  if (((HeaderType & PCI_MULTIFUNCTION) == 0) && ((SlotNumber & 0xE0) != 0))
    {
      if (Offset == 0 && Length >= 2)
        {
          *(PU16)Buffer = PCI_INVALID_VENDORID;
          return 2;
        }
      return 0;
    }
  ReadPciConfigUlong(BusNumber,
    SlotNumber,
    0x00,
    &Vendor);

  /* some broken boards return 0 if a slot is empty: */
  if (Vendor == 0xFFFFFFFF || Vendor == 0)
    {
      if (BusNumber == 0 && Offset == 0 && Length >= 2)
        {
          *(PU16)Buffer = PCI_INVALID_VENDORID;
          return 2;
        }
        return 0;
    }
  
  if ((Address & 1) && (Len >= 1))
    {
      ReadPciConfigUchar(BusNumber,
        SlotNumber,
        Address,
        Ptr);
      Ptr = Ptr + 1;
      Address++;
      Len--;
    }
  
  if ((Address & 2) && (Len >= 2))
    {
      ReadPciConfigUshort(BusNumber,
        SlotNumber,
        Address,
        Ptr);
      Ptr = Ptr + 2;
      Address += 2;
      Len -= 2;
    }
  
  while (Len >= 4)
    {
      ReadPciConfigUlong(BusNumber,
        SlotNumber,
        Address,
        Ptr);
        Ptr = Ptr + 4;
        Address += 4;
        Len -= 4;
    }
  
  if (Len >= 2)
    {
      ReadPciConfigUshort(BusNumber,
      SlotNumber,
      Address,
      Ptr);
      Ptr = Ptr + 2;
      Address += 2;
      Len -= 2;
    }
  
  if (Len >= 1)
    {
      ReadPciConfigUchar(BusNumber,
        SlotNumber,
        Address,
        Ptr);
      Ptr = Ptr + 1;
      Address++;
      Len--;
    }
  
  return Length - Len;
}

U32
HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType,
  U32 BusNumber,
  U32 SlotNumber,
  PVOID Buffer,
  U32 Offset,
  U32 Length)
{
  U32 Result;

  if (BusDataType == PCIConfiguration)
    {
      Result = HalpGetPciData(BusNumber,
        SlotNumber,
        Buffer,
        Offset,
        Length);
    }
  else
    {
       DbgPrint((DPRINT_WARNING, "Unknown bus data type %d", (U32) BusDataType));
       Result = 0;
    }

  return Result;
}

U32
HalGetBusData(BUS_DATA_TYPE BusDataType,
  U32 BusNumber,
  U32 SlotNumber,
  PVOID Buffer,
  U32 Length)
{
  return (HalGetBusDataByOffset(BusDataType,
    BusNumber,
    SlotNumber,
    Buffer,
    0,
    Length));
}

/* ***** END Bus ***** */



/* ***** BEGIN Helper functions ***** */

static LIST_ENTRY BusKeyListHead; /* REGISTRY_BUS_INFORMATION */

static VOID
RegisterBusKey(HKEY Key, INTERFACE_TYPE BusType, U32 BusNumber)
{
  PREGISTRY_BUS_INFORMATION BusKey;

  BusKey = MmAllocateMemory(sizeof(REGISTRY_BUS_INFORMATION));
  if (BusKey == NULL)
    return;

  BusKey->BusType = BusType;
  BusKey->BusNumber = BusNumber;
  InsertTailList(&BusKeyListHead, &BusKey->ListEntry);
}


static HKEY
GetBusKey(INTERFACE_TYPE BusType, U32 BusNumber)
{
  PREGISTRY_BUS_INFORMATION BusKey;
  PLIST_ENTRY ListEntry;

  ListEntry = BusKeyListHead.Flink;
  while (ListEntry != &BusKeyListHead)
  	{
  	  BusKey = CONTAINING_RECORD(ListEntry,
        REGISTRY_BUS_INFORMATION,
        ListEntry);

      if ((BusKey->BusType == BusType) && (BusKey->BusNumber == BusNumber))
        {
          return BusKey->BusKey;
        }

  	  ListEntry = ListEntry->Flink;
  	}

  DbgPrint((DPRINT_WARNING, "Key not found for BysType %d and BusNumber %d\n",
    BusType, BusNumber));
  return NULL;
}


static HKEY
CreateOrOpenKey(HKEY RelativeKey, PCHAR KeyName)
{
  HKEY Key;
  S32 Error;

  Error = RegOpenKey(RelativeKey,
    KeyName,
    &Key);
  if (Error == ERROR_SUCCESS)
    {
      return Key;
    }

  Error = RegCreateKey(RelativeKey,
    KeyName,
    &Key);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return NULL;
    }

  return Key;
}

/* ***** END Helper functions ***** */



/* ***** BEGIN ATA ***** */

typedef struct _PCI_NATIVE_CONTROLLER 
{
  U16 VendorID;
  U16 DeviceID;
} PCI_NATIVE_CONTROLLER, *PPCI_NATIVE_CONTROLLER;

PCI_NATIVE_CONTROLLER const PciNativeController[] = 
{
  {
  	0x105A,		    // Promise 
  	0x4D68,		    // PDC20268, Ultra100TX2
  },
  {
  	0x105A,		    // Promise 
  	0x4D30,		    // PDC20267, Ultra100
  }
};

static PCI_SLOT_NUMBER LastSlotNumber;

static BOOLEAN AtdiskPrimaryClaimed = FALSE;
static BOOLEAN AtdiskSecondaryClaimed = FALSE;

inline void
IDESwapBytePairs(char *Buf,
  int Cnt)
{
  char t;
  int i;

  for (i = 0; i < Cnt; i += 2)
    {
      t = Buf[i];
      Buf[i] = Buf[i+1];
      Buf[i+1] = t;
    }
}

/*
 * AtapiPolledRead
 *
 * DESCRIPTION:
 *   Read a sector of data from the drive in a polled fashion.
 * ARGUMENTS:
 *   CommandPort
 *     Address of command port for drive
 *   ControlPort
 *     Address of control port for drive
 *   PreComp
 *     Value to write to precomp register
 *   SectorCnt
 *     Value to write to sectorCnt register
 *   SectorNum
 *     Value to write to sectorNum register
 *   CylinderLow
 *     Value to write to CylinderLow register
 *   CylinderHigh
 *     Value to write to CylinderHigh register
 *   DrvHead
 *     Value to write to Drive/Head register
 *   Command
 *     Value to write to Command register
 *   Buffer
 *     Buffer for output data
 * RETURNS:
 *   0 is success, non 0 is an error code
 */
static S32
AtapiPolledRead(U32 CommandPort,
  U32 ControlPort,
  U8 PreComp,
  U8 SectorCnt,
  U8 SectorNum,
  U8 CylinderLow,
  U8 CylinderHigh,
  U8 DrvHead,
  U8 Command,
  U8 *Buffer)
{
  U32 SectorCount = 0;
  U32 RetryCount;
  BOOLEAN Junk = FALSE;
  U8 Status;

  /* Wait for BUSY to clear */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      KeStallExecutionProcessor(10);
    }
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      DbgPrint((DPRINT_HWDETECT, "Drive is BUSY for too long\n"));
      return(IDE_ER_ABRT);
    }

  /* Write Drive/Head to select drive */
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);
  KeStallExecutionProcessor(500);

  /* Disable interrupts */
  IDEWriteDriveControl(ControlPort, IDE_DC_nIEN);
  KeStallExecutionProcessor(500);

  /* Issue command to drive */
  if (DrvHead & IDE_DH_LBA)
    {
      DbgPrint((DPRINT_HWDETECT, "READ:DRV=%d:LBA=1:BLK=%d:SC=%x:CM=%x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     ((DrvHead & 0x0f) << 24) + (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
	     SectorCnt,
	     Command));
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "READ:DRV=%d:LBA=0:CH=%x:CL=%x:HD=%x:SN=%x:SC=%x:CM=%x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     CylinderHigh,
	     CylinderLow,
	     DrvHead & 0x0f,
	     SectorNum,
	     SectorCnt,
	     Command));
    }

  /* Setup command parameters */
  IDEWritePrecomp(CommandPort, PreComp);
  IDEWriteSectorCount(CommandPort, SectorCnt);
  IDEWriteSectorNum(CommandPort, SectorNum);
  IDEWriteCylinderHigh(CommandPort, CylinderHigh);
  IDEWriteCylinderLow(CommandPort, CylinderLow);
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);

  /* Issue the command */
  IDEWriteCommand(CommandPort, Command);
  KeStallExecutionProcessor(50);

  /* wait for DRQ or error */
  for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
	{
	  if (Status & IDE_SR_ERR)
	    {
	      IDEWriteDriveControl(ControlPort, 0);
	      KeStallExecutionProcessor(50);
	      IDEReadStatus(CommandPort);

	      return(IDE_ER_ABRT);
	    }
	  if (Status & IDE_SR_DRQ)
	    {
	      break;
	    }
	  else
	    {
	      IDEWriteDriveControl(ControlPort, 0);
	      KeStallExecutionProcessor(50);
	      IDEReadStatus(CommandPort);

	      return(IDE_ER_ABRT);
	    }
	}
      KeStallExecutionProcessor(10);
    }

  /*  timed out  */
  if (RetryCount >= IDE_MAX_POLL_RETRIES)
    {
      IDEWriteDriveControl(ControlPort, 0);
      KeStallExecutionProcessor(50);
      IDEReadStatus(CommandPort);

      return(IDE_ER_ABRT);
    }

  while (1)
    {
      /* Read data into buffer */
      if (Junk == FALSE)
	{
	  IDEReadBlock(CommandPort, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer += IDE_SECTOR_BUF_SZ;
	}
      else
	{
	  U8 JunkBuffer[IDE_SECTOR_BUF_SZ];
	  IDEReadBlock(CommandPort, JunkBuffer, IDE_SECTOR_BUF_SZ);
	}
      SectorCount++;

      /* Check for error or more sectors to read */
      for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
	{
	  Status = IDEReadStatus(CommandPort);
	  if (!(Status & IDE_SR_BUSY))
	    {
	      if (Status & IDE_SR_ERR)
		{
		  IDEWriteDriveControl(ControlPort, 0);
		  KeStallExecutionProcessor(50);
		  IDEReadStatus(CommandPort);

		  return(IDE_ER_ABRT);
		}
	      if (Status & IDE_SR_DRQ)
		{
		  if (SectorCount >= SectorCnt)
		    {
		      DbgPrint((DPRINT_HWDETECT, "Buffer size exceeded\n"));
		      Junk = TRUE;
		    }
		  break;
		}
	      else
		{
		  if (SectorCount > SectorCnt)
		    {
		      DbgPrint((DPRINT_HWDETECT, "Read %d sectors of junk\n",
			     SectorCount - SectorCnt));
		    }
		  IDEWriteDriveControl(ControlPort, 0);
		  KeStallExecutionProcessor(50);
		  IDEReadStatus(CommandPort);

		  return(0);
		}
	    }
	}
    }
}

/*
 * AtapiIdentifyDevice
 *
 * DESCRIPTION:
 *   Get the identification block from the drive
 *
 * ARGUMENTS:
 *   CommandPort
 *     Address of the command port
 *	 ControlPort
 *     Address of the control port
 *   DriveNum
 *     The drive index (0,1)
 *   Atapi
 *     Send an ATA(FALSE) or an ATAPI(TRUE) identify comand
 *   DrvParms
 *     Address to write drive identication block
 *
 * RETURNS:
 *	  TRUE: The drive identification block was retrieved successfully
 *	  FALSE: an error ocurred
 */
static BOOLEAN
AtapiIdentifyDevice(U32 CommandPort,
  U32 ControlPort,
  U32 DriveNum,
  BOOLEAN Atapi,
  PIDE_DRIVE_IDENTIFY DrvParms)
{
  S32 i;

  /* Get the Drive Identify block from drive or die */
  if (AtapiPolledRead(CommandPort,
    ControlPort,
    0,
    1,
    0,
    0,
    0,
    (DriveNum ? IDE_DH_DRV1 : 0),
    (Atapi ? IDE_CMD_IDENT_ATAPI_DRV : IDE_CMD_IDENT_ATA_DRV),
    (PU8)DrvParms) != 0)
    {
      DbgPrint((DPRINT_HWDETECT, "IDEPolledRead() failed\n"));
      return(FALSE);
    }

  /* Report on drive parameters if debug mode */
  IDESwapBytePairs(DrvParms->SerialNumber, 20);
  IDESwapBytePairs(DrvParms->FirmwareRev, 8);
  IDESwapBytePairs(DrvParms->ModelNumber, 40);
  DbgPrint((DPRINT_HWDETECT, "Config:%x  Cyls:%d  Heads:%d  Sectors/Track:%d  Gaps:%d %d\n",
    DrvParms->ConfigBits,
    DrvParms->LogicalCyls,
    DrvParms->LogicalHeads,
    DrvParms->SectorsPerTrack,
    DrvParms->InterSectorGap,
    DrvParms->InterSectorGapSize));
  DbgPrint((DPRINT_HWDETECT, "Bytes/PLO:%d  Vendor Cnt:%d  Serial number:[%s]\n",
    DrvParms->BytesInPLO,
    DrvParms->VendorUniqueCnt,
    DrvParms->SerialNumber));
  DbgPrint((DPRINT_HWDETECT, "Cntlr type:%d  BufSiz:%d  ECC bytes:%d  Firmware Rev:[%s]\n",
    DrvParms->ControllerType,
    DrvParms->BufferSize * IDE_SECTOR_BUF_SZ,
    DrvParms->ECCByteCnt,
    DrvParms->FirmwareRev));
  DbgPrint((DPRINT_HWDETECT, "Model:[%s]\n", DrvParms->ModelNumber));
  DbgPrint((DPRINT_HWDETECT, "RWMultMax?:%x  RWMult?:%x  LBA:%d  DMA:%d  MinPIO:%d ns  MinDMA:%d ns\n",
    (DrvParms->RWMultImplemented),
    (DrvParms->RWMultCurrent) & 0xff,
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0,
    (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0,
    DrvParms->MinPIOTransTime,
    DrvParms->MinDMATransTime));
  DbgPrint((DPRINT_HWDETECT, "TM:Cyls:%d  Heads:%d  Sectors/Trk:%d Capacity:%d\n",
    DrvParms->TMCylinders,
    DrvParms->TMHeads,
    DrvParms->TMSectorsPerTrk,
    (U32)(DrvParms->TMCapacityLo + (DrvParms->TMCapacityHi << 16))));

  DbgPrint((DPRINT_HWDETECT, "TM:SectorCount: 0x%x%x = %d\n",
    DrvParms->TMSectorCountHi,
    DrvParms->TMSectorCountLo,
    (U32)((DrvParms->TMSectorCountHi << 16) + DrvParms->TMSectorCountLo)));

  if (!Atapi && 0 != (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED))
    {
      /* LBA ATA drives always have a sector size of 512 */
      DrvParms->BytesPerSector = 512;
    }
  else
    {
      if (DrvParms->BytesPerSector == 0)
        {
          DbgPrint((DPRINT_HWDETECT, "BytesPerSector is 0. Defaulting to 512\n"));
          DrvParms->BytesPerSector = 512;
        }
      else
        {
          DbgPrint((DPRINT_HWDETECT, "BytesPerSector %d\n", DrvParms->BytesPerSector));
          for (i = 15; i >= 0; i--)
            {
              if (DrvParms->BytesPerSector & (1 << i))
                {
                  DrvParms->BytesPerSector = 1 << i;
                  break;
                }
            }
        }
    }
  DbgPrint((DPRINT_HWDETECT, "BytesPerSector %d\n", DrvParms->BytesPerSector));

  return(TRUE);
}

/*
 * AtapiFindDevices
 *
 * DESCRIPTION:
 *   Find all devices on a controller
 *
 * ARGUMENTS:
 *   CommandPort
 *     Address of the command port
 *	 ControlPort
 *     Address of the control port
 *   DeviceParams
 *     Buffer for drive parameters
 *
 * RETURNS:
 *	  Number of devices found
 */
static U32
AtapiFindDevices(U32 CommandPortBase,
  U32 ControlPortBase,
  PIDE_DRIVE_IDENTIFY DeviceParams)
{
  U32 DevicesFound = 0;
  U32 UnitNumber;
  U32 Retries;
  U8 High, Low;

  DbgPrint((DPRINT_HWDETECT, "AtapiFindDevices(CommandPortBase: %x, ControlPortBase %x)\n",
    CommandPortBase, ControlPortBase));

  for (UnitNumber = 0; UnitNumber < 2; UnitNumber++)
    {
      /* Select drive */
      IDEWriteDriveHead(CommandPortBase,
        IDE_DH_FIXED | (UnitNumber ? IDE_DH_DRV1 : 0));
      KeStallExecutionProcessor(500);

      /* Disable interrupts */
      IDEWriteDriveControl(ControlPortBase,
        IDE_DC_nIEN);
      KeStallExecutionProcessor(500);

      /* Check if a device is attached to the interface */
      IDEWriteCylinderHigh(CommandPortBase, 0xaa);
      IDEWriteCylinderLow(CommandPortBase, 0x55);

      High = IDEReadCylinderHigh(CommandPortBase);
      Low = IDEReadCylinderLow(CommandPortBase);

      IDEWriteCylinderHigh(CommandPortBase, 0);
      IDEWriteCylinderLow(CommandPortBase, 0);

      if (Low != 0x55 || High != 0xaa)
        {
          DbgPrint((DPRINT_HWDETECT, "No Drive found. UnitNumber %d CommandPortBase %x\n",
            UnitNumber, CommandPortBase));
          continue;
        }

      IDEWriteCommand(CommandPortBase, IDE_CMD_RESET);

      for (Retries = 0; Retries < 20000; Retries++)
        {
	      if (!(IDEReadStatus(CommandPortBase) & IDE_SR_BUSY))
	        {
	          break;
	        }
	      KeStallExecutionProcessor(150);
	    }

      if (Retries >= 20000)
        {
          DbgPrint((DPRINT_HWDETECT, "Timeout on drive %d\n", UnitNumber));
          continue;
        }

      High = IDEReadCylinderHigh(CommandPortBase);
      Low = IDEReadCylinderLow(CommandPortBase);

      DbgPrint((DPRINT_HWDETECT, "Check drive %d: High 0x%x Low 0x%x\n",
	     UnitNumber,
	     High,
	     Low));

      if (High == 0xEB && Low == 0x14)
        {
          if (AtapiIdentifyDevice(CommandPortBase,
            ControlPortBase,
            UnitNumber,
            TRUE,
            &DeviceParams[UnitNumber]))
            {
              DbgPrint((DPRINT_HWDETECT, "ATAPI drive found\n"));
              DevicesFound++;
	        }
	      else
	        {
	          DbgPrint((DPRINT_HWDETECT, "No ATAPI drive found\n"));
	    }
	 }
   else
     {
       if (AtapiIdentifyDevice(CommandPortBase,
         ControlPortBase,
         UnitNumber,
         FALSE,
         &DeviceParams[UnitNumber]))
    	    {
            DbgPrint((DPRINT_HWDETECT, "IDE drive found\n"));
    	      DevicesFound++;
    	    }
    	  else
    	    {
    	      DbgPrint((DPRINT_HWDETECT, "No IDE drive found\n"));
    	    }
      }
    }

  /* Reset pending interrupts */
  IDEReadStatus(CommandPortBase);
  /* Reenable interrupts */
  IDEWriteDriveControl(ControlPortBase, 0);
  KeStallExecutionProcessor(500);
  /* Return with drive 0 selected */
  IDEWriteDriveHead(CommandPortBase, IDE_DH_FIXED);
  KeStallExecutionProcessor(500);

  DbgPrint((DPRINT_HWDETECT, "AtapiFindDevices() Done (DevicesFound %d)\n", DevicesFound));

  return(DevicesFound);
}

// TRUE if a controller was found, FALSE if not
static BOOLEAN
AtapiFindCompatiblePciController(U32 SystemIoBusNumber,
	PU32 SystemSlotNumber,
  PU32 CommandPortBase,
  PU32 ControlPortBase,
	PU32 BusMasterPortBase,
  PU32 InterruptVector,
  PU32 DevicesFound,
  PIDE_DRIVE_IDENTIFY DeviceParams,
  PBOOLEAN Again)
{
  PCI_SLOT_NUMBER SlotNumber;
  PCI_COMMON_CONFIG PciConfig;
  U32 DataSize;
  U32 StartDeviceNumber;
  U32 DeviceNumber;
  U32 StartFunctionNumber;
  U32 FunctionNumber;
  BOOLEAN ChannelFound;
  U32 BusMasterBasePort = 0;

  *Again = FALSE;

  /* both channels were claimed: exit */
  if (AtdiskPrimaryClaimed == TRUE &&
    AtdiskSecondaryClaimed == TRUE)
    return FALSE;

  SlotNumber.u.AsULONG = *SystemSlotNumber;
  StartDeviceNumber = SlotNumber.u.bits.DeviceNumber;
  StartFunctionNumber = SlotNumber.u.bits.FunctionNumber;
  for (DeviceNumber = StartDeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
      SlotNumber.u.bits.DeviceNumber = DeviceNumber;
      for (FunctionNumber = StartFunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
          SlotNumber.u.bits.FunctionNumber = FunctionNumber;
          ChannelFound = FALSE;
  
          DataSize = HalGetBusData(PCIConfiguration,
            SystemIoBusNumber,
            SlotNumber.u.AsULONG,
            &PciConfig,
            PCI_COMMON_HDR_LENGTH);
          if (DataSize != PCI_COMMON_HDR_LENGTH)
    	      {
    	        if (FunctionNumber == 0)
    	          {
    	            break;
    	          }
    	         else
    	          {
    	            continue;
    	          }
    	       }
  
      	DbgPrint((DPRINT_HWDETECT, "BaseClass %x  SubClass %x\n", PciConfig.BaseClass, PciConfig.SubClass));
      	if (PciConfig.BaseClass == 0x01 &&
      	    PciConfig.SubClass == 0x01)
        	{
        	   /* both channels are in compatibility mode */
        	   DbgPrint((DPRINT_HWDETECT, "Bus %d  Device %d  Func %d  VenID 0x%x  DevID 0x%x\n",
        		   SystemIoBusNumber,
        		   SlotNumber.u.bits.DeviceNumber,
        		   SlotNumber.u.bits.FunctionNumber,
        		   PciConfig.VendorID,
        		   PciConfig.DeviceID));
        
        	   DbgPrint((DPRINT_HWDETECT, "Found IDE controller in compatibility mode\n"));
    
        	   if (PciConfig.ProgIf & 0x80)
          	   {
          	      DbgPrint((DPRINT_HWDETECT, "Found IDE Bus Master controller!\n"));
          	      if (PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_SPACE)
            	      {
              		  	BusMasterBasePort = PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK;
              		  	DbgPrint((DPRINT_HWDETECT, "IDE Bus Master Registers at IO %x\n", BusMasterBasePort));
            	      }
          	   }
        	   if (AtdiskPrimaryClaimed == FALSE)
          	   {
          	      /* Both channels unclaimed: Claim primary channel */
          	      DbgPrint((DPRINT_HWDETECT, "Primary channel\n"));
                  AtdiskPrimaryClaimed = TRUE;
                  *CommandPortBase = 0x1F0;
                  *ControlPortBase = 0x3F4;
                  *BusMasterPortBase = BusMasterBasePort;
                  *InterruptVector = 14;
          	      *Again = TRUE;
          
          	   }
        	   else if (AtdiskSecondaryClaimed == FALSE)
          	   {
          	      /* Primary channel already claimed: claim secondary channel */
          	      DbgPrint((DPRINT_HWDETECT, "Secondary channel\n"));
                  AtdiskSecondaryClaimed = TRUE;
                  *CommandPortBase = 0x170;
                  *ControlPortBase = 0x374;
                  *BusMasterPortBase = BusMasterBasePort ? BusMasterBasePort + 8 : 0;
                  *InterruptVector = 15;
          	      *Again = FALSE;
          	   }
        
        	   /* Find attached devices */
             *DevicesFound = AtapiFindDevices(*CommandPortBase, *ControlPortBase, DeviceParams);
             *SystemSlotNumber = SlotNumber.u.AsULONG;
             return TRUE;
        	}
  
      	if (FunctionNumber == 0 && !(PciConfig.HeaderType & PCI_MULTIFUNCTION))
        	{
        	   break;
        	}
      }	
      StartFunctionNumber = 0;
    }
  return FALSE;
}

// TRUE if a controller was found, FALSE if not
static BOOLEAN
AtapiFindNativePciController(U32 SystemIoBusNumber,
	PU32 SystemSlotNumber,
  PU32 CommandPortBase,
  PU32 ControlPortBase,
	PU32 BusMasterPortBase,
  PU32 InterruptVector,
  PU32 DevicesFound,
  PIDE_DRIVE_IDENTIFY DeviceParams,
  PBOOLEAN Again)
{
  PCI_COMMON_CONFIG PciConfig;
  PCI_SLOT_NUMBER SlotNumber;
  U32 DataSize;
  U32 DeviceNumber;
  U32 StartDeviceNumber;
  U32 FunctionNumber;
  U32 StartFunctionNumber;
  U32 BusMasterBasePort;
  U32 Count;
  BOOLEAN ChannelFound;

  SlotNumber.u.AsULONG = *SystemSlotNumber;
  StartDeviceNumber = SlotNumber.u.bits.DeviceNumber;
  StartFunctionNumber = SlotNumber.u.bits.FunctionNumber;
  for (DeviceNumber = StartDeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
      SlotNumber.u.bits.DeviceNumber = DeviceNumber;
      for (FunctionNumber = StartFunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
          SlotNumber.u.bits.FunctionNumber = FunctionNumber;
          DataSize = HalGetBusData(PCIConfiguration,
            SystemIoBusNumber,
            SlotNumber.u.AsULONG,
            &PciConfig,
            PCI_COMMON_HDR_LENGTH);
          if (DataSize != PCI_COMMON_HDR_LENGTH)
            {
              break;
            }
          for (Count = 0; Count < sizeof(PciNativeController)/sizeof(PCI_NATIVE_CONTROLLER); Count++)
            {
              if (PciConfig.VendorID == PciNativeController[Count].VendorID &&
                PciConfig.DeviceID == PciNativeController[Count].DeviceID)
                {
                  break;
                }
              }
          if (Count < sizeof(PciNativeController)/sizeof(PCI_NATIVE_CONTROLLER)) 
            {
              /* We have found a known native pci ide controller */
              if ((PciConfig.ProgIf & 0x80) && (PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_SPACE))
                {
                  DbgPrint((DPRINT_HWDETECT, "Found IDE Bus Master controller\n"));
                  BusMasterBasePort = PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK;
                  DbgPrint((DPRINT_HWDETECT, "IDE Bus Master Registers at IO %x\n", BusMasterBasePort));
                }
              else
                {
                  BusMasterBasePort = 0;
                }
          
              DbgPrint((DPRINT_HWDETECT, "VendorID: %04x, DeviceID: %04x\n",
                PciConfig.VendorID, PciConfig.DeviceID));
              ChannelFound = FALSE;
              if (LastSlotNumber.u.AsULONG != SlotNumber.u.AsULONG)
                {
                  /* primary channel */
                  if ((PciConfig.u.type0.BaseAddresses[0] & PCI_ADDRESS_IO_SPACE) &&
                    (PciConfig.u.type0.BaseAddresses[1] & PCI_ADDRESS_IO_SPACE))
                    {
                      /* primary channel is enabled */
                      ChannelFound = TRUE;
                      *CommandPortBase = PciConfig.u.type0.BaseAddresses[0] & PCI_ADDRESS_IO_ADDRESS_MASK;
                      *ControlPortBase = PciConfig.u.type0.BaseAddresses[1] & PCI_ADDRESS_IO_ADDRESS_MASK;
                      *BusMasterPortBase = BusMasterBasePort;
                      *InterruptVector = PciConfig.u.type0.InterruptLine;
    
                      if (ChannelFound)
                        {
                          *DevicesFound = AtapiFindDevices(*CommandPortBase, *ControlPortBase, DeviceParams);
                          *Again = TRUE;
                          *SystemSlotNumber = LastSlotNumber.u.AsULONG = SlotNumber.u.AsULONG;
                          return(TRUE);
                        }
                    }
                }
              if (!ChannelFound)
              {
                /* secondary channel */
                if ((PciConfig.u.type0.BaseAddresses[2] & PCI_ADDRESS_IO_SPACE) &&
                  (PciConfig.u.type0.BaseAddresses[3] & PCI_ADDRESS_IO_SPACE))
                  {
                    /* secondary channel is enabled */
                    *CommandPortBase = PciConfig.u.type0.BaseAddresses[2] & PCI_ADDRESS_IO_ADDRESS_MASK;
                    *ControlPortBase = PciConfig.u.type0.BaseAddresses[3] & PCI_ADDRESS_IO_ADDRESS_MASK;
                    *BusMasterPortBase = BusMasterBasePort ? BusMasterBasePort + 8 : 0;
                    *InterruptVector = PciConfig.u.type0.InterruptLine;
    
                    if (ChannelFound)
                      {
                        *DevicesFound = AtapiFindDevices(*CommandPortBase, *ControlPortBase, DeviceParams);
                        *Again = FALSE;
                        LastSlotNumber.u.AsULONG = 0xFFFFFFFF;
                        return(TRUE);
                      }
                  }
              }
          }
        }
      StartFunctionNumber = 0;
    }
  *Again = FALSE;
  LastSlotNumber.u.AsULONG = 0xFFFFFFFF;
  
  return(FALSE);
}

// TRUE if a controller was found, FALSE if not
static BOOLEAN
AtapiFindIsaBusController(U32 SystemIoBusNumber,
	PU32 SystemSlotNumber,
  PU32 CommandPortBase,
  PU32 ControlPortBase,
	PU32 BusMasterPortBase,
  PU32 InterruptVector,
  PU32 DevicesFound,
  PIDE_DRIVE_IDENTIFY DeviceParams,
  PBOOLEAN Again)
{
  BOOLEAN ChannelFound = FALSE;

  *Again = FALSE;

  if (AtdiskPrimaryClaimed == FALSE)
    {
      /* Both channels unclaimed: Claim primary channel */
      DbgPrint((DPRINT_HWDETECT, "Primary channel\n"));
      AtdiskPrimaryClaimed = TRUE;
      ChannelFound = TRUE;
      *CommandPortBase = 0x1F0;
      *ControlPortBase = 0x3F4;
      *BusMasterPortBase = 0;
      *InterruptVector = 14;
      *Again = TRUE;
    }
  else if (AtdiskSecondaryClaimed == FALSE)
    {
      /* Primary channel already claimed: claim secondary channel */
      DbgPrint((DPRINT_HWDETECT, "Secondary channel\n"));
      AtdiskSecondaryClaimed = TRUE;
      ChannelFound = TRUE;
      *CommandPortBase = 0x170;
      *ControlPortBase = 0x374;
      *BusMasterPortBase = 0;
      *InterruptVector = 15;
      *Again = FALSE;
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "AtapiFindIsaBusController() both channels claimed\n"));
      *Again = FALSE;
      return(FALSE);
    }

  /* Find attached devices */
  if (ChannelFound)
    {
      *DevicesFound = AtapiFindDevices(*CommandPortBase, *ControlPortBase, DeviceParams);
      return(TRUE);
    }
  *Again = FALSE;
  return(FALSE);
}

static VOID
FindIDEControllers(PDETECTED_STORAGE DetectedStorage)
{
  U32 SystemIoBusNumber;
  U32 SystemSlotNumber;
  U32 CommandPortBase;
  U32 ControlPortBase;
  U32 BusMasterPortBase;
  U32 InterruptVector;
  U32 DevicesFound;
  PDETECTED_STORAGE_CONTROLLER StorageController;
  BOOLEAN Again;
  BOOLEAN Found;
  U32 MaxBus;

  AtdiskPrimaryClaimed = FALSE;
  AtdiskSecondaryClaimed = FALSE;

/* Search the PCI bus for all IDE controllers */

  MaxBus = 8; /* Max 8 PCI busses */

  SystemIoBusNumber = 0;
  SystemSlotNumber = 0;

  while (TRUE)
    {
  	  /* Search the PCI bus for compatibility mode IDE controllers */
      StorageController = MmAllocateMemory(sizeof(DETECTED_STORAGE_CONTROLLER));
  	  Found = AtapiFindCompatiblePciController(SystemIoBusNumber,
        &SystemSlotNumber,
        &CommandPortBase,
        &ControlPortBase,
        &BusMasterPortBase,
        &InterruptVector,
        &DevicesFound,
        &StorageController->IdeDriveIdentify[0],
        &Again);
  
        if (Found)
          {
            DbgPrint((DPRINT_HWDETECT, "Found compatible IDE controller\n"));

            StorageController->DriveCount = DevicesFound;
            StorageController->BusType = PCIBus;
            StorageController->BusNumber = SystemIoBusNumber;
            InsertTailList(&DetectedStorage->StorageControllers,
              &StorageController->ListEntry);
          }
        else
          {
            MmFreeMemory(StorageController);
          }
  
        if (Again == FALSE)
          {
            SystemIoBusNumber++;
            SystemSlotNumber = 0;
          }
  
        if (SystemIoBusNumber >= MaxBus)
          {
            DbgPrint((DPRINT_HWDETECT, "Scanned all PCI buses\n"));
            break;
          }
      }

  /* Search the PCI bus for all IDE controllers */

  SystemIoBusNumber = 0;
  SystemSlotNumber = 0;

  LastSlotNumber.u.AsULONG = 0;

  while (TRUE)
    {
  	  /* Search the PCI bus for native PCI IDE controllers */
      StorageController = MmAllocateMemory(sizeof(DETECTED_STORAGE_CONTROLLER));
  	  Found = AtapiFindNativePciController(SystemIoBusNumber,
        &SystemSlotNumber,
        &CommandPortBase,
        &ControlPortBase,
        &BusMasterPortBase,
        &InterruptVector,
        &DevicesFound,
        &StorageController->IdeDriveIdentify[0],
        &Again);

        if (Found)
          {
            DbgPrint((DPRINT_HWDETECT, "Found native PCI IDE controller\n"));

            StorageController->DriveCount = DevicesFound;
            StorageController->BusType = PCIBus;
            StorageController->BusNumber = SystemIoBusNumber;
            InsertTailList(&DetectedStorage->StorageControllers,
              &StorageController->ListEntry);
          }
  
        if (Again == FALSE)
          {
            SystemIoBusNumber++;
            SystemSlotNumber = 0;
          }
        else
          {
            MmFreeMemory(StorageController);
          }
  
        if (SystemIoBusNumber >= MaxBus)
          {
            DbgPrint((DPRINT_HWDETECT, "Scanned all PCI buses\n"));
            break;
          }
      }

  LastSlotNumber.u.AsULONG = 0xFFFFFFFF;

  SystemIoBusNumber = 0;
  SystemSlotNumber = 0;

  LastSlotNumber.u.AsULONG = 0;

  while (TRUE)
    {
  	  /* Search the ISA bus for an IDE controller */
  	  Found = AtapiFindIsaBusController(SystemIoBusNumber,
        &SystemSlotNumber,
        &CommandPortBase,
        &ControlPortBase,
        &BusMasterPortBase,
        &InterruptVector,
        &DevicesFound,
        &StorageController->IdeDriveIdentify[0],
        &Again);
  
      if (Found)
        {
          DbgPrint((DPRINT_HWDETECT, "Found ISA IDE controller\n"));

          StorageController->DriveCount = DevicesFound;
          StorageController->BusType = Isa;
          StorageController->BusNumber = SystemIoBusNumber;
          InsertTailList(&DetectedStorage->StorageControllers,
            &StorageController->ListEntry);
        }
      else
        {
          MmFreeMemory(StorageController);
        }

      if (Again == FALSE)
        {
          break;
        }
    }
}

/* ***** END ATA ***** */

VOID
DetectStorage(VOID)
{
  DETECTED_STORAGE DetectedStorage;
  PDETECTED_STORAGE_CONTROLLER Controller;
  PLIST_ENTRY ListEntry;
  CHAR buf[200];
  S32 Error;
  U32 ScsiPortNumber;
  HKEY ScsiPortKey;

  U32 i;
  HKEY BusKey;
  HKEY DiskControllerKey;
  HKEY DiskPeripheralKey;
  HKEY DriveKey;
  CM_INT13_DRIVE_PARAMETER Int13;

  InitializeListHead(&DetectedStorage.StorageControllers);

  FindIDEControllers(&DetectedStorage);

  ScsiPortNumber = 0;
  ListEntry = DetectedStorage.StorageControllers.Flink;
  while (ListEntry != &DetectedStorage.StorageControllers)
  	{
  	  Controller = CONTAINING_RECORD(ListEntry,
        DETECTED_STORAGE_CONTROLLER,
        ListEntry);

      BusKey = GetBusKey(Controller->BusType, Controller->BusNumber);
    
      /* Create or open DiskController key */
      DiskControllerKey = CreateOrOpenKey(BusKey,
        "DiskController");
      if (DiskControllerKey == NULL)
        {
          DbgPrint((DPRINT_HWDETECT, "CreateOrOpenKey() failed\n"));
          return;
        }

      /* Create X key */
      sprintf(buf, "%d", (int) ScsiPortNumber);
      Error = RegCreateKey(DiskControllerKey,
        buf,
        &ScsiPortKey);
      if (Error != ERROR_SUCCESS)
        {
          DbgPrint((DPRINT_HWDETECT, "RegCreateKey(X) failed (Error %u)\n", (int)Error));
          return;
        }

      /* FIXME: Create value: 'Component information' */

      /* FIXME: Create value: 'Configuration data' */

      /* Create DiskPeripheral key */
      sprintf(buf, "%d", (int) ScsiPortNumber);
      Error = RegCreateKey(ScsiPortKey,
        "DiskPeripheral",
        &DiskPeripheralKey);
      if (Error != ERROR_SUCCESS)
        {
          DbgPrint((DPRINT_HWDETECT, "RegCreateKey(DiskPeripheral) failed (Error %u)\n", (int)Error));
          return;
        }

      for (i = 0; i < Controller->DriveCount; i++)
        {
          /* Create DiskPeripheral/X key */
          sprintf(buf, "%d", (int) i);
          Error = RegCreateKey(DiskPeripheralKey,
            buf,
            &DriveKey);
          if (Error != ERROR_SUCCESS)
            {
              DbgPrint((DPRINT_HWDETECT, "RegCreateKey(DiskPeripheral/X) failed (Error %u)\n", (int)Error));
              return;
            }

          Int13.DriveSelect = 0;  /* FIXME: What is this? */
          Int13.MaxCylinders = Controller->IdeDriveIdentify[i].TMCylinders;
          Int13.SectorsPerTrack = Controller->IdeDriveIdentify[i].TMSectorsPerTrk;
          Int13.MaxHeads = Controller->IdeDriveIdentify[i].TMHeads;
          Int13.NumberDrives = Controller->DriveCount; /* FIXME: This does not make sense */ 
          Error = RegSetValue(DriveKey,
            "Component information",
            REG_BINARY,
            (PU8) &Int13,
            sizeof(CM_INT13_DRIVE_PARAMETER));
          if (Error != ERROR_SUCCESS)
            {
              DbgPrint((DPRINT_HWDETECT, "RegSetValue(Component information) failed (Error %u)\n", (int)Error));
              return;
            }

          /* FIXME: Create value: 'Configuration data' */
        }

      ScsiPortNumber++;
  	  ListEntry = ListEntry->Flink;
  	}

  DbgPrint((DPRINT_HWDETECT, "%d controllers found\n", ScsiPortNumber));
}

VOID
PrepareRegistry()
{
  HKEY HardwareKey;
  HKEY DescriptionKey;
  HKEY SystemKey;
  HKEY DeviceMapKey;
  HKEY ResourceMapKey;
  S32 Error;

  Error = RegOpenKey(NULL,
    "\\Registry\\Machine\\HARDWARE",
    &HardwareKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegOpenKey(HARDWARE) failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create DESCRIPTION key */
  Error = RegCreateKey(HardwareKey,
    "DESCRIPTION",
    &DescriptionKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey(DESCRIPTION) failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create DESCRIPTION/System key */
  Error = RegCreateKey(DescriptionKey,
    "System",
    &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey(System) failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create DEVICEMAP key */
  Error = RegCreateKey(HardwareKey,
    "DEVICEMAP",
    &DeviceMapKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey(DEVICEMAP) failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create RESOURCEMAP key */
  Error = RegCreateKey(HardwareKey,
    "RESOURCEMAP",
    &ResourceMapKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey(RESOURCEMAP) failed (Error %u)\n", (int)Error));
      return;
    }
}

VOID
FindPciBusses(PDETECTED_BUSSES DetectedBusses)
{
  PDETECTED_BUS DetectedBus;

  PciBusConfigType = GetPciBusConfigType();

  if (PciBusConfigType == 0)
    {
      /* No PCI bus present */
    }
  else if (PciBusConfigType == 1)
    {
      /* 1 PCI bus present */
      DetectedBus = MmAllocateMemory(sizeof(DETECTED_BUS));
      if (DetectedBus == NULL)
        return;

      DetectedBus->BusType = PCIBus;
      DetectedBus->BusNumber = 0;
      strcpy(DetectedBus->Identifier, "PCI");
      InsertHeadList(&DetectedBusses->Busses, &DetectedBus->ListEntry);
    }
  else if (PciBusConfigType == 2)
    {
      /* PCI and AGP bus present */
      DetectedBus = MmAllocateMemory(sizeof(DETECTED_BUS));
      if (DetectedBus == NULL)
        return;

      DetectedBus->BusType = PCIBus;
      DetectedBus->BusNumber = 0;
      strcpy(DetectedBus->Identifier, "PCI");
      InsertHeadList(&DetectedBusses->Busses, &DetectedBus->ListEntry);


      DetectedBus = MmAllocateMemory(sizeof(DETECTED_BUS));
      if (DetectedBus == NULL)
        return;

      DetectedBus->BusType = PCIBus;
      DetectedBus->BusNumber = 1;
      strcpy(DetectedBus->Identifier, "PCI");
      InsertHeadList(&DetectedBusses->Busses, &DetectedBus->ListEntry);
    }
  else
    {
      /* Unknown PCI bus configuration */
      DbgPrint((DPRINT_WARNING, "Unknown PCI bus configuration %d\n", PciBusConfigType));
    }
}

VOID
FindIsaBus(PDETECTED_BUSSES DetectedBusses)
{
  PDETECTED_BUS DetectedBus;

  /* Assume an ISA bus is present */

  DetectedBus = MmAllocateMemory(sizeof(DETECTED_BUS));
  if (DetectedBus == NULL)
    return;

  DetectedBus->BusType = Isa;
  DetectedBus->BusNumber = 0;
  strcpy(DetectedBus->Identifier, "ISA");

  InsertHeadList(&DetectedBusses->Busses, &DetectedBus->ListEntry);
}

VOID
DetectBusses()
{
  CHAR buf[200];
  DETECTED_BUSSES DetectedBusses;
  PDETECTED_BUS Bus;
  S32 Error;
  U32 BusNumber;
  PLIST_ENTRY ListEntry;
  HKEY SystemKey;
  HKEY MultifunctionAdapterKey;
  HKEY BusKey;

  InitializeListHead(&DetectedBusses.Busses);
  FindPciBusses(&DetectedBusses);
  FindIsaBus(&DetectedBusses);

  Error = RegOpenKey(NULL,
    "\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
    &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegOpenKey(System) failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create DESCRIPTION/System/MultifunctionAdapter key */
  Error = RegCreateKey(SystemKey,
    "MultifunctionAdapter",
    &MultifunctionAdapterKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey(MultifunctionAdapter) failed (Error %u)\n", (int)Error));
      return;
    }

  BusNumber = 0;
  ListEntry = DetectedBusses.Busses.Flink;
  while (ListEntry != &DetectedBusses.Busses)
  	{
  	  Bus = CONTAINING_RECORD(ListEntry,
        DETECTED_BUS,
        ListEntry);

      /* Create DESCRIPTION/System/MultifunctionAdapter/X key */
      sprintf(buf, "%d", (int) BusNumber);
      Error = RegCreateKey(MultifunctionAdapterKey,
        buf,
        &BusKey);
      if (Error != ERROR_SUCCESS)
        {
          DbgPrint((DPRINT_HWDETECT, "RegCreateKey(X) failed (Error %u)\n", (int)Error));
          return;
        }

      RegisterBusKey(BusKey, Bus->BusType, Bus->BusNumber);

      Error = RegSetValue(BusKey,
        "Identifier",
        REG_SZ,
        (PU8) Bus->Identifier,
        strlen(Bus->Identifier));
      if (Error != ERROR_SUCCESS)
        {
          DbgPrint((DPRINT_HWDETECT, "RegSetValue(Identifier) failed (Error %u)\n", (int)Error));
          return;
        }

      BusNumber++;
  	  ListEntry = ListEntry->Flink;
  	}

  DbgPrint((DPRINT_HWDETECT, "%d busses found\n", BusNumber));
}

VOID
DetectHardware(VOID)
{
  DbgPrint((DPRINT_REACTOS, "DetectHardware()\n"));

  InitializeListHead(&BusKeyListHead);

  HalpCalibrateStallExecution();

  PrepareRegistry();

  DetectBusses();

  DetectStorage();

  DbgPrint((DPRINT_HWDETECT, "DetectHardware() Done\n"));
}
