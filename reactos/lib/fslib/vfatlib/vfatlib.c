/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatlib.c
 * PURPOSE:     Main API
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 05/04-2003 Created
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <ddk/ntddscsi.h>
#include <fslib/vfatlib.h>
#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


NTSTATUS
VfatInitialize(VOID)
{
  DPRINT("VfatInitialize()\n");

  return STATUS_SUCCESS;
}


NTSTATUS
VfatFormat (PUNICODE_STRING DriveRoot,
	    ULONG MediaFlag,
	    PUNICODE_STRING Label,
	    BOOLEAN QuickFormat,
	    ULONG ClusterSize,
	    PFMIFSCALLBACK Callback)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  DISK_GEOMETRY DiskGeometry;
  IO_STATUS_BLOCK Iosb;
  HANDLE FileHandle;
  PARTITION_INFORMATION PartitionInfo;
  FORMAT_CONTEXT Context;
  NTSTATUS Status;

  DPRINT("VfatFormat(DriveRoot '%wZ')\n", DriveRoot);

  Context.TotalSectorCount = 0;
  Context.CurrentSectorCount = 0;
  Context.Callback = Callback;
  Context.Success = FALSE;
  Context.Percent = 0;

  InitializeObjectAttributes(&ObjectAttributes,
    DriveRoot,
    0,
    NULL,
    NULL);

  Status = NtOpenFile(&FileHandle,
    FILE_WRITE_ACCESS | FILE_WRITE_ATTRIBUTES,
    &ObjectAttributes,
    &Iosb,
    FILE_SHARE_READ,
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
      DPRINT("Cylinders %I64d\n", DiskGeometry.Cylinders.QuadPart);
      DPRINT("TracksPerCylinder %ld\n", DiskGeometry.TracksPerCylinder);
      DPRINT("SectorsPerTrack %ld\n", DiskGeometry.SectorsPerTrack);
      DPRINT("BytesPerSector %ld\n", DiskGeometry.BytesPerSector);
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

      /*
       * FIXME: This is a hack!
       *        Partitioning software MUST set the correct number of hidden sectors!
       */
      PartitionInfo.HiddenSectors = DiskGeometry.SectorsPerTrack;
    }
  else
    {
      PartitionInfo.PartitionType = 0;
      PartitionInfo.StartingOffset.QuadPart = 0ULL;
      PartitionInfo.PartitionLength.QuadPart =
	DiskGeometry.Cylinders.QuadPart *
	(ULONGLONG)DiskGeometry.TracksPerCylinder *
	(ULONGLONG)DiskGeometry.SectorsPerTrack *
	(ULONGLONG)DiskGeometry.BytesPerSector;
      PartitionInfo.HiddenSectors = 0;
      PartitionInfo.PartitionNumber = 0;
      PartitionInfo.BootIndicator = FALSE;
      PartitionInfo.RewritePartition = FALSE;
      PartitionInfo.RecognizedPartition = FALSE;
    }

  DPRINT("PartitionType 0x%x\n", PartitionInfo.PartitionType);
  DPRINT("StartingOffset %I64d\n", PartitionInfo.StartingOffset.QuadPart);
  DPRINT("PartitionLength %I64d\n", PartitionInfo.PartitionLength.QuadPart);
  DPRINT("HiddenSectors %lu\n", PartitionInfo.HiddenSectors);
  DPRINT("PartitionNumber %d\n", PartitionInfo.PartitionNumber);
  DPRINT("BootIndicator 0x%x\n", PartitionInfo.BootIndicator);
  DPRINT("RewritePartition %d\n", PartitionInfo.RewritePartition);
  DPRINT("RecognizedPartition %d\n", PartitionInfo.RecognizedPartition);

  if (Callback != NULL)
    {
      Context.Percent = 0;
      Callback (PROGRESS, 0, (PVOID)&Context.Percent);
    }

  if (PartitionInfo.PartitionLength.QuadPart < (4200ULL * 1024ULL))
    {
      /* FAT12 (volume is smaller than 4.1MB) */
      Status = Fat12Format (FileHandle,
			    &PartitionInfo,
			    &DiskGeometry,
			    Label,
			    QuickFormat,
			    ClusterSize,
			    &Context);
    }
  else if (PartitionInfo.PartitionLength.QuadPart < (512ULL * 1024ULL * 1024ULL))
    {
      /* FAT16 (volume is smaller than 512MB) */
      Status = Fat16Format (FileHandle,
			    &PartitionInfo,
			    &DiskGeometry,
			    Label,
			    QuickFormat,
			    ClusterSize,
			    &Context);
    }
  else
    {
      /* FAT32 (volume is 512MB or larger) */
      Status = Fat32Format (FileHandle,
			    &PartitionInfo,
			    &DiskGeometry,
			    Label,
			    QuickFormat,
			    ClusterSize,
			    &Context);
    }

  NtClose(FileHandle);

  if (Callback != NULL)
    {
      Context.Success = (BOOLEAN)(NT_SUCCESS(Status));
      Callback (DONE, 0, (PVOID)&Context.Success);
    }

  DPRINT("VfatFormat() done. Status 0x%.08x\n", Status);

  return Status;
}


NTSTATUS
VfatCleanup(VOID)
{
  DPRINT("VfatCleanup()\n");

  return STATUS_SUCCESS;
}


VOID
UpdateProgress (PFORMAT_CONTEXT Context,
		ULONG Increment)
{
  ULONG NewPercent;

  Context->CurrentSectorCount += (ULONGLONG)Increment;


  NewPercent = (Context->CurrentSectorCount * 100ULL) / Context->TotalSectorCount;

  if (NewPercent > Context->Percent)
    {
      Context->Percent = NewPercent;
      Context->Callback (PROGRESS, 0, &Context->Percent);
    }
}

/* EOF */
