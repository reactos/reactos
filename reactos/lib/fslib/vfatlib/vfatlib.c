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


NTSTATUS
VfatInitialize(VOID)
{
  DPRINT("VfatInitialize()\n");

  return STATUS_SUCCESS;
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
  NTSTATUS Status;

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
      DPRINT("DiskSize %I64d\n",
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
      DPRINT("StartingOffset %I64d\n", PartitionInfo.StartingOffset);
      DPRINT("PartitionLength %I64d\n", PartitionInfo.PartitionLength);
      DPRINT("HiddenSectors %d\n", PartitionInfo.HiddenSectors);
      DPRINT("PartitionNumber %d\n", PartitionInfo.PartitionNumber);
      DPRINT("BootIndicator 0x%x\n", PartitionInfo.BootIndicator);
      DPRINT("RewritePartition %d\n", PartitionInfo.RewritePartition);
      DPRINT("RecognizedPartition %d\n", PartitionInfo.RecognizedPartition);


      if (PartitionInfo.PartitionType == PARTITION_FAT32_XINT13 ||
	  PartitionInfo.PartitionType == PARTITION_FAT32)
	{
	  Status = Fat32Format (FileHandle,
				&PartitionInfo,
				&DiskGeometry,
				Label,
				QuickFormat,
				ClusterSize,
				Callback);
	}
      else if (PartitionInfo.PartitionType == PARTITION_FAT_12)
	{
	  Status = Fat12Format (FileHandle,
				&PartitionInfo,
				&DiskGeometry,
				Label,
				QuickFormat,
				ClusterSize,
				Callback);
	}
      else
	{
	  Status = Fat16Format (FileHandle,
				&PartitionInfo,
				&DiskGeometry,
				Label,
				QuickFormat,
				ClusterSize,
				Callback);
	}
    }
  else
    {
      DPRINT1("FIXME: Removable media is not supported\n");
      NtClose(FileHandle);
      return STATUS_UNSUCCESSFUL;
    }

  NtClose(FileHandle);

  DPRINT("VfatFormat() done. Status 0x%.08x\n", Status);

  return Status;
}


NTSTATUS
VfatCleanup(VOID)
{
  DPRINT("VfatCleanup()\n");

  return STATUS_SUCCESS;
}
