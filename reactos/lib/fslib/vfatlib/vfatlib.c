/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatlib.c
 * PURPOSE:     Main API
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 05/04-2003 Created
 */
#define NDEBUG
#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <ddk/ntddscsi.h>
#include <fslib/vfatlib.h>
#include "vfatlib.h"


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


NTSTATUS
VfatInitialize()
{
  DPRINT("VfatInitialize()\n");

  return STATUS_SUCCESS;
}

static NTSTATUS
VfatWriteBootSector(IN HANDLE FileHandle,
  IN PFAT32_BOOT_SECTOR BootSector)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  NTSTATUS Status;
  PUCHAR NewBootSector;
  LARGE_INTEGER FileOffset;

  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    SECTORSIZE);
  if (NewBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the new bootsector */
  memset(NewBootSector, 0, SECTORSIZE);

  /* Copy FAT32 BPB to new bootsector */
  memcpy((NewBootSector + 3),
    &BootSector->OEMName[0],
    87); /* FAT32 BPB length (up to (not including) Res2) */

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

  /* Write backup boot sector */
  if (BootSector->BootBackup != 0x0000)
    {
      FileOffset.QuadPart = (ULONGLONG)((ULONG) BootSector->BootBackup * SECTORSIZE);
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
    }

  /* Free the new boot sector */
  RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);

  return(Status);
}


static NTSTATUS
VfatWriteFsInfo(IN HANDLE FileHandle,
  IN PFAT32_BOOT_SECTOR BootSector)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  NTSTATUS Status;
  PFAT32_FSINFO FsInfo;
  LARGE_INTEGER FileOffset;

  /* Allocate buffer for new sector */
  FsInfo = (PFAT32_FSINFO)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    BootSector->BytesPerSector);
  if (FsInfo == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the new sector */
  memset(FsInfo, 0, BootSector->BytesPerSector);

  FsInfo->LeadSig = 0x41615252;
  FsInfo->StrucSig = 0x61417272;
  FsInfo->FreeCount = 0xffffffff;
  FsInfo->NextFree = 0xffffffff;

  /* Write sector */
  FileOffset.QuadPart = BootSector->FSInfoSector * BootSector->BytesPerSector;
  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    FsInfo,
    BootSector->BytesPerSector,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, FsInfo);
      return(Status);
    }

  /* Free the new sector buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, FsInfo);

  return(Status);
}


static NTSTATUS
VfatWriteFAT(IN HANDLE FileHandle,
  ULONG SectorOffset,
  IN PFAT32_BOOT_SECTOR BootSector)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  NTSTATUS Status;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONG i;

  /* Allocate buffer */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    BootSector->BytesPerSector);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, BootSector->BytesPerSector);

  /* FAT cluster 0 */
  Buffer[0] = 0xf8; /* Media type */
  Buffer[1] = 0xff;
  Buffer[2] = 0xff;
  Buffer[3] = 0xff;
  /* FAT cluster 1 */
  Buffer[4] = 0xcf; /* Clean shutdown, no disk read/write errors, end-of-cluster (EOC) mark */
  Buffer[5] = 0xff;
  Buffer[6] = 0xff;
  Buffer[7] = 0xf7;
  /* FAT cluster 2 */
  Buffer[8] = 0xff; /* End of root directory */
  Buffer[9] = 0xff;
  Buffer[10] = 0xff;
  Buffer[11] = 0x0f;

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

  /* Zero the buffer */
  memset(Buffer, 0, BootSector->BytesPerSector);

  /* Zero the rest of the FAT */
  for (i = 1; i < BootSector->FATSectors32 - 1; i++)
    {
      /* Zero one sector of the FAT */
      FileOffset.QuadPart = (SectorOffset + BootSector->ReservedSectors + i) * BootSector->BytesPerSector;
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
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}


static NTSTATUS
VfatWriteRootDirectory(IN HANDLE FileHandle,
  IN PFAT32_BOOT_SECTOR BootSector)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PUCHAR Buffer;
  LARGE_INTEGER FileOffset;
  ULONGLONG FirstDataSector;
  ULONGLONG FirstRootDirSector;

  /* Allocate buffer for the cluster */
  Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
    0,
    BootSector->SectorsPerCluster * BootSector->BytesPerSector);
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Zero the buffer */
  memset(Buffer, 0, BootSector->SectorsPerCluster * BootSector->BytesPerSector);

  DPRINT("BootSector->ReservedSectors = %lu\n", BootSector->ReservedSectors);
  DPRINT("BootSector->FATSectors32 = %lu\n", BootSector->FATSectors32);
  DPRINT("BootSector->RootCluster = %lu\n", BootSector->RootCluster);
  DPRINT("BootSector->SectorsPerCluster = %lu\n", BootSector->SectorsPerCluster);

  /* Write cluster */
  FirstDataSector = BootSector->ReservedSectors +
    (BootSector->FATCount * BootSector->FATSectors32) + 0 /* RootDirSectors */;

  DPRINT("FirstDataSector = %lu\n", FirstDataSector);

  FirstRootDirSector = ((BootSector->RootCluster - 2) * BootSector->SectorsPerCluster) + FirstDataSector;
  FileOffset.QuadPart = FirstRootDirSector * BootSector->BytesPerSector;

  DPRINT("FirstRootDirSector = %lu\n", FirstRootDirSector);
  DPRINT("FileOffset = %lu\n", FileOffset.QuadPart);

  Status = NtWriteFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &IoStatusBlock,
    Buffer,
    BootSector->SectorsPerCluster * BootSector->BytesPerSector,
    &FileOffset,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
      return(Status);
    }

  /* Free the buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

  return(Status);
}


NTSTATUS
VfatFormat(
	PUNICODE_STRING  DriveRoot,
	DWORD  MediaFlag,
	PUNICODE_STRING  Label,
	BOOL  QuickFormat,
	DWORD  ClusterSize,
	PFMIFSCALLBACK  Callback)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  DISK_GEOMETRY DiskGeometry;
  IO_STATUS_BLOCK Iosb;
  HANDLE FileHandle;
  SCSI_ADDRESS ScsiAddress;
  PARTITION_INFORMATION PartitionInfo;
  FAT32_BOOT_SECTOR BootSector;
  ANSI_STRING VolumeLabel;
  NTSTATUS Status;
  ULONG RootDirSectors;
  ULONG TmpVal1;
  ULONG TmpVal2;

  DPRINT("VfatFormat(DriveRoot '%S')\n", DriveRoot->Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
    DriveRoot,
    0,
    NULL,
    NULL);

  Status = NtOpenFile(&FileHandle,
    FILE_WRITE_ACCESS | FILE_WRITE_ATTRIBUTES,
    &ObjectAttributes,
    &Iosb,
    1,
    FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed with status 0x%.08x\n", Status);
      return Status;
    }

  Status = NtDeviceIoControlFile(FileHandle,
    NULL,
    NULL,
    NULL,
    &Iosb,
    IOCTL_DISK_GET_DRIVE_GEOMETRY,
    NULL,
    0,
    &DiskGeometry,
    sizeof(DISK_GEOMETRY));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("IOCTL_DISK_GET_DRIVE_GEOMETRY failed with status 0x%.08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  if (DiskGeometry.MediaType == FixedMedia)
    {
      DPRINT("Cylinders %d\n", DiskGeometry.Cylinders.QuadPart);
      DPRINT("TracksPerCylinder %d\n", DiskGeometry.TracksPerCylinder);
      DPRINT("SectorsPerTrack %d\n", DiskGeometry.SectorsPerTrack);
      DPRINT("BytesPerSector %d\n", DiskGeometry.BytesPerSector);
      DPRINT("DiskSize %d\n",
        DiskGeometry.Cylinders.QuadPart *
        (ULONGLONG)DiskGeometry.TracksPerCylinder *
        (ULONGLONG)DiskGeometry.SectorsPerTrack *
        (ULONGLONG)DiskGeometry.BytesPerSector);

      Status = NtDeviceIoControlFile(FileHandle,
        NULL,
        NULL,
        NULL,
        &Iosb,
        IOCTL_DISK_GET_PARTITION_INFO,
        NULL,
        0,
        &PartitionInfo,
        sizeof(PARTITION_INFORMATION));
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IOCTL_DISK_GET_PARTITION_INFO failed with status 0x%.08x\n", Status);
          NtClose(FileHandle);
          return Status;
        }

	    DPRINT("PartitionType 0x%x\n", PartitionInfo.PartitionType);
	    DPRINT("StartingOffset %d\n", PartitionInfo.StartingOffset);
	    DPRINT("PartitionLength %d\n", PartitionInfo.PartitionLength);
	    DPRINT("HiddenSectors %d\n", PartitionInfo.HiddenSectors);
	    DPRINT("PartitionNumber %d\n", PartitionInfo.PartitionNumber);
	    DPRINT("BootIndicator 0x%x\n", PartitionInfo.BootIndicator);
	    DPRINT("RewritePartition %d\n", PartitionInfo.RewritePartition);
	    DPRINT("RecognizedPartition %d\n", PartitionInfo.RecognizedPartition);
    }
  else
    {
      DPRINT1("FIXME: Removable media is not supported\n");
      NtClose(FileHandle);
      return Status;
    }

  memset(&BootSector, 0, sizeof(FAT32_BOOT_SECTOR));
  memcpy(&BootSector.OEMName[0], "MSWIN4.1", 8);
  BootSector.BytesPerSector = DiskGeometry.BytesPerSector;
  BootSector.SectorsPerCluster = 4096 / BootSector.BytesPerSector;  // 4KB clusters /* FIXME: */
  BootSector.ReservedSectors = 32;
  BootSector.FATCount = 2;
  BootSector.RootEntries = 0;
  BootSector.Sectors = 0;
  BootSector.Media = 0xf8;
  BootSector.FATSectors = 0;
  BootSector.SectorsPerTrack = DiskGeometry.SectorsPerTrack;
  BootSector.Heads = DiskGeometry.TracksPerCylinder;
  BootSector.HiddenSectors = PartitionInfo.HiddenSectors;
  BootSector.SectorsHuge = PartitionInfo.PartitionLength.QuadPart >>
    GetShiftCount(BootSector.BytesPerSector); /* Use shifting to avoid 64-bit division */
  BootSector.FATSectors32 = 0; /* Set later */
  BootSector.ExtFlag = 0; /* Mirror all FATs */
  BootSector.FSVersion = 0x0000; /* 0:0 */
  BootSector.RootCluster = 2;
  BootSector.FSInfoSector = 1;
  BootSector.BootBackup = 6;
  BootSector.Drive = 0xff; /* No BIOS boot drive available */
  BootSector.ExtBootSignature = 0x29;
  BootSector.VolumeID = 0x45768798; /* FIXME: */
  if ((Label == NULL) || (Label->Buffer == NULL))
    {
      memcpy(&BootSector.VolumeLabel[0], "NO NAME    ", 11);
    }
  else
    {
      RtlUnicodeStringToAnsiString(&VolumeLabel, Label, TRUE);
      memset(&BootSector.VolumeLabel[0], ' ', 11);
      memcpy(&BootSector.VolumeLabel[0], VolumeLabel.Buffer,
        VolumeLabel.Length < 11 ? VolumeLabel.Length : 11);
      RtlFreeAnsiString(&VolumeLabel);
    }
  memcpy(&BootSector.SysType[0], "FAT32   ", 8);

  RootDirSectors = ((BootSector.RootEntries * 32) +
    (BootSector.BytesPerSector - 1)) / BootSector.BytesPerSector;
  TmpVal1 = BootSector.SectorsHuge - (BootSector.ReservedSectors + RootDirSectors);
  TmpVal2 = (256 * BootSector.SectorsPerCluster) + BootSector.FATCount;
  if (TRUE /* FAT32 */)
    {
      TmpVal2 /= 2;
      BootSector.FATSectors = 0;
      BootSector.FATSectors32 = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
    }

  Status = VfatWriteBootSector(FileHandle,
    &BootSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("VfatWriteBootSector() failed with status 0x%.08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  Status = VfatWriteFsInfo(FileHandle,
    &BootSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("VfatWriteFsInfo() failed with status 0x%.08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  /* Write first FAT copy */
  Status = VfatWriteFAT(FileHandle,
    0,
    &BootSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("VfatWriteFAT() failed with status 0x%.08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  /* Write second FAT copy */
  Status = VfatWriteFAT(FileHandle,
    BootSector.FATSectors32,
    &BootSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("VfatWriteFAT() failed with status 0x%.08x.\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  Status = VfatWriteRootDirectory(FileHandle,
    &BootSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("VfatWriteRootDirectory() failed with status 0x%.08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }

  NtClose(FileHandle);

  DPRINT("VfatFormat() done. Status 0x%.08x\n", Status);

  return Status;
}


NTSTATUS
VfatCleanup()
{
  DPRINT("VfatCleanup()\n");

  return STATUS_SUCCESS;
}
