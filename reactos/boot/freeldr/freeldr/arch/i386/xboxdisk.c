/* $Id$
 *
 *  FreeLoader
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
 *
 * Note: mostly ripped from atapi.c
 *       Some of this code was based on knowledge and/or code developed
 *       by the Xbox Linux group: http://www.xbox-linux.org
 *
 */

#include "freeldr.h"
#include "debug.h"
#include "hardware.h"
#include "machine.h"
#include "machxbox.h"
#include "portio.h"
#include "rtl.h"

#define XBOX_IDE_COMMAND_PORT 0x1f0
#define XBOX_IDE_CONTROL_PORT 0x170

#define XBOX_SIGNATURE_SECTOR 3
#define XBOX_SIGNATURE        ('B' | ('R' << 8) | ('F' << 16) | ('R' << 24))

static struct
{
  U32 SectorCountBeforePartition;
  U32 PartitionSectorCount;
  U8 SystemIndicator;
} XboxPartitions[] =
{
  /* This is in the \Device\Harddisk0\Partition.. order used by the Xbox kernel */
  { 0x0055F400, 0x0098f800, PARTITION_FAT32  }, /* Store, E: */
  { 0x00465400, 0x000FA000, PARTITION_FAT_16 }, /* System, C: */
  { 0x00000400, 0x00177000, PARTITION_FAT_16 }, /* Cache1, X: */
  { 0x00177400, 0x00177000, PARTITION_FAT_16 }, /* Cache2, Y: */
  { 0x002EE400, 0x00177000, PARTITION_FAT_16 }  /* Cache3, Z: */
};

#define  IDE_SECTOR_BUF_SZ         512
#define  IDE_MAX_POLL_RETRIES      100000
#define  IDE_MAX_BUSY_RETRIES      50000

/* Control Block offsets and masks */
#define  IDE_REG_ALT_STATUS     0x0000
#define  IDE_REG_DEV_CNTRL      0x0000  /* device control register */
#define    IDE_DC_SRST            0x04  /* drive reset (both drives) */
#define    IDE_DC_nIEN            0x02  /* IRQ enable (active low) */
#define  IDE_REG_DRV_ADDR       0x0001

/* Command Block offsets and masks */
#define  IDE_REG_DATA_PORT      0x0000
#define  IDE_REG_ERROR          0x0001  /* error register */
#define    IDE_ER_AMNF            0x01  /* addr mark not found */
#define    IDE_ER_TK0NF           0x02  /* track 0 not found */
#define    IDE_ER_ABRT            0x04  /* command aborted */
#define    IDE_ER_MCR             0x08  /* media change requested */
#define    IDE_ER_IDNF            0x10  /* ID not found */
#define    IDE_ER_MC              0x20  /* Media changed */
#define    IDE_ER_UNC             0x40  /* Uncorrectable data error */
#define  IDE_REG_PRECOMP        0x0001
#define  IDE_REG_SECTOR_CNT     0x0002
#define  IDE_REG_SECTOR_NUM     0x0003
#define  IDE_REG_CYL_LOW        0x0004
#define  IDE_REG_CYL_HIGH       0x0005
#define  IDE_REG_DRV_HEAD       0x0006
#define    IDE_DH_FIXED           0xA0
#define    IDE_DH_LBA             0x40
#define    IDE_DH_HDMASK          0x0F
#define    IDE_DH_DRV0            0x00
#define    IDE_DH_DRV1            0x10
#define  IDE_REG_STATUS           0x0007
#define    IDE_SR_BUSY              0x80
#define    IDE_SR_DRDY              0x40
#define    IDE_SR_WERR              0x20
#define    IDE_SR_DRQ               0x08
#define    IDE_SR_ERR               0x01
#define  IDE_REG_COMMAND          0x0007

/* IDE/ATA commands */
#define    IDE_CMD_RESET            0x08
#define    IDE_CMD_READ             0x20
#define    IDE_CMD_READ_RETRY       0x21
#define    IDE_CMD_WRITE            0x30
#define    IDE_CMD_WRITE_RETRY      0x31
#define    IDE_CMD_PACKET           0xA0
#define    IDE_CMD_READ_MULTIPLE    0xC4
#define    IDE_CMD_WRITE_MULTIPLE   0xC5
#define    IDE_CMD_READ_DMA         0xC8
#define    IDE_CMD_WRITE_DMA        0xCA
#define    IDE_CMD_FLUSH_CACHE      0xE7
#define    IDE_CMD_FLUSH_CACHE_EXT  0xEA
#define    IDE_CMD_IDENT_ATA_DRV    0xEC
#define    IDE_CMD_IDENT_ATAPI_DRV  0xA1
#define    IDE_CMD_GET_MEDIA_STATUS 0xDA

/*
 *  Access macros for command registers
 *  Each macro takes an address of the command port block, and data
 */
#define IDEReadError(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_ERROR)))
#define IDEWritePrecomp(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_PRECOMP), (Data)))
#define IDEReadSectorCount(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_SECTOR_CNT)))
#define IDEWriteSectorCount(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_SECTOR_CNT), (Data)))
#define IDEReadSectorNum(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_SECTOR_NUM)))
#define IDEWriteSectorNum(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_SECTOR_NUM), (Data)))
#define IDEReadCylinderLow(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_CYL_LOW)))
#define IDEWriteCylinderLow(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_CYL_LOW), (Data)))
#define IDEReadCylinderHigh(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_CYL_HIGH)))
#define IDEWriteCylinderHigh(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_CYL_HIGH), (Data)))
#define IDEReadDriveHead(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_DRV_HEAD)))
#define IDEWriteDriveHead(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_DRV_HEAD), (Data)))
#define IDEReadStatus(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_STATUS)))
#define IDEWriteCommand(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_COMMAND), (Data)))
#define IDEReadDMACommand(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address))))
#define IDEWriteDMACommand(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address)), (Data)))
#define IDEReadDMAStatus(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + 2)))
#define IDEWriteDMAStatus(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + 2), (Data)))
#define IDEWritePRDTable(Address, Data) \
  (WRITE_PORT_ULONG((PULONG)((Address) + 4), (Data)))  

/*
 *  Data block read and write commands
 */
#define IDEReadBlock(Address, Buffer, Count) \
  (READ_PORT_BUFFER_USHORT((PU16)((Address) + IDE_REG_DATA_PORT), (PU16)(Buffer), (Count) / 2))
#define IDEWriteBlock(Address, Buffer, Count) \
  (WRITE_PORT_BUFFER_USHORT((PU16)((Address) + IDE_REG_DATA_PORT), (PU16)(Buffer), (Count) / 2))

#define IDEReadBlock32(Address, Buffer, Count) \
  (READ_PORT_BUFFER_ULONG((PU32)((Address) + IDE_REG_DATA_PORT), (PU32)(Buffer), (Count) / 4))
#define IDEWriteBlock32(Address, Buffer, Count) \
  (WRITE_PORT_BUFFER_ULONG((PU32)((Address) + IDE_REG_DATA_PORT), (PU32)(Buffer), (Count) / 4))

#define IDEReadWord(Address) \
  (READ_PORT_USHORT((PU16)((Address) + IDE_REG_DATA_PORT)))

/*
 *  Access macros for control registers
 *  Each macro takes an address of the control port blank and data
 */
#define IDEReadAltStatus(Address) \
  (READ_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_ALT_STATUS)))
#define IDEWriteDriveControl(Address, Data) \
  (WRITE_PORT_UCHAR((PUCHAR)((Address) + IDE_REG_DEV_CNTRL), (Data)))

/* IDE_DRIVE_IDENTIFY */

typedef struct _IDE_DRIVE_IDENTIFY
{
  U16   ConfigBits;          /*00*/
  U16   LogicalCyls;         /*01*/
  U16   Reserved02;          /*02*/
  U16   LogicalHeads;        /*03*/
  U16   BytesPerTrack;       /*04*/
  U16   BytesPerSector;      /*05*/
  U16   SectorsPerTrack;     /*06*/
  U8    InterSectorGap;      /*07*/
  U8    InterSectorGapSize;
  U8    Reserved08H;         /*08*/
  U8    BytesInPLO;
  U16   VendorUniqueCnt;     /*09*/
  char  SerialNumber[20];    /*10*/
  U16   ControllerType;      /*20*/
  U16   BufferSize;          /*21*/
  U16   ECCByteCnt;          /*22*/
  char  FirmwareRev[8];      /*23*/
  char  ModelNumber[40];     /*27*/
  U16   RWMultImplemented;   /*47*/
  U16   DWordIo;	     /*48*/
  U16   Capabilities;        /*49*/
#define IDE_DRID_STBY_SUPPORTED   0x2000
#define IDE_DRID_IORDY_SUPPORTED  0x0800
#define IDE_DRID_IORDY_DISABLE    0x0400
#define IDE_DRID_LBA_SUPPORTED    0x0200
#define IDE_DRID_DMA_SUPPORTED    0x0100
  U16   Reserved50;          /*50*/
  U16   MinPIOTransTime;     /*51*/
  U16   MinDMATransTime;     /*52*/
  U16   TMFieldsValid;       /*53*/
  U16   TMCylinders;         /*54*/
  U16   TMHeads;             /*55*/
  U16   TMSectorsPerTrk;     /*56*/
  U16   TMCapacityLo;        /*57*/
  U16   TMCapacityHi;        /*58*/
  U16   RWMultCurrent;       /*59*/
  U16   TMSectorCountLo;     /*60*/
  U16   TMSectorCountHi;     /*61*/
  U16   DmaModes;            /*62*/
  U16   MultiDmaModes;       /*63*/
  U16   Reserved64[5];       /*64*/
  U16   Reserved69[2];       /*69*/
  U16   Reserved71[4];       /*71*/
  U16   MaxQueueDepth;       /*75*/
  U16   Reserved76[4];       /*76*/
  U16   MajorRevision;       /*80*/
  U16   MinorRevision;       /*81*/
  U16   SupportedFeatures82; /*82*/
  U16   SupportedFeatures83; /*83*/
  U16   SupportedFeatures84; /*84*/
  U16   EnabledFeatures85;   /*85*/
  U16   EnabledFeatures86;   /*86*/
  U16   EnabledFeatures87;   /*87*/
  U16   UltraDmaModes;       /*88*/
  U16   Reserved89[11];      /*89*/
  U16   Max48BitAddress[4];  /*100*/
  U16   Reserved104[151];    /*104*/
  U16   Checksum;            /*255*/
} IDE_DRIVE_IDENTIFY, *PIDE_DRIVE_IDENTIFY;

/*  XboxDiskPolledRead
 *
 *  DESCRIPTION:
 *    Read a sector of data from the drive in a polled fashion.
 *
 *  RUN LEVEL:
 *    PASSIVE_LEVEL
 *
 *  ARGUMENTS:
 *    U32   CommandPort   Address of command port for drive
 *    U32   ControlPort   Address of control port for drive
 *    U8    PreComp       Value to write to precomp register
 *    U8    SectorCnt     Value to write to sectorCnt register
 *    U8    SectorNum     Value to write to sectorNum register
 *    U8    CylinderLow   Value to write to CylinderLow register
 *    U8    CylinderHigh  Value to write to CylinderHigh register
 *    U8    DrvHead       Value to write to Drive/Head register
 *    U8    Command       Value to write to Command register
 *    PVOID Buffer        Buffer for output data
 *
 *  RETURNS:
 *    BOOL: TRUE success, FALSE error
 */

static BOOL
XboxDiskPolledRead(U32 CommandPort,
                   U32 ControlPort,
                   U8 PreComp,
                   U8 SectorCnt,
                   U8 SectorNum,
                   U8 CylinderLow,
                   U8 CylinderHigh,
                   U8 DrvHead,
                   U8 Command,
                   PVOID Buffer)
{
  U32 SectorCount = 0;
  U32 RetryCount;
  BOOL Junk = FALSE;
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
  DbgPrint((DPRINT_DISK, "status=0x%x\n", Status));
  DbgPrint((DPRINT_DISK, "waited %d usecs for busy to clear\n", RetryCount * 10));
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      DbgPrint((DPRINT_DISK, "Drive is BUSY for too long\n"));
      return FALSE;
    }

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);
  KeStallExecutionProcessor(500);

  /* Disable interrupts */
  IDEWriteDriveControl(ControlPort, IDE_DC_nIEN);
  KeStallExecutionProcessor(500);

  /*  Issue command to drive  */
  if (DrvHead & IDE_DH_LBA)
    {
      DbgPrint((DPRINT_DISK, "READ:DRV=%d:LBA=1:BLK=%d:SC=0x%x:CM=0x%x\n",
                DrvHead & IDE_DH_DRV1 ? 1 : 0,
                ((DrvHead & 0x0f) << 24) + (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
                SectorCnt,
                Command));
    }
  else
    {
      DbgPrint((DPRINT_DISK, "READ:DRV=%d:LBA=0:CH=0x%x:CL=0x%x:HD=0x%x:SN=0x%x:SC=0x%x:CM=0x%x\n",
                DrvHead & IDE_DH_DRV1 ? 1 : 0,
                CylinderHigh,
                CylinderLow,
                DrvHead & 0x0f,
                SectorNum,
                SectorCnt,
                Command));
    }

  /*  Setup command parameters  */
  IDEWritePrecomp(CommandPort, PreComp);
  IDEWriteSectorCount(CommandPort, SectorCnt);
  IDEWriteSectorNum(CommandPort, SectorNum);
  IDEWriteCylinderHigh(CommandPort, CylinderHigh);
  IDEWriteCylinderLow(CommandPort, CylinderLow);
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);

  /*  Issue the command  */
  IDEWriteCommand(CommandPort, Command);
  KeStallExecutionProcessor(50);

  /*  wait for DRQ or error  */
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

	      return FALSE;
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

	      return FALSE;
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

      return FALSE;
    }

  while (1)
    {
      /*  Read data into buffer  */
      if (Junk == FALSE)
	{
	  IDEReadBlock(CommandPort, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer += IDE_SECTOR_BUF_SZ;
	}
      else
	{
	  UCHAR JunkBuffer[IDE_SECTOR_BUF_SZ];
	  IDEReadBlock(CommandPort, JunkBuffer, IDE_SECTOR_BUF_SZ);
	}
      SectorCount++;

      /*  Check for error or more sectors to read  */
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

		  return FALSE;
		}
	      if (Status & IDE_SR_DRQ)
		{
		  if (SectorCount >= SectorCnt)
		    {
		      DbgPrint((DPRINT_DISK, "Buffer size exceeded!\n"));
		      Junk = TRUE;
		    }
		  break;
		}
	      else
		{
		  if (SectorCount > SectorCnt)
		    {
		      DbgPrint((DPRINT_DISK, "Read %lu sectors of junk!\n",
                                SectorCount - SectorCnt));
		    }
		  IDEWriteDriveControl(ControlPort, 0);
		  KeStallExecutionProcessor(50);
		  IDEReadStatus(CommandPort);

		  return TRUE;
		}
	    }
	}
    }
}

BOOL
XboxDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{
  U32 StartSector;
  U8 Count;

  if (DriveNumber < 0x80 || 2 <= (DriveNumber & 0x0f))
    {
      /* Xbox has only 1 IDE controller and no floppy */
      DbgPrint((DPRINT_DISK, "Invalid drive number\n"));
      return FALSE;
    }

  if (UINT64_C(0) != ((SectorNumber + SectorCount) & UINT64_C(0xfffffffff0000000)))
    {
      DbgPrint((DPRINT_DISK, "48bit LBA required but not implemented\n"));
      return FALSE;
    }

  StartSector = (U32) SectorNumber;
  while (0 < SectorCount)
    {
      Count = (SectorCount <= 255 ? SectorCount : 255);
      if (! XboxDiskPolledRead(XBOX_IDE_COMMAND_PORT,
                               XBOX_IDE_CONTROL_PORT,
                               0, Count,
                               StartSector & 0xff,
                               (StartSector >> 8) & 0xff,
                               (StartSector >> 16) & 0xff,
                               ((StartSector >> 24) & 0x0f) | IDE_DH_LBA |
                               (0 == (DriveNumber & 0x0f) ? IDE_DH_DRV0 : IDE_DH_DRV1),
                               IDE_CMD_READ,
                                Buffer))
        {
          return FALSE;
        }
      SectorCount -= Count;
      Buffer = (PVOID) ((PCHAR) Buffer + Count * IDE_SECTOR_BUF_SZ);
    }

  return TRUE;
}

BOOL
XboxDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  U8 SectorData[IDE_SECTOR_BUF_SZ];

  /* This is the Xbox, chances are that there is a Xbox-standard partitionless
   * disk in it so let's check that first */

  if (1 <= PartitionNumber && PartitionNumber <= sizeof(XboxPartitions) / sizeof(XboxPartitions[0]) &&
      MachDiskReadLogicalSectors(DriveNumber, XBOX_SIGNATURE_SECTOR, 1, SectorData))
    {
      if (*((PU32) SectorData) == XBOX_SIGNATURE)
        {
          memset(PartitionTableEntry, 0, sizeof(PARTITION_TABLE_ENTRY));
          PartitionTableEntry->SystemIndicator = XboxPartitions[PartitionNumber - 1].SystemIndicator;
          PartitionTableEntry->SectorCountBeforePartition = XboxPartitions[PartitionNumber - 1].SectorCountBeforePartition;
          PartitionTableEntry->PartitionSectorCount = XboxPartitions[PartitionNumber - 1].PartitionSectorCount;
          return TRUE;
        }
    }

  /* No magic Xbox partitions. Maybe there's a MBR */
  return DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

BOOL
XboxDiskGetDriveGeometry(U32 DriveNumber, PGEOMETRY Geometry)
{
  IDE_DRIVE_IDENTIFY DrvParms;
  U32 i;
  BOOL Atapi;

  Atapi = FALSE; /* FIXME */
  /*  Get the Drive Identify block from drive or die  */
  if (! XboxDiskPolledRead(XBOX_IDE_COMMAND_PORT,
                           XBOX_IDE_CONTROL_PORT,
                           0,
                           1,
                           0,
                           0,
                           0,
                           (0 == (DriveNumber & 0x0f) ? IDE_DH_DRV0 : IDE_DH_DRV1),
                           (Atapi ? IDE_CMD_IDENT_ATAPI_DRV : IDE_CMD_IDENT_ATA_DRV),
                           (PUCHAR) &DrvParms))
    {
      DbgPrint((DPRINT_DISK, "XboxDiskPolledRead() failed\n"));
      return FALSE;
    }

  Geometry->Cylinders = DrvParms.LogicalCyls;
  Geometry->Heads = DrvParms.LogicalHeads;
  Geometry->Sectors = DrvParms.SectorsPerTrack;

  if (! Atapi && 0 != (DrvParms.Capabilities & IDE_DRID_LBA_SUPPORTED))
    {
      /* LBA ATA drives always have a sector size of 512 */
      Geometry->BytesPerSector = 512;
    }
  else
    {
      DbgPrint((DPRINT_DISK, "BytesPerSector %d\n", DrvParms.BytesPerSector));
      if (DrvParms.BytesPerSector == 0)
        {
          Geometry->BytesPerSector = 512;
        }
      else
        {
          for (i = 15; i >= 0; i--)
            {
              if (0 != (DrvParms.BytesPerSector & (1 << i)))
                {
                  Geometry->BytesPerSector = 1 << i;
                  break;
                }
            }
        }
    }
  DbgPrint((DPRINT_DISK, "Cylinders %d\n", Geometry->Cylinders));
  DbgPrint((DPRINT_DISK, "Heads %d\n", Geometry->Heads));
  DbgPrint((DPRINT_DISK, "Sectors %d\n", Geometry->Sectors));
  DbgPrint((DPRINT_DISK, "BytesPerSector %d\n", Geometry->BytesPerSector));

  return TRUE;
}

U32
XboxDiskGetCacheableBlockCount(U32 DriveNumber)
{
  /* 64 seems a nice number, it is used by the machpc code for LBA devices */
  return 64;
}

/* EOF */
