/* $Id: xhaldrv.c,v 1.32 2003/05/11 18:31:09 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/xhaldrv.c
 * PURPOSE:         Hal drive routines
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 19/06/2000
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/xhal.h>

#define NDEBUG
#include <internal/debug.h>

/* LOCAL MACROS and TYPES ***************************************************/

#define  AUTO_DRIVE         ((ULONG)-1)

#define  PARTITION_MAGIC    0xaa55

#define  PARTITION_TBL_SIZE 4


typedef struct _PARTITION
{
  unsigned char   BootFlags;					/* bootable?  0=no, 128=yes  */
  unsigned char   StartingHead;					/* beginning head number */
  unsigned char   StartingSector;				/* beginning sector number */
  unsigned char   StartingCylinder;				/* 10 bit nmbr, with high 2 bits put in begsect */
  unsigned char   PartitionType;				/* Operating System type indicator code */
  unsigned char   EndingHead;					/* ending head number */
  unsigned char   EndingSector;					/* ending sector number */
  unsigned char   EndingCylinder;				/* also a 10 bit nmbr, with same high 2 bit trick */
  unsigned int  StartingBlock;					/* first sector relative to start of disk */
  unsigned int  SectorCount;					/* number of sectors in partition */
} PACKED PARTITION, *PPARTITION;


typedef struct _PARTITION_SECTOR
{
  UCHAR BootCode[440];				/* 0x000 */
  ULONG Signature;				/* 0x1B8 */
  UCHAR Reserved[2];				/* 0x1BC */
  PARTITION Partition[PARTITION_TBL_SIZE];	/* 0x1BE */
  USHORT Magic;					/* 0x1FE */
} PACKED PARTITION_SECTOR, *PPARTITION_SECTOR;


typedef enum _DISK_MANAGER
{
  NoDiskManager,
  OntrackDiskManager,
  EZ_Drive
} DISK_MANAGER;


/* FUNCTIONS *****************************************************************/

NTSTATUS
xHalQueryDriveLayout(IN PUNICODE_STRING DeviceName,
		     OUT PDRIVE_LAYOUT_INFORMATION *LayoutInfo)
{
  IO_STATUS_BLOCK StatusBlock;
  DISK_GEOMETRY DiskGeometry;
  PDEVICE_OBJECT DeviceObject = NULL;
  PFILE_OBJECT FileObject;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalpQueryDriveLayout %wZ %p\n",
	 DeviceName,
	 LayoutInfo);

  /* Get the drives sector size */
  Status = IoGetDeviceObjectPointer(DeviceName,
				    FILE_READ_DATA,
				    &FileObject,
				    &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Status %x\n",Status);
      return(Status);
    }

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
				      DeviceObject,
				      NULL,
				      0,
				      &DiskGeometry,
				      sizeof(DISK_GEOMETRY),
				      FALSE,
				      &Event,
				      &StatusBlock);
  if (Irp == NULL)
    {
      ObDereferenceObject(FileObject);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }
  if (!NT_SUCCESS(Status))
    {
      if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
	{
	  DiskGeometry.BytesPerSector = 512;
	}
      else
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
    }

  DPRINT("DiskGeometry.BytesPerSector: %d\n",
	 DiskGeometry.BytesPerSector);

  /* read the partition table */
  Status = IoReadPartitionTable(DeviceObject,
				DiskGeometry.BytesPerSector,
				FALSE,
				LayoutInfo);

  if ((!NT_SUCCESS(Status) || (*LayoutInfo)->PartitionCount == 0) &&
      DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
      PDRIVE_LAYOUT_INFORMATION Buffer;

      if (NT_SUCCESS(Status))
	{
	  ExFreePool(*LayoutInfo);
	}

      /* Allocate a partition list for a single entry. */
      Buffer = ExAllocatePool(NonPagedPool,
			      sizeof(DRIVE_LAYOUT_INFORMATION));
      if (Buffer != NULL)
	{
	  RtlZeroMemory(Buffer,
			sizeof(DRIVE_LAYOUT_INFORMATION));
	  Buffer->PartitionCount = 1;
	  *LayoutInfo = Buffer;

	  Status = STATUS_SUCCESS;
	}
    }

  ObDereferenceObject(FileObject);

  return(Status);
}


static NTSTATUS
xHalpReadSector(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG SectorSize,
		IN PLARGE_INTEGER SectorOffset,
		OUT PVOID *Buffer)
{
  KEVENT Event;
  IO_STATUS_BLOCK StatusBlock;
  PUCHAR Sector;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalReadMBR()\n");

  assert(DeviceObject);
  assert(Buffer);

  *Buffer = NULL;

  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  Sector = (PUCHAR)ExAllocatePool(PagedPool,
				  SectorSize);
  if (Sector == NULL)
    return STATUS_NO_MEMORY;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  /* Read MBR (Master Boot Record) */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     DeviceObject,
				     Sector,
				     SectorSize,
				     SectorOffset,
				     &Event,
				     &StatusBlock);

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Reading MBR failed (Status 0x%08lx)\n",
	     Status);
      ExFreePool(Sector);
      return Status;
    }

  *Buffer = (PVOID)Sector;
  return Status;
}


static NTSTATUS
xHalpWriteSector(IN PDEVICE_OBJECT DeviceObject,
		 IN ULONG SectorSize,
		 IN PLARGE_INTEGER SectorOffset,
		 OUT PVOID Sector)
{
  KEVENT Event;
  IO_STATUS_BLOCK StatusBlock;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalWriteMBR()\n");

  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  /* Write MBR (Master Boot Record) */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				     DeviceObject,
				     Sector,
				     SectorSize,
				     SectorOffset,
				     &Event,
				     &StatusBlock);

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Writing MBR failed (Status 0x%08lx)\n",
	     Status);
      return Status;
    }

  return Status;
}


VOID FASTCALL
xHalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
	       IN ULONG SectorSize,
	       IN ULONG MBRTypeIdentifier,
	       OUT PVOID *Buffer)
{
  LARGE_INTEGER SectorOffset;
  PPARTITION_SECTOR Sector;
  PULONG Shift;
  NTSTATUS Status;

  DPRINT("xHalExamineMBR()\n");

  *Buffer = NULL;

  SectorOffset.QuadPart = 0ULL;
  Status = xHalpReadSector(DeviceObject,
			   SectorSize,
			   &SectorOffset,
			   (PVOID *)&Sector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("xHalpReadSector() failed (Status %lx)\n", Status);
      return;
    }

  if (Sector->Magic != PARTITION_MAGIC)
    {
      DPRINT("Invalid MBR magic value\n");
      ExFreePool(Sector);
      return;
    }

  if (Sector->Partition[0].PartitionType != MBRTypeIdentifier)
    {
      DPRINT("Invalid MBRTypeIdentifier\n");
      ExFreePool(Sector);
      return;
    }

  if (Sector->Partition[0].PartitionType == 0x54)
    {
      /* Found 'Ontrack Disk Manager'. Shift all sectors by 63 */
      DPRINT("Found 'Ontrack Disk Manager'!\n");
      Shift = (PULONG)Sector;
      *Shift = 63;
    }

  *Buffer = (PVOID)Sector;
}


static VOID
HalpAssignDrive(IN PUNICODE_STRING PartitionName,
		IN ULONG DriveNumber,
		IN UCHAR DriveType)
{
  WCHAR DriveNameBuffer[8];
  UNICODE_STRING DriveName;
  ULONG i;

  DPRINT("HalpAssignDrive()\n");

  if ((DriveNumber != AUTO_DRIVE) && (DriveNumber < 24))
    {
      /* Force assignment */
      if ((SharedUserData->DosDeviceMap & (1 << DriveNumber)) != 0)
	{
	  DbgPrint("Drive letter already used!\n");
	  return;
	}
    }
  else
    {
      /* Automatic assignment */
      DriveNumber = AUTO_DRIVE;

      for (i = 2; i < 24; i++)
	{
	  if ((SharedUserData->DosDeviceMap & (1 << i)) == 0)
	    {
	      DriveNumber = i;
	      break;
	    }
	}

      if (DriveNumber == AUTO_DRIVE)
	{
	  DbgPrint("No drive letter available!\n");
	  return;
	}
    }

  DPRINT("DriveNumber %d\n", DriveNumber);

  /* Update the shared user page */
  SharedUserData->DosDeviceMap |= (1 << DriveNumber);
  SharedUserData->DosDeviceDriveType[DriveNumber] = DriveType;

  /* Build drive name */
  swprintf(DriveNameBuffer,
	   L"\\??\\%C:",
	   'A' + DriveNumber);
  RtlInitUnicodeString(&DriveName,
		       DriveNameBuffer);

  DPRINT("  %wZ ==> %wZ\n",
	 &DriveName,
	 PartitionName);

  /* Create symbolic link */
  IoCreateSymbolicLink(&DriveName,
		       PartitionName);
}


VOID FASTCALL
xHalIoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
			 IN PSTRING NtDeviceName,
			 OUT PUCHAR NtSystemPath,
			 OUT PSTRING NtSystemPathString)
{
  PDRIVE_LAYOUT_INFORMATION *LayoutArray;
  PCONFIGURATION_INFORMATION ConfigInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK StatusBlock;
  UNICODE_STRING UnicodeString1;
  UNICODE_STRING UnicodeString2;
  HANDLE FileHandle;
  PWSTR Buffer1;
  PWSTR Buffer2;
  ULONG i;
  NTSTATUS Status;
  ULONG j;

  DPRINT("xHalIoAssignDriveLetters()\n");

  ConfigInfo = IoGetConfigurationInformation();

  Buffer1 = (PWSTR)ExAllocatePool(PagedPool,
				  64 * sizeof(WCHAR));
  Buffer2 = (PWSTR)ExAllocatePool(PagedPool,
				  32 * sizeof(WCHAR));

  /* Create PhysicalDrive links */
  DPRINT("Physical disk drives: %d\n", ConfigInfo->DiskCount);
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Harddisk%d\\Partition0",
	        i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      InitializeObjectAttributes(&ObjectAttributes,
				 &UnicodeString1,
				 0,
				 NULL,
				 NULL);

      Status = NtOpenFile(&FileHandle,
			  0x10001,
			  &ObjectAttributes,
			  &StatusBlock,
			  1,
			  FILE_SYNCHRONOUS_IO_NONALERT);
      if (NT_SUCCESS(Status))
	{
	  NtClose(FileHandle);

	  swprintf(Buffer2,
		   L"\\??\\PhysicalDrive%d",
		   i);
	  RtlInitUnicodeString(&UnicodeString2,
			       Buffer2);

	  DPRINT("Creating link: %S ==> %S\n",
		 Buffer2,
		 Buffer1);

	  IoCreateSymbolicLink(&UnicodeString2,
			       &UnicodeString1);
	}
    }

  /* Initialize layout array */
  LayoutArray = ExAllocatePool(NonPagedPool,
			       ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
  RtlZeroMemory(LayoutArray,
		ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Harddisk%d\\Partition0",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      Status = xHalQueryDriveLayout(&UnicodeString1,
				    &LayoutArray[i]);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("xHalQueryDriveLayout() failed (Status = 0x%lx)\n",
		   Status);
	  LayoutArray[i] = NULL;
	  continue;
	}
    }

#ifndef NDEBUG
  /* Dump layout array */
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      DPRINT("Harddisk %d:\n",
	     i);

      if (LayoutArray[i] == NULL)
	continue;

      DPRINT("Logical partitions: %d\n",
	     LayoutArray[i]->PartitionCount);

      for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
	{
	  DPRINT("  %d: nr:%x boot:%x type:%x startblock:%I64u count:%I64u\n",
		 j,
		 LayoutArray[i]->PartitionEntry[j].PartitionNumber,
		 LayoutArray[i]->PartitionEntry[j].BootIndicator,
		 LayoutArray[i]->PartitionEntry[j].PartitionType,
		 LayoutArray[i]->PartitionEntry[j].StartingOffset.QuadPart,
		 LayoutArray[i]->PartitionEntry[j].PartitionLength.QuadPart);
	}
    }
#endif

  /* Assign pre-assigned (registry) partitions */


  /* Assign bootable partition on first harddisk */
  DPRINT("Assigning bootable primary partition on first harddisk:\n");
  if (ConfigInfo->DiskCount > 0)
    {
      /* Search for bootable partition */
      for (j = 0; j < LayoutArray[0]->PartitionCount; j++)
	{
	  if ((LayoutArray[0]->PartitionEntry[j].BootIndicator == TRUE) &&
	      IsRecognizedPartition(LayoutArray[0]->PartitionEntry[j].PartitionType))
	    {
	      swprintf(Buffer2,
		       L"\\Device\\Harddisk0\\Partition%d",
		       LayoutArray[0]->PartitionEntry[j].PartitionNumber);
	      RtlInitUnicodeString(&UnicodeString2,
				   Buffer2);

	      /* Assign drive */
	      DPRINT("  %wZ\n", &UnicodeString2);
	      HalpAssignDrive(&UnicodeString2,
			      AUTO_DRIVE,
			      DOSDEVICE_DRIVE_FIXED);
	    }
	}
    }

  /* Assign remaining  primary partitions */
  DPRINT("Assigning remaining primary partitions:\n");
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      /* Search for primary partitions */
      for (j = 0; (j < PARTITION_TBL_SIZE) && (j < LayoutArray[i]->PartitionCount); j++)
	{
	  if ((i == 0) && (LayoutArray[i]->PartitionEntry[j].BootIndicator == TRUE))
	    continue;

	  if (IsRecognizedPartition(LayoutArray[i]->PartitionEntry[j].PartitionType))
	    {
	      swprintf(Buffer2,
		       L"\\Device\\Harddisk%d\\Partition%d",
		       i,
		       LayoutArray[i]->PartitionEntry[j].PartitionNumber);
	      RtlInitUnicodeString(&UnicodeString2,
				   Buffer2);

	      /* Assign drive */
	      DPRINT("  %wZ\n",
		     &UnicodeString2);
	      HalpAssignDrive(&UnicodeString2,
			      AUTO_DRIVE,
			      DOSDEVICE_DRIVE_FIXED);
	    }
	}
    }

  /* Assign extended (logical) partitions */
  DPRINT("Assigning extended (logical) partitions:\n");
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      if (LayoutArray[i])
	{
	  /* Search for extended partitions */
	  for (j = PARTITION_TBL_SIZE; j < LayoutArray[i]->PartitionCount; j++)
	    {
	      if (IsRecognizedPartition(LayoutArray[i]->PartitionEntry[j].PartitionType) &&
		  (LayoutArray[i]->PartitionEntry[j].PartitionNumber != 0))
		{
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   i,
			   LayoutArray[i]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* Assign drive */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  AUTO_DRIVE,
				  DOSDEVICE_DRIVE_FIXED);
		}
	    }
	}
    }

  /* Assign removable disk drives */
  DPRINT("Assigning removable disk drives:\n");
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      /* Search for virtual partitions */
      if (LayoutArray[i]->PartitionCount == 1 &&
	  LayoutArray[i]->PartitionEntry[0].PartitionType == 0)
	{
	  swprintf(Buffer2,
		   L"\\Device\\Harddisk%d\\Partition1",
		   i);
	  RtlInitUnicodeString(&UnicodeString2,
			       Buffer2);

	  /* Assign drive */
	  DPRINT("  %wZ\n",
		 &UnicodeString2);
	  HalpAssignDrive(&UnicodeString2,
			  AUTO_DRIVE,
			  DOSDEVICE_DRIVE_REMOVABLE);
	}
    }

  /* Free layout array */
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      if (LayoutArray[i] != NULL)
	ExFreePool(LayoutArray[i]);
    }
  ExFreePool(LayoutArray);

  /* Assign floppy drives */
  DPRINT("Floppy drives: %d\n", ConfigInfo->FloppyCount);
  for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Floppy%d",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      /* Assign drive letters A: or B: or first free drive letter */
      DPRINT("  %wZ\n",
	     &UnicodeString1);
      HalpAssignDrive(&UnicodeString1,
		      (i < 2) ? i : AUTO_DRIVE,
		      DOSDEVICE_DRIVE_REMOVABLE);
    }

  /* Assign cdrom drives */
  DPRINT("CD-Rom drives: %d\n", ConfigInfo->CDRomCount);
  for (i = 0; i < ConfigInfo->CDRomCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\CdRom%d",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      /* Assign first free drive letter */
      DPRINT("  %wZ\n", &UnicodeString1);
      HalpAssignDrive(&UnicodeString1,
		      AUTO_DRIVE,
		      DOSDEVICE_DRIVE_CDROM);
    }

  /* Anything else ?? */

  ExFreePool(Buffer2);
  ExFreePool(Buffer1);
}


NTSTATUS FASTCALL
xHalIoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
			 ULONG SectorSize,
			 BOOLEAN ReturnRecognizedPartitions,
			 PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
  KEVENT Event;
  IO_STATUS_BLOCK StatusBlock;
  ULARGE_INTEGER PartitionOffset;
  ULARGE_INTEGER RealPartitionOffset;
  ULARGE_INTEGER nextPartitionOffset;
  ULARGE_INTEGER containerOffset;
  PIRP Irp;
  NTSTATUS Status;
  PPARTITION_SECTOR PartitionSector;
  PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
  ULONG i;
  ULONG Count = 0;
  ULONG Number = 1;
  BOOLEAN ExtendedFound = FALSE;
  PVOID MbrBuffer;
  DISK_MANAGER DiskManager = NoDiskManager;

  DPRINT("xHalIoReadPartitionTable(%p %lu %x %p)\n",
	 DeviceObject,
	 SectorSize,
	 ReturnRecognizedPartitions,
	 PartitionBuffer);

  *PartitionBuffer = NULL;

  /* Check for 'Ontrack Disk Manager' */
  xHalExamineMBR(DeviceObject,
		 SectorSize,
		 0x54,
		 &MbrBuffer);
  if (MbrBuffer != NULL)
    {
      DPRINT("Found 'Ontrack Disk Manager'\n");
      DiskManager = OntrackDiskManager;
      ExFreePool(MbrBuffer);
    }

  /* Check for 'EZ-Drive' */
  xHalExamineMBR(DeviceObject,
		 SectorSize,
		 0x55,
		 &MbrBuffer);
  if (MbrBuffer != NULL)
    {
      DPRINT("Found 'EZ-Drive'\n");
      DiskManager = EZ_Drive;
      ExFreePool(MbrBuffer);
    }

  PartitionSector = (PPARTITION_SECTOR)ExAllocatePool(PagedPool,
						      SectorSize);
  if (PartitionSector == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  LayoutBuffer = (PDRIVE_LAYOUT_INFORMATION)ExAllocatePool(NonPagedPool,
							   0x1000);
  if (LayoutBuffer == NULL)
    {
      ExFreePool(PartitionSector);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(LayoutBuffer,
		0x1000);

  PartitionOffset.QuadPart = (ULONGLONG)0;
  containerOffset.QuadPart = (ULONGLONG)0;

  do
    {
      DPRINT("PartitionOffset: %I64u\n", PartitionOffset.QuadPart / SectorSize);

      if (DiskManager == OntrackDiskManager)
	{
	  RealPartitionOffset.QuadPart = PartitionOffset.QuadPart + (ULONGLONG)(63 * SectorSize);
	}
      else
	{
	  RealPartitionOffset.QuadPart = PartitionOffset.QuadPart;
	}

      DPRINT("RealPartitionOffset: %I64u\n", RealPartitionOffset.QuadPart / SectorSize);

      KeInitializeEvent(&Event,
			NotificationEvent,
			FALSE);

      Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
					 DeviceObject,
					 PartitionSector, //SectorBuffer,
					 SectorSize,
					 (PLARGE_INTEGER)&RealPartitionOffset,
					 &Event,
					 &StatusBlock);
      Status = IoCallDriver(DeviceObject,
			    Irp);
      if (Status == STATUS_PENDING)
	{
	  KeWaitForSingleObject(&Event,
				Executive,
				KernelMode,
				FALSE,
				NULL);
	  Status = StatusBlock.Status;
	}

      if (!NT_SUCCESS(Status))
	{
	  DPRINT("Failed to read partition table sector (Status = 0x%08lx)\n",
		 Status);
	  ExFreePool(PartitionSector);
	  ExFreePool(LayoutBuffer);
	  return(Status);
	}

      /* check the boot sector id */
      DPRINT("Magic %x\n", PartitionSector->Magic);
      if (PartitionSector->Magic != PARTITION_MAGIC)
	{
	  DbgPrint("Invalid partition sector magic\n");
	  ExFreePool(PartitionSector);
	  *PartitionBuffer = LayoutBuffer;
	  return(STATUS_SUCCESS);
	}

#ifndef NDEBUG
      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  DPRINT1("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n",
		  i,
		  PartitionSector->Partition[i].BootFlags,
		  PartitionSector->Partition[i].PartitionType,
		  PartitionSector->Partition[i].StartingHead,
		  PartitionSector->Partition[i].StartingSector & 0x3f,
		  (((PartitionSector->Partition[i].StartingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].StartingCylinder,
		  PartitionSector->Partition[i].EndingHead,
		  PartitionSector->Partition[i].EndingSector & 0x3f,
		  (((PartitionSector->Partition[i].EndingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].EndingCylinder,
		  PartitionSector->Partition[i].StartingBlock,
		  PartitionSector->Partition[i].SectorCount);
	}
#endif

      if (PartitionOffset.QuadPart == 0ULL)
	{
	  LayoutBuffer->Signature = PartitionSector->Signature;
	  DPRINT("Disk signature: %lx\n", LayoutBuffer->Signature);
	}

      ExtendedFound = FALSE;

      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  if ((ReturnRecognizedPartitions == FALSE) ||
	       ((ReturnRecognizedPartitions == TRUE) &&
	        IsRecognizedPartition(PartitionSector->Partition[i].PartitionType)))
	    {
	      /* handle normal partition */
	      DPRINT("Partition %u: Normal Partition\n", i);
	      Count = LayoutBuffer->PartitionCount;
	      DPRINT("Logical Partition %u\n", Count);
	      if (PartitionSector->Partition[i].StartingBlock == 0)
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart = 0;
		}
	      else if (IsContainerPartition(PartitionSector->Partition[i].PartitionType))
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
		    (ULONGLONG)PartitionOffset.QuadPart;
		}
	      else
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
		    (ULONGLONG)PartitionOffset.QuadPart +
		    ((ULONGLONG)PartitionSector->Partition[i].StartingBlock * (ULONGLONG)SectorSize);
		}
	      LayoutBuffer->PartitionEntry[Count].PartitionLength.QuadPart =
		(ULONGLONG)PartitionSector->Partition[i].SectorCount * (ULONGLONG)SectorSize;
	      LayoutBuffer->PartitionEntry[Count].HiddenSectors = 0;

	      if (IsRecognizedPartition(PartitionSector->Partition[i].PartitionType))
		{
		  LayoutBuffer->PartitionEntry[Count].PartitionNumber = Number;
		  Number++;
		}
	      else
		{
		  LayoutBuffer->PartitionEntry[Count].PartitionNumber = 0;
		}

	      LayoutBuffer->PartitionEntry[Count].PartitionType =
		PartitionSector->Partition[i].PartitionType;
	      LayoutBuffer->PartitionEntry[Count].BootIndicator =
		(PartitionSector->Partition[i].BootFlags & 0x80)?TRUE:FALSE;
	      LayoutBuffer->PartitionEntry[Count].RecognizedPartition =
		IsRecognizedPartition (PartitionSector->Partition[i].PartitionType);
	      LayoutBuffer->PartitionEntry[Count].RewritePartition = FALSE;

	      DPRINT(" %ld: nr: %d boot: %1x type: %x start: 0x%I64x count: 0x%I64x\n",
		     Count,
		     LayoutBuffer->PartitionEntry[Count].PartitionNumber,
		     LayoutBuffer->PartitionEntry[Count].BootIndicator,
		     LayoutBuffer->PartitionEntry[Count].PartitionType,
		     LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart,
		     LayoutBuffer->PartitionEntry[Count].PartitionLength.QuadPart);

	      LayoutBuffer->PartitionCount++;
	    }

	  if (IsContainerPartition(PartitionSector->Partition[i].PartitionType))
	    {
	      ExtendedFound = TRUE;
	      if ((ULONGLONG) containerOffset.QuadPart == (ULONGLONG) 0)
		{
		  containerOffset = PartitionOffset;
		}
	      nextPartitionOffset.QuadPart = (ULONGLONG) containerOffset.QuadPart +
		(ULONGLONG) PartitionSector->Partition[i].StartingBlock *
		(ULONGLONG) SectorSize;
	    }
	}
      PartitionOffset = nextPartitionOffset;
    }
  while (ExtendedFound == TRUE);

  *PartitionBuffer = LayoutBuffer;
  ExFreePool(PartitionSector);

  return(STATUS_SUCCESS);
}


NTSTATUS FASTCALL
xHalIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
			      IN ULONG SectorSize,
			      IN ULONG PartitionNumber,
			      IN ULONG PartitionType)
{
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS FASTCALL
xHalIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG SectorSize,
			  IN ULONG SectorsPerTrack,
			  IN ULONG NumberOfHeads,
			  IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
  PPARTITION_SECTOR PartitionSector;
  LARGE_INTEGER SectorOffset;
  NTSTATUS Status;
  ULONG i;

  ULONG StartBlock;
  ULONG SectorCount;
  ULONG StartCylinder;
  ULONG StartSector;
  ULONG StartHead;
  ULONG EndCylinder;
  ULONG EndSector;
  ULONG EndHead;
  ULONG lba;
  ULONG x;

  DPRINT("xHalIoWritePartitionTable(%p %lu %lu %lu %p)\n",
	 DeviceObject,
	 SectorSize,
	 SectorsPerTrack,
   NumberOfHeads,
	 PartitionBuffer);

  assert(DeviceObject);
  assert(PartitionBuffer);

  /* Check for 'Ontrack Disk Manager' */
  xHalExamineMBR(DeviceObject,
		 SectorSize,
		 0x54,
		 (PVOID *) &PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT1("Ontrack Disk Manager is not supported\n");
      ExFreePool(PartitionSector);
      return STATUS_UNSUCCESSFUL;
    }

  /* Check for 'EZ-Drive' */
  xHalExamineMBR(DeviceObject,
		 SectorSize,
		 0x55,
		 (PVOID *) &PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT1("EZ-Drive is not supported\n");
      ExFreePool(PartitionSector);
      return STATUS_UNSUCCESSFUL;
    }


  for (i = 0; i < PartitionBuffer->PartitionCount; i++)
    {
      if (IsContainerPartition(PartitionBuffer->PartitionEntry[i].PartitionType))
        {
          /* FIXME: Implement */
          DPRINT1("Writing MBRs with extended partitions is not implemented\n");
          return STATUS_UNSUCCESSFUL;
        }
    }


  SectorOffset.QuadPart = 0ULL;
  Status = xHalpReadSector(DeviceObject,
			   SectorSize,
			   &SectorOffset,
			   (PVOID *) &PartitionSector);
    if (!NT_SUCCESS(Status))
    {
      DPRINT1("xHalpReadSector() failed (Status %lx)\n", Status);
      return Status;
    }

  for (i = 0; i < PartitionBuffer->PartitionCount; i++)
    {
      if (PartitionBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED)
        {
          /*
           * CHS formulas:
           * x = LBA DIV SectorsPerTrack
           * cylinder = x DIV NumberOfHeads
           * head = x MOD NumberOfHeads
           * sector = (LBA MOD SectorsPerTrack) + 1
           */

          if (PartitionBuffer->PartitionEntry[i].BootIndicator)
            {
              PartitionSector->Partition[i].BootFlags |= 0x80;
            }
          else
            {
              PartitionSector->Partition[i].BootFlags &= ~0x80;
            }
    
          PartitionSector->Partition[i].PartitionType = PartitionBuffer->PartitionEntry[i].PartitionType;

          /* Compute starting CHS values */
          lba = (PartitionBuffer->PartitionEntry[i].StartingOffset.QuadPart / SectorSize);
          StartBlock = lba; /* Save this for later */
          x = lba / SectorsPerTrack;
          StartCylinder = x / NumberOfHeads;
          StartHead = x % NumberOfHeads;
          StartSector = (lba % SectorsPerTrack) + 1;
          DPRINT("StartingOffset (LBA:%d  C:%d  H:%d  S:%d)\n", lba, StartCylinder, StartHead, StartSector);

          /* Compute ending CHS values */
          lba = (PartitionBuffer->PartitionEntry[i].StartingOffset.QuadPart
            + (PartitionBuffer->PartitionEntry[i].PartitionLength.QuadPart - 1)) / SectorSize;
          SectorCount = lba - StartBlock; /* Save this for later */
          x = lba / SectorsPerTrack;
          EndCylinder = x / NumberOfHeads;
          EndHead = x % NumberOfHeads;
          EndSector = (lba % SectorsPerTrack) + 1;
          DPRINT("EndingOffset (LBA:%d  C:%d  H:%d  S:%d)\n", lba, EndCylinder, EndHead, EndSector);

          /* Set startsector and sectorcount */
          PartitionSector->Partition[i].StartingBlock = StartBlock;
          PartitionSector->Partition[i].SectorCount = SectorCount;

          /* Set starting CHS values */
          PartitionSector->Partition[i].StartingCylinder = StartCylinder & 0xff;
          PartitionSector->Partition[i].StartingHead = StartHead;
          PartitionSector->Partition[i].StartingSector = ((StartCylinder & 0x0300) >> 2) + (StartSector & 0x3f);

          /* Set ending CHS values */
          PartitionSector->Partition[i].EndingCylinder = EndCylinder & 0xff;
          PartitionSector->Partition[i].EndingHead = EndHead;
          PartitionSector->Partition[i].EndingSector = ((EndCylinder & 0x0300) >> 2) + (EndSector & 0x3f);
        }
      else
        {
          PartitionSector->Partition[i].BootFlags = 0;
          PartitionSector->Partition[i].PartitionType = PARTITION_ENTRY_UNUSED;
          PartitionSector->Partition[i].StartingHead = 0;
          PartitionSector->Partition[i].StartingSector = 0;
          PartitionSector->Partition[i].StartingCylinder = 0;
          PartitionSector->Partition[i].EndingHead = 0;
          PartitionSector->Partition[i].EndingSector = 0;
          PartitionSector->Partition[i].EndingCylinder = 0;
          PartitionSector->Partition[i].StartingBlock = 0;
          PartitionSector->Partition[i].SectorCount = 0;
        }
    }


  SectorOffset.QuadPart = 0ULL;
  Status = xHalpWriteSector(DeviceObject,
			    SectorSize,
			    &SectorOffset,
			    (PVOID) PartitionSector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("xHalpWriteSector() failed (Status %lx)\n", Status);
      ExFreePool(PartitionSector);
      return Status;
    }

#ifndef NDEBUG
      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  DPRINT1("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n",
		  i,
		  PartitionSector->Partition[i].BootFlags,
		  PartitionSector->Partition[i].PartitionType,
		  PartitionSector->Partition[i].StartingHead,
		  PartitionSector->Partition[i].StartingSector & 0x3f,
		  (((PartitionSector->Partition[i].StartingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].StartingCylinder,
		  PartitionSector->Partition[i].EndingHead,
		  PartitionSector->Partition[i].EndingSector & 0x3f,
		  (((PartitionSector->Partition[i].EndingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].EndingCylinder,
		  PartitionSector->Partition[i].StartingBlock,
		  PartitionSector->Partition[i].SectorCount);
	}
#endif

  ExFreePool(PartitionSector);

  return(STATUS_SUCCESS);
}

/* EOF */
