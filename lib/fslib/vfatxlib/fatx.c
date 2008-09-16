/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFATX filesystem library
 * FILE:        fatx.c
 * PURPOSE:     Fatx support
 * PROGRAMMERS: 
 * REVISIONS:
 */
#include "vfatxlib.h"

#define NDEBUG
#include <debug.h>

static ULONG
GetShiftCount(ULONG Value)
{
  ULONG i = 1;
  while (Value > 0)
    {
      i++;
      Value /= 2;
    }
  return i - 2;
}


static ULONG
CalcVolumeSerialNumber(VOID)
{
  LARGE_INTEGER SystemTime;
  TIME_FIELDS TimeFields;
  ULONG Serial;
  PUCHAR Buffer;

  NtQuerySystemTime (&SystemTime);
  RtlTimeToTimeFields (&SystemTime, &TimeFields);

  Buffer = (PUCHAR)&Serial;
  Buffer[0] = (UCHAR)(TimeFields.Year & 0xFF) + (UCHAR)(TimeFields.Hour & 0xFF);
  Buffer[1] = (UCHAR)(TimeFields.Year >> 8) + (UCHAR)(TimeFields.Minute & 0xFF);
  Buffer[2] = (UCHAR)(TimeFields.Month & 0xFF) + (UCHAR)(TimeFields.Second & 0xFF);
  Buffer[3] = (UCHAR)(TimeFields.Day & 0xFF) + (UCHAR)(TimeFields.Milliseconds & 0xFF);

  return Serial;
}


static NTSTATUS
FatxWriteBootSector (IN HANDLE FileHandle,
		     IN PFATX_BOOT_SECTOR BootSector,
		     IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR NewBootSector;
  LARGE_INTEGER FileOffset;

  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    sizeof(FATX_BOOT_SECTOR));
  if (NewBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the new bootsector */
  memset(NewBootSector, 0, sizeof(FATX_BOOT_SECTOR));

  /* Copy FAT16 BPB to new bootsector */
  memcpy(NewBootSector, BootSector, 18); /* FAT16 BPB length (up to (not including) Res2) */

  /* Write sector 0 */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    NewBootSector,
    sizeof(FATX_BOOT_SECTOR),
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);
      return Status;
    }

  VfatxUpdateProgress (Context, 1);

  /* Free the new boot sector */
  RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);

  return Status;
}


static NTSTATUS
Fatx16WriteFAT (IN HANDLE FileHandle,
	        IN ULONG SectorOffset,
		IN ULONG FATSectors,
	        IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG i;
  ULONG Sectors;

  /* Allocate buffer */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    32 * 1024);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, 32 * 1024);

  /* FAT cluster 0 */
  Buffer[0] = 0xf8; /* Media type */
  Buffer[1] = 0xff;

  /* FAT cluster 1 */
  Buffer[2] = 0xff; /* Clean shutdown, no disk read/write errors, end-of-cluster (EOC) mark */
  Buffer[3] = 0xff;

  /* Write first sector of the FAT */
  FileOffset.QuadPart = (SectorOffset * 512) + sizeof(FATX_BOOT_SECTOR);
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    Buffer,
    512,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
      return(Status);
    }

  VfatxUpdateProgress (Context, 1);

  /* Zero the begin of the buffer */
  memset(Buffer, 0, 4);

  /* Zero the rest of the FAT */
  Sectors = 32 * 1024 / 512;
  for (i = 1; i < FATSectors; i += Sectors)
    {
      /* Zero some sectors of the FAT */
      FileOffset.QuadPart = (SectorOffset + i) * 512 + sizeof(FATX_BOOT_SECTOR) ;
      if ((FATSectors - i) <= Sectors)
	{
	  Sectors = FATSectors - i;
	}

      Status = NtWriteFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Buffer,
        Sectors * 512,
        &FileOffset,
        NULL);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
          RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
          return(Status);
        }

      VfatxUpdateProgress (Context, Sectors);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}

static NTSTATUS
Fatx32WriteFAT (IN HANDLE FileHandle,
	        IN ULONG SectorOffset,
	        IN ULONG FATSectors,
	        IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG i;
  ULONG Sectors;

  /* Allocate buffer */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    64 * 1024);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, 64 * 1024);

  /* FAT cluster 0 */
  Buffer[0] = 0xf8; /* Media type */
  Buffer[1] = 0xff;
  Buffer[2] = 0xff;
  Buffer[3] = 0x0f;
  /* FAT cluster 1 */
  Buffer[4] = 0xff; /* Clean shutdown, no disk read/write errors, end-of-cluster (EOC) mark */
  Buffer[5] = 0xff;
  Buffer[6] = 0xff;
  Buffer[7] = 0x0f;

  /* Write first sector of the FAT */
  FileOffset.QuadPart = (SectorOffset * 512) + sizeof(FATX_BOOT_SECTOR);
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    Buffer,
    512,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
      return(Status);
    }

  VfatxUpdateProgress (Context, 1);

  /* Zero the begin of the buffer */
  memset(Buffer, 0, 8);

  /* Zero the rest of the FAT */
  Sectors = 64 * 1024 / 512;
  for (i = 1; i < FATSectors; i += Sectors)
    {
      /* Zero some sectors of the FAT */
      FileOffset.QuadPart = (SectorOffset + i) * 512 + sizeof(FATX_BOOT_SECTOR);

      if ((FATSectors - i) <= Sectors)
        {
          Sectors = FATSectors - i;
        }

      Status = NtWriteFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Buffer,
        Sectors * 512,
        &FileOffset,
        NULL);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
          RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
          return(Status);
        }

      VfatxUpdateProgress (Context, Sectors);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}

static NTSTATUS
FatxWriteRootDirectory (IN HANDLE FileHandle,
			IN ULONG FATSectors,
			IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status = STATUS_SUCCESS;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG FirstRootDirSector;
  ULONG RootDirSectors;

  /* Write cluster */
  RootDirSectors = 256 * 64 / 512;
  FirstRootDirSector = sizeof(FATX_BOOT_SECTOR) / 512 + FATSectors;

  DPRINT("RootDirSectors = %lu\n", RootDirSectors);
  DPRINT("FirstRootDirSector = %lu\n", FirstRootDirSector);

  /* Allocate buffer for the cluster */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    RootDirSectors * 512);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0xff, RootDirSectors * 512);

  /* Zero some sectors of the root directory */
  FileOffset.QuadPart = FirstRootDirSector * 512;

  Status = NtWriteFile(FileHandle,
	     NULL,
	     NULL,
	     NULL,
	     &IoStatusBlock,
	     Buffer,
	     RootDirSectors * 512,
	     &FileOffset,
	     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}


NTSTATUS
FatxFormat (HANDLE FileHandle,
	    PPARTITION_INFORMATION PartitionInfo,
	    PDISK_GEOMETRY DiskGeometry,
	    BOOLEAN QuickFormat,
	    PFORMAT_CONTEXT Context)
{
  FATX_BOOT_SECTOR BootSector;
  ULONGLONG SectorCount;
  ULONG ClusterCount;
  ULONG RootDirSectors;
  ULONG FATSectors;

  NTSTATUS Status;

  SectorCount = PartitionInfo->PartitionLength.QuadPart >> GetShiftCount(512); /* Use shifting to avoid 64-bit division */
  
  memset(&BootSector, 0, sizeof(FATX_BOOT_SECTOR));
  memcpy(&BootSector.SysType[0], "FATX", 4);
  BootSector.SectorsPerCluster = 32;
  BootSector.FATCount = 1;
  BootSector.VolumeID = CalcVolumeSerialNumber();
  RootDirSectors = 256 * 64 / 512;

  /* Calculate number of FAT sectors */
  ClusterCount = SectorCount >> GetShiftCount(32);

  if (ClusterCount > 65525)
  {
     FATSectors = (((ClusterCount * 4) + 4095) & ~4095) >> GetShiftCount(512);
  }
  else
  {
     FATSectors = (((ClusterCount * 2) + 4095) & ~4095) >> GetShiftCount(512);
  }
  DPRINT("FATSectors = %hu\n", FATSectors);

  /* Init context data */
  if (QuickFormat)
    {
      Context->TotalSectorCount =
	1 + FATSectors + RootDirSectors;
    }
  else
    {
      Context->TotalSectorCount = SectorCount;
    }

  Status = FatxWriteBootSector (FileHandle,
				&BootSector,
				Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("FatxWriteBootSector() failed with status 0x%.08x\n", Status);
      return Status;
    }

  /* Write first FAT copy */
  if (ClusterCount > 65525)
  {
     Status = Fatx32WriteFAT (FileHandle,
			      0,
			      FATSectors,
			      Context);
  }
  else
  {
     Status = Fatx16WriteFAT (FileHandle,
			      0,
			      FATSectors,
			      Context);
  }
  if (!NT_SUCCESS(Status))
    {
      DPRINT("FatxWriteFAT() failed with status 0x%.08x\n", Status);
      return Status;
    }

  Status = FatxWriteRootDirectory (FileHandle,
				   FATSectors,
				   Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("FatxWriteRootDirectory() failed with status 0x%.08x\n", Status);
    }

  if (!QuickFormat)
    {
      /* FIXME: Fill remaining sectors */
    }

  return Status;
}
