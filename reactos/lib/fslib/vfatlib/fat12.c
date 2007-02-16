/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        fat12.c
 * PURPOSE:     Fat12 support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl (ekohl@rz-online.de)
 * REVISIONS:
 *   EK 05/04-2003 Created
 */
#include "vfatlib.h"

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
Fat12WriteBootSector (IN HANDLE FileHandle,
		      IN PFAT16_BOOT_SECTOR BootSector,
		      IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR NewBootSector;
  LARGE_INTEGER FileOffset;

  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap (RtlGetProcessHeap (),
					   0,
					   SECTORSIZE);
  if (NewBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the new bootsector */
  memset(NewBootSector, 0, SECTORSIZE);

  /* Copy FAT16 BPB to new bootsector */
  memcpy((NewBootSector + 3),
    &BootSector->OEMName[0],
    59); /* FAT16 BPB length (up to (not including) Res2) */

  /* Write sector 0 */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    NewBootSector,
    SECTORSIZE,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);
      return(Status);
    }

  /* Free the new boot sector */
  RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);

  UpdateProgress (Context, 1);

  return(Status);
}


static NTSTATUS
Fat12WriteFAT (IN HANDLE FileHandle,
	       IN ULONG SectorOffset,
	       IN PFAT16_BOOT_SECTOR BootSector,
	       IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG i;
  ULONG Size;
  ULONG Sectors;

  /* Allocate buffer */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    32 * 1024);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, 32 * 1024);

  /* FAT cluster 0 & 1*/
  Buffer[0] = 0xf8; /* Media type */
  Buffer[1] = 0xff;
  Buffer[2] = 0xff;

  /* Write first sector of the FAT */
  FileOffset.QuadPart = (SectorOffset + BootSector->ReservedSectors) * BootSector->BytesPerSector;
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    Buffer,
    BootSector->BytesPerSector,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
      return(Status);
    }

  UpdateProgress (Context, 1);

  /* Zero the begin of the buffer */
  memset(Buffer, 0, 3);

  /* Zero the rest of the FAT */
  Sectors = 32 * 1024 / BootSector->BytesPerSector;
  for (i = 1; i < (ULONG)BootSector->FATSectors; i += Sectors)
    {
      /* Zero some sectors of the FAT */
      FileOffset.QuadPart = (SectorOffset + BootSector->ReservedSectors + i) * BootSector->BytesPerSector;
      if (((ULONG)BootSector->FATSectors - i) <= Sectors)
        {
	  Sectors = (ULONG)BootSector->FATSectors - i;
        }

      Size = Sectors * BootSector->BytesPerSector;
      Status = NtWriteFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Buffer,
	Size,
        &FileOffset,
        NULL);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
          RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
          return(Status);
        }

      UpdateProgress (Context, Sectors);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}


static NTSTATUS
Fat12WriteRootDirectory (IN HANDLE FileHandle,
			 IN PFAT16_BOOT_SECTOR BootSector,
			 IN OUT PFORMAT_CONTEXT Context)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status = STATUS_SUCCESS;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG FirstRootDirSector;
  ULONG RootDirSectors;
  ULONG Sectors;
  ULONG Size;
  ULONG i;

  DPRINT("BootSector->ReservedSectors = %hu\n", BootSector->ReservedSectors);
  DPRINT("BootSector->FATSectors = %hu\n", BootSector->FATSectors);
  DPRINT("BootSector->SectorsPerCluster = %u\n", BootSector->SectorsPerCluster);

  /* Write cluster */
  RootDirSectors = ((BootSector->RootEntries * 32) +
    (BootSector->BytesPerSector - 1)) / BootSector->BytesPerSector;
  FirstRootDirSector =
    BootSector->ReservedSectors + (BootSector->FATCount * BootSector->FATSectors);

  DPRINT("RootDirSectors = %lu\n", RootDirSectors);
  DPRINT("FirstRootDirSector = %lu\n", FirstRootDirSector);

  /* Allocate buffer for the cluster */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    32 * 1024);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, 32 * 1024);

  Sectors = 32 * 1024 / BootSector->BytesPerSector;
  for (i = 0; i < RootDirSectors; i += Sectors)
    {
      /* Zero some sectors of the root directory */
      FileOffset.QuadPart = (FirstRootDirSector + i) * BootSector->BytesPerSector;

      if ((RootDirSectors - i) <= Sectors)
        {
	  Sectors = RootDirSectors - i;
        }
      Size = Sectors * BootSector->BytesPerSector;

      Status = NtWriteFile(FileHandle,
	NULL,
	NULL,
	NULL,
	&IoStatusBlock,
	Buffer,
	Size,
	&FileOffset,
	NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
	  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
	  return(Status);
	}
      UpdateProgress (Context, Sectors);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}


NTSTATUS
Fat12Format (HANDLE FileHandle,
	     PPARTITION_INFORMATION PartitionInfo,
	     PDISK_GEOMETRY DiskGeometry,
	     PUNICODE_STRING Label,
	     BOOLEAN QuickFormat,
	     ULONG ClusterSize,
	     PFORMAT_CONTEXT Context)
{
  FAT16_BOOT_SECTOR BootSector;
  OEM_STRING VolumeLabel;
  ULONG SectorCount;
  ULONG RootDirSectors;
  ULONG TmpVal1;
  ULONG TmpVal2;
  ULONG TmpVal3;
  NTSTATUS Status;

  /* Calculate cluster size */
  if (ClusterSize == 0)
    {
      /* 4KB Cluster (Harddisk only) */
      ClusterSize = 4096;
    }

  SectorCount = PartitionInfo->PartitionLength.QuadPart >>
    GetShiftCount(DiskGeometry->BytesPerSector); /* Use shifting to avoid 64-bit division */

  DPRINT("SectorCount = %lu\n", SectorCount);

  memset(&BootSector, 0, sizeof(FAT16_BOOT_SECTOR));
  memcpy(&BootSector.OEMName[0], "MSWIN4.1", 8);
  BootSector.BytesPerSector = DiskGeometry->BytesPerSector;
  BootSector.SectorsPerCluster = ClusterSize / BootSector.BytesPerSector;
  BootSector.ReservedSectors = 1;
  BootSector.FATCount = 2;
  BootSector.RootEntries = 512;
  BootSector.Sectors = (SectorCount < 0x10000) ? (unsigned short)SectorCount : 0;
  BootSector.Media = 0xf8;
  BootSector.FATSectors = 0;  /* Set later. See below. */
  BootSector.SectorsPerTrack = DiskGeometry->SectorsPerTrack;
  BootSector.Heads = DiskGeometry->TracksPerCylinder;
  BootSector.HiddenSectors = PartitionInfo->HiddenSectors;
  BootSector.SectorsHuge = (SectorCount >= 0x10000) ? (unsigned long)SectorCount : 0;
  BootSector.Drive = 0xff; /* No BIOS boot drive available */
  BootSector.ExtBootSignature = 0x29;
  BootSector.VolumeID = CalcVolumeSerialNumber();
  if ((Label == NULL) || (Label->Buffer == NULL))
    {
      memcpy(&BootSector.VolumeLabel[0], "NO NAME    ", 11);
    }
  else
    {
      RtlUnicodeStringToOemString(&VolumeLabel, Label, TRUE);
      memset(&BootSector.VolumeLabel[0], ' ', 11);
      memcpy(&BootSector.VolumeLabel[0], VolumeLabel.Buffer,
        VolumeLabel.Length < 11 ? VolumeLabel.Length : 11);
      RtlFreeOemString(&VolumeLabel);
    }
  memcpy(&BootSector.SysType[0], "FAT12   ", 8);

  RootDirSectors = ((BootSector.RootEntries * 32) +
    (BootSector.BytesPerSector - 1)) / BootSector.BytesPerSector;

  /* Calculate number of FAT sectors */
  /* ((BootSector.BytesPerSector * 2) / 3) FAT entries (12bit) fit into one sector */
  TmpVal1 = SectorCount - (BootSector.ReservedSectors + RootDirSectors);
  TmpVal2 = (((BootSector.BytesPerSector * 2) / 3) * BootSector.SectorsPerCluster) + BootSector.FATCount;
  TmpVal3 = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
  BootSector.FATSectors = (unsigned short)(TmpVal3 & 0xffff);

  DPRINT("BootSector.FATSectors = %hx\n", BootSector.FATSectors);

  /* Init context data */
  Context->TotalSectorCount =
    1 + (BootSector.FATSectors * 2) + RootDirSectors;

  Status = Fat12WriteBootSector (FileHandle,
				 &BootSector,
				 Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Fat12WriteBootSector() failed with status 0x%.08x\n", Status);
      return Status;
    }

  /* Write first FAT copy */
  Status = Fat12WriteFAT (FileHandle,
			  0,
			  &BootSector,
			  Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Fat12WriteFAT() failed with status 0x%.08x\n", Status);
      return Status;
    }

  /* Write second FAT copy */
  Status = Fat12WriteFAT (FileHandle,
			  (ULONG)BootSector.FATSectors,
			  &BootSector,
			  Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Fat12WriteFAT() failed with status 0x%.08x.\n", Status);
      return Status;
    }

  Status = Fat12WriteRootDirectory (FileHandle,
				    &BootSector,
				    Context);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Fat12WriteRootDirectory() failed with status 0x%.08x\n", Status);
    }

  if (!QuickFormat)
    {
      /* FIXME: Fill remaining sectors */
    }

  return Status;
}
