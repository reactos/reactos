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

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

#define XBOX_IDE_COMMAND_PORT 0x1f0
#define XBOX_IDE_CONTROL_PORT 0x170

#define XBOX_SIGNATURE_SECTOR 3
#define XBOX_SIGNATURE        ('B' | ('R' << 8) | ('F' << 16) | ('R' << 24))

static struct
{
  ULONG SectorCountBeforePartition;
  ULONG PartitionSectorCount;
  UCHAR SystemIndicator;
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
  (__inwordstring(((Address) + IDE_REG_DATA_PORT), (PUSHORT)(Buffer), (Count) / 2))
#define IDEWriteBlock(Address, Buffer, Count) \
  (WRITE_PORT_BUFFER_USHORT((PUSHORT)((Address) + IDE_REG_DATA_PORT), (PUSHORT)(Buffer), (Count) / 2))

#define IDEReadBlock32(Address, Buffer, Count) \
  (READ_PORT_BUFFER_ULONG((PULONG)((Address) + IDE_REG_DATA_PORT), (PULONG)(Buffer), (Count) / 4))
#define IDEWriteBlock32(Address, Buffer, Count) \
  (WRITE_PORT_BUFFER_ULONG((PULONG)((Address) + IDE_REG_DATA_PORT), (PULONG)(Buffer), (Count) / 4))

#define IDEReadWord(Address) \
  (READ_PORT_USHORT((PUSHORT)((Address) + IDE_REG_DATA_PORT)))

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
  USHORT   ConfigBits;          /*00*/
  USHORT   LogicalCyls;         /*01*/
  USHORT   Reserved02;          /*02*/
  USHORT   LogicalHeads;        /*03*/
  USHORT   BytesPerTrack;       /*04*/
  USHORT   BytesPerSector;      /*05*/
  USHORT   SectorsPerTrack;     /*06*/
  UCHAR    InterSectorGap;      /*07*/
  UCHAR    InterSectorGapSize;
  UCHAR    Reserved08H;         /*08*/
  UCHAR    BytesInPLO;
  USHORT   VendorUniqueCnt;     /*09*/
  char  SerialNumber[20];    /*10*/
  USHORT   ControllerType;      /*20*/
  USHORT   BufferSize;          /*21*/
  USHORT   ECCByteCnt;          /*22*/
  char  FirmwareRev[8];      /*23*/
  char  ModelNumber[40];     /*27*/
  USHORT   RWMultImplemented;   /*47*/
  USHORT   DWordIo;	     /*48*/
  USHORT   Capabilities;        /*49*/
#define IDE_DRID_STBY_SUPPORTED   0x2000
#define IDE_DRID_IORDY_SUPPORTED  0x0800
#define IDE_DRID_IORDY_DISABLE    0x0400
#define IDE_DRID_LBA_SUPPORTED    0x0200
#define IDE_DRID_DMA_SUPPORTED    0x0100
  USHORT   Reserved50;          /*50*/
  USHORT   MinPIOTransTime;     /*51*/
  USHORT   MinDMATransTime;     /*52*/
  USHORT   TMFieldsValid;       /*53*/
  USHORT   TMCylinders;         /*54*/
  USHORT   TMHeads;             /*55*/
  USHORT   TMSectorsPerTrk;     /*56*/
  USHORT   TMCapacityLo;        /*57*/
  USHORT   TMCapacityHi;        /*58*/
  USHORT   RWMultCurrent;       /*59*/
  USHORT   TMSectorCountLo;     /*60*/
  USHORT   TMSectorCountHi;     /*61*/
  USHORT   DmaModes;            /*62*/
  USHORT   MultiDmaModes;       /*63*/
  USHORT   Reserved64[5];       /*64*/
  USHORT   Reserved69[2];       /*69*/
  USHORT   Reserved71[4];       /*71*/
  USHORT   MaxQueueDepth;       /*75*/
  USHORT   Reserved76[4];       /*76*/
  USHORT   MajorRevision;       /*80*/
  USHORT   MinorRevision;       /*81*/
  USHORT   SupportedFeatures82; /*82*/
  USHORT   SupportedFeatures83; /*83*/
  USHORT   SupportedFeatures84; /*84*/
  USHORT   EnabledFeatures85;   /*85*/
  USHORT   EnabledFeatures86;   /*86*/
  USHORT   EnabledFeatures87;   /*87*/
  USHORT   UltraDmaModes;       /*88*/
  USHORT   Reserved89[11];      /*89*/
  USHORT   Max48BitAddress[4];  /*100*/
  USHORT   Reserved104[151];    /*104*/
  USHORT   Checksum;            /*255*/
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
 *    ULONG   CommandPort   Address of command port for drive
 *    ULONG   ControlPort   Address of control port for drive
 *    UCHAR    PreComp       Value to write to precomp register
 *    UCHAR    SectorCnt     Value to write to sectorCnt register
 *    UCHAR    SectorNum     Value to write to sectorNum register
 *    UCHAR    CylinderLow   Value to write to CylinderLow register
 *    UCHAR    CylinderHigh  Value to write to CylinderHigh register
 *    UCHAR    DrvHead       Value to write to Drive/Head register
 *    UCHAR    Command       Value to write to Command register
 *    PVOID Buffer        Buffer for output data
 *
 *  RETURNS:
 *    BOOLEAN: TRUE success, FALSE error
 */

static BOOLEAN
XboxDiskPolledRead(ULONG CommandPort,
                   ULONG ControlPort,
                   UCHAR PreComp,
                   UCHAR SectorCnt,
                   UCHAR SectorNum,
                   UCHAR CylinderLow,
                   UCHAR CylinderHigh,
                   UCHAR DrvHead,
                   UCHAR Command,
                   PVOID Buffer)
{
  ULONG SectorCount = 0;
  ULONG RetryCount;
  BOOLEAN Junk = FALSE;
  UCHAR Status;

  /* Wait for BUSY to clear */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      StallExecutionProcessor(10);
    }
  DPRINTM(DPRINT_DISK, "status=0x%x\n", Status);
  DPRINTM(DPRINT_DISK, "waited %d usecs for busy to clear\n", RetryCount * 10);
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINTM(DPRINT_DISK, "Drive is BUSY for too long\n");
      return FALSE;
    }

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);
  StallExecutionProcessor(500);

  /* Disable interrupts */
  IDEWriteDriveControl(ControlPort, IDE_DC_nIEN);
  StallExecutionProcessor(500);

  /*  Issue command to drive  */
  if (DrvHead & IDE_DH_LBA)
    {
      DPRINTM(DPRINT_DISK, "READ:DRV=%d:LBA=1:BLK=%d:SC=0x%x:CM=0x%x\n",
                DrvHead & IDE_DH_DRV1 ? 1 : 0,
                ((DrvHead & 0x0f) << 24) + (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
                SectorCnt,
                Command);
    }
  else
    {
      DPRINTM(DPRINT_DISK, "READ:DRV=%d:LBA=0:CH=0x%x:CL=0x%x:HD=0x%x:SN=0x%x:SC=0x%x:CM=0x%x\n",
                DrvHead & IDE_DH_DRV1 ? 1 : 0,
                CylinderHigh,
                CylinderLow,
                DrvHead & 0x0f,
                SectorNum,
                SectorCnt,
                Command);
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
  StallExecutionProcessor(50);

  /*  wait for DRQ or error  */
  for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
      if (!(Status & IDE_SR_BUSY))
	{
	  if (Status & IDE_SR_ERR)
	    {
	      IDEWriteDriveControl(ControlPort, 0);
	      StallExecutionProcessor(50);
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
	      StallExecutionProcessor(50);
	      IDEReadStatus(CommandPort);

	      return FALSE;
	    }
	}
      StallExecutionProcessor(10);
    }

  /*  timed out  */
  if (RetryCount >= IDE_MAX_POLL_RETRIES)
    {
      IDEWriteDriveControl(ControlPort, 0);
      StallExecutionProcessor(50);
      IDEReadStatus(CommandPort);

      return FALSE;
    }

  while (1)
    {
      /*  Read data into buffer  */
      if (Junk == FALSE)
	{
	  IDEReadBlock(CommandPort, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer = (PVOID)((ULONG_PTR)Buffer + IDE_SECTOR_BUF_SZ);
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
		  StallExecutionProcessor(50);
		  IDEReadStatus(CommandPort);

		  return FALSE;
		}
	      if (Status & IDE_SR_DRQ)
		{
		  if (SectorCount >= SectorCnt)
		    {
		      DPRINTM(DPRINT_DISK, "Buffer size exceeded!\n");
		      Junk = TRUE;
		    }
		  break;
		}
	      else
		{
		  if (SectorCount > SectorCnt)
		    {
		      DPRINTM(DPRINT_DISK, "Read %lu sectors of junk!\n",
                                SectorCount - SectorCnt);
		    }
		  IDEWriteDriveControl(ControlPort, 0);
		  StallExecutionProcessor(50);
		  IDEReadStatus(CommandPort);

		  return TRUE;
		}
	    }
	}
    }
}

BOOLEAN
XboxDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
  ULONG StartSector;
  UCHAR Count;

  if (DriveNumber < 0x80 || 2 <= (DriveNumber & 0x0f))
    {
      /* Xbox has only 1 IDE controller and no floppy */
      DPRINTM(DPRINT_DISK, "Invalid drive number\n");
      return FALSE;
    }

  if (UINT64_C(0) != ((SectorNumber + SectorCount) & UINT64_C(0xfffffffff0000000)))
    {
      DPRINTM(DPRINT_DISK, "48bit LBA required but not implemented\n");
      return FALSE;
    }

  StartSector = (ULONG) SectorNumber;
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

BOOLEAN
XboxDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  UCHAR SectorData[IDE_SECTOR_BUF_SZ];

  /* This is the Xbox, chances are that there is a Xbox-standard partitionless
   * disk in it so let's check that first */

  if (1 <= PartitionNumber && PartitionNumber <= sizeof(XboxPartitions) / sizeof(XboxPartitions[0]) &&
      MachDiskReadLogicalSectors(DriveNumber, XBOX_SIGNATURE_SECTOR, 1, SectorData))
    {
      if (*((PULONG) SectorData) == XBOX_SIGNATURE)
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

BOOLEAN
XboxDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY Geometry)
{
  IDE_DRIVE_IDENTIFY DrvParms;
  ULONG i;
  BOOLEAN Atapi;

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
      DPRINTM(DPRINT_DISK, "XboxDiskPolledRead() failed\n");
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
      DPRINTM(DPRINT_DISK, "BytesPerSector %d\n", DrvParms.BytesPerSector);
      if (DrvParms.BytesPerSector == 0)
        {
          Geometry->BytesPerSector = 512;
        }
      else
        {
          for (i = 1 << 15; i; i /= 2)
            {
              if (0 != (DrvParms.BytesPerSector & i))
                {
                  Geometry->BytesPerSector = i;
                  break;
                }
            }
        }
    }
  DPRINTM(DPRINT_DISK, "Cylinders %d\n", Geometry->Cylinders);
  DPRINTM(DPRINT_DISK, "Heads %d\n", Geometry->Heads);
  DPRINTM(DPRINT_DISK, "Sectors %d\n", Geometry->Sectors);
  DPRINTM(DPRINT_DISK, "BytesPerSector %d\n", Geometry->BytesPerSector);

  return TRUE;
}

ULONG
XboxDiskGetCacheableBlockCount(ULONG DriveNumber)
{
  /* 64 seems a nice number, it is used by the machpc code for LBA devices */
  return 64;
}

/* EOF */
