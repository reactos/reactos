/* $Id: xhaldrv.c,v 1.17 2002/03/21 19:35:58 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/xhaldrv.c
 * PURPOSE:         Hal drive routines
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
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
#define  PART_MAGIC_OFFSET  0x01fe
#define  PARTITION_OFFSET   0x01be
#define  SIGNATURE_OFFSET   0x01b8
#define  PARTITION_TBL_SIZE 4


#define IsUsablePartition(P)  \
    ((P) == PTDOS3xPrimary  || \
     (P) == PTOLDDOS16Bit   || \
     (P) == PTDos5xPrimary  || \
     (P) == PTWin95FAT32    || \
     (P) == PTWin95FAT32LBA || \
     (P) == PTWin95FAT16LBA)


typedef struct _PARTITION
{
  unsigned char   BootFlags;
  unsigned char   StartingHead;
  unsigned char   StartingSector;
  unsigned char   StartingCylinder;
  unsigned char   PartitionType;
  unsigned char   EndingHead;
  unsigned char   EndingSector;
  unsigned char   EndingCylinder;
  unsigned int  StartingBlock;
  unsigned int  SectorCount;
} PARTITION, *PPARTITION;

typedef struct _PARTITION_TABLE
{
   PARTITION Partition[PARTITION_TBL_SIZE];
   unsigned short Magic;
} PARTITION_TABLE, *PPARTITION_TABLE;

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
	return Status;
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
	return STATUS_INSUFFICIENT_RESOURCES;
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
	ObDereferenceObject(FileObject);
	return Status;
     }

   DPRINT("DiskGeometry.BytesPerSector: %d\n",
	  DiskGeometry.BytesPerSector);

   /* read the partition table */
   Status = IoReadPartitionTable(DeviceObject,
				 DiskGeometry.BytesPerSector,
				 FALSE,
				 LayoutInfo);

   ObDereferenceObject(FileObject);

   return Status;
}


VOID FASTCALL
xHalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
	       IN ULONG SectorSize,
	       IN ULONG MBRTypeIdentifier,
	       OUT PVOID *Buffer)
{
   KEVENT Event;
   IO_STATUS_BLOCK StatusBlock;
   LARGE_INTEGER Offset;
   PUCHAR LocalBuffer;
   PIRP Irp;
   NTSTATUS Status;

   DPRINT("xHalExamineMBR()\n");
   *Buffer = NULL;

   if (SectorSize < 512)
     SectorSize = 512;
   if (SectorSize > 4096)
     SectorSize = 4096;

   LocalBuffer = (PUCHAR)ExAllocatePool(PagedPool,
					SectorSize);
   if (LocalBuffer == NULL)
     return;

   KeInitializeEvent(&Event,
		     NotificationEvent,
		     FALSE);

   Offset.QuadPart = 0;

   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      DeviceObject,
				      LocalBuffer,
				      SectorSize,
				      &Offset,
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
	DPRINT("xHalExamineMBR failed (Status = 0x%08lx)\n",
	       Status);
	ExFreePool(LocalBuffer);
	return;
     }

   if (LocalBuffer[0x1FE] != 0x55 || LocalBuffer[0x1FF] != 0xAA)
     {
	DPRINT("xHalExamineMBR: invalid MBR signature\n");
	ExFreePool(LocalBuffer);
	return;
     }

   if (LocalBuffer[0x1C2] != MBRTypeIdentifier)
     {
	DPRINT("xHalExamineMBR: invalid MBRTypeIdentifier\n");
	ExFreePool(LocalBuffer);
	return;
     }

   *Buffer = (PVOID)LocalBuffer;
}


static VOID
HalpAssignDrive(IN PUNICODE_STRING PartitionName,
		IN OUT PULONG DriveMap,
		IN ULONG DriveNumber)
{
   WCHAR DriveNameBuffer[8];
   UNICODE_STRING DriveName;
   ULONG i;

   DPRINT("HalpAssignDrive()\n");

   if ((DriveNumber != AUTO_DRIVE) && (DriveNumber < 24))
     {
	/* force assignment */
	if ((*DriveMap & (1 << DriveNumber)) != 0)
	  {
	     DbgPrint("Drive letter already used!\n");
	     return;
	  }
     }
   else
     {
	/* automatic assignment */
	DriveNumber = AUTO_DRIVE;

	for (i = 2; i < 24; i++)
	  {
	     if ((*DriveMap & (1 << i)) == 0)
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

   /* set bit in drive map */
   *DriveMap = *DriveMap | (1 << DriveNumber);

   /* build drive name */
   swprintf(DriveNameBuffer,
	    L"\\??\\%C:",
	    'A' + DriveNumber);
   RtlInitUnicodeString(&DriveName,
			DriveNameBuffer);

   DPRINT("  %wZ ==> %wZ\n",
	  &DriveName,
	  PartitionName);

   /* create symbolic link */
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
   ULONG DriveMap = 0;
   ULONG j;

   DPRINT("xHalIoAssignDriveLetters()\n");

   ConfigInfo = IoGetConfigurationInformation ();

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
	     DbgPrint("xHalpQueryDriveLayout() failed (Status = 0x%lx)\n",
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

#if 0
   /* Assign bootable partitions */
   DPRINT("Assigning bootable primary partitions:\n");
   for (i = 0; i < ConfigInfo->DiskCount; i++)
     {
	/* search for bootable partitions */
	for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
	  {
	     if ((LayoutArray[i]->PartitionEntry[j].BootIndicator == TRUE) &&
		 IsUsablePartition(LayoutArray[i]->PartitionEntry[j].PartitionType))
	       {
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   i,
			   LayoutArray[i]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  DPRINT("  %wZ\n", &UnicodeString2);

		  /* assign it */
		  HalpAssignDrive(&UnicodeString2,
				  &DriveMap,
				  AUTO_DRIVE);
	       }
	  }
     }
#endif

   /* Assign bootable partition on first harddisk */
   DPRINT("Assigning bootable primary partition on first harddisk:\n");
   if (ConfigInfo->DiskCount > 0)
     {
	/* search for bootable partition */
	for (j = 0; j < LayoutArray[0]->PartitionCount; j++)
	  {
	     if ((LayoutArray[0]->PartitionEntry[j].BootIndicator == TRUE) &&
		 IsUsablePartition(LayoutArray[0]->PartitionEntry[j].PartitionType))
	       {
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk0\\Partition%d",
			   LayoutArray[0]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  DPRINT("  %wZ\n", &UnicodeString2);

		  /* assign it */
		  HalpAssignDrive(&UnicodeString2,
				  &DriveMap,
				  AUTO_DRIVE);
	       }
	  }
     }

#if 0
   /* Assign non-bootable primary partitions */
   DPRINT("Assigning non-bootable primary partitions:\n");
   for (i = 0; i < ConfigInfo->DiskCount; i++)
     {
	/* search for primary (non-bootable) partitions */
	for (j = 0; j < PARTITION_TBL_SIZE; j++)
	  {
	     if ((LayoutArray[i]->PartitionEntry[j].BootIndicator == FALSE) &&
		 IsUsablePartition(LayoutArray[i]->PartitionEntry[j].PartitionType))
	       {
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   i,
			   LayoutArray[i]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* assign it */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  &DriveMap,
				  AUTO_DRIVE);
	       }
	  }
     }
#endif

   /* Assign remaining  primary partitions */
   DPRINT("Assigning remaining primary partitions:\n");
   for (i = 0; i < ConfigInfo->DiskCount; i++)
     {
	/* search for primary partitions */
	for (j = 0; j < PARTITION_TBL_SIZE; j++)
	  {
	     if (!(i == 0 &&
		   LayoutArray[i]->PartitionEntry[j].BootIndicator == TRUE) &&
		 IsUsablePartition(LayoutArray[i]->PartitionEntry[j].PartitionType))
	       {
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   i,
			   LayoutArray[i]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* assign it */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  &DriveMap,
				  AUTO_DRIVE);
	       }
	  }
     }

   /* Assign extended (logical) partitions */
   DPRINT("Assigning extended (logical) partitions:\n");
   for (i = 0; i < ConfigInfo->DiskCount; i++)
     {
	/* search for extended partitions */
	for (j = PARTITION_TBL_SIZE; j < LayoutArray[i]->PartitionCount; j++)
	  {
	     if (IsUsablePartition(LayoutArray[i]->PartitionEntry[j].PartitionType) &&
		 (LayoutArray[i]->PartitionEntry[j].PartitionNumber != 0))
	       {
		  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   i,
			   LayoutArray[i]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* assign it */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  &DriveMap,
				  AUTO_DRIVE);
	       }
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

	/* assign drive letters A: or B: or first free drive letter */
	DPRINT("  %wZ\n",
	       &UnicodeString1);
	HalpAssignDrive(&UnicodeString1,
			&DriveMap,
			(i < 2) ? i : AUTO_DRIVE);
     }

   /* Assign cdrom drives */
   DPRINT("CD-Rom drives: %d\n", ConfigInfo->CDRomCount);
   for (i = 0; i < ConfigInfo->CDRomCount; i++)
     {
	swprintf(Buffer1,
		 L"\\Device\\Cdrom%d",
		 i);
	RtlInitUnicodeString(&UnicodeString1,
			     Buffer1);

	/* assign first free drive letter */
	DPRINT("  %wZ\n", &UnicodeString1);
	HalpAssignDrive(&UnicodeString1,
			&DriveMap,
			AUTO_DRIVE);
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
   ULARGE_INTEGER  nextPartitionOffset;
   ULARGE_INTEGER  containerOffset;
   PUCHAR SectorBuffer;
   PIRP Irp;
   NTSTATUS Status;
   PPARTITION_TABLE PartitionTable;
   PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
   ULONG i;
   ULONG Count = 0;
   ULONG Number = 1;
   BOOLEAN ExtendedFound = FALSE;

   DPRINT("xHalIoReadPartitionTable(%p %lu %x %p)\n",
	  DeviceObject,
	  SectorSize,
	  ReturnRecognizedPartitions,
	  PartitionBuffer);

   *PartitionBuffer = NULL;

   SectorBuffer = (PUCHAR)ExAllocatePool(PagedPool,
					 SectorSize);
   if (SectorBuffer == NULL)
     {
	return STATUS_INSUFFICIENT_RESOURCES;
     }

   LayoutBuffer = (PDRIVE_LAYOUT_INFORMATION)ExAllocatePool(NonPagedPool,
							    0x1000);
   if (LayoutBuffer == NULL)
     {
	ExFreePool (SectorBuffer);
	return STATUS_INSUFFICIENT_RESOURCES;
     }

   RtlZeroMemory(LayoutBuffer,
		 0x1000);

   PartitionOffset.QuadPart = 0;
   containerOffset.QuadPart = 0;

   do
     {
	KeInitializeEvent(&Event,
			  NotificationEvent,
			  FALSE);

	DPRINT("PartitionOffset: %I64u\n", PartitionOffset.QuadPart / SectorSize);

	Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
					   DeviceObject,
					   SectorBuffer,
					   SectorSize,
					   (PLARGE_INTEGER)&PartitionOffset,
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
	     DbgPrint("xHalIoReadPartitonTable failed (Status = 0x%08lx)\n",
		    Status);
	     ExFreePool(SectorBuffer);
	     ExFreePool(LayoutBuffer);
	     return Status;
	  }

	PartitionTable = (PPARTITION_TABLE)(SectorBuffer+PARTITION_OFFSET);

	/* check the boot sector id */
	DPRINT("Magic %x\n", PartitionTable->Magic);
	if (PartitionTable->Magic != PARTITION_MAGIC)
	  {
	     DbgPrint("Invalid partition table magic\n");
	     ExFreePool(SectorBuffer);
	     *PartitionBuffer = LayoutBuffer;
	     return STATUS_SUCCESS;
	  }

#ifndef NDEBUG
	for (i = 0; i < PARTITION_TBL_SIZE; i++)
	  {
	     DPRINT1("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n", 
		    i,
		    PartitionTable->Partition[i].BootFlags,
		    PartitionTable->Partition[i].PartitionType,
		    PartitionTable->Partition[i].StartingHead,
		    PartitionTable->Partition[i].StartingSector & 0x3f,
		    (((PartitionTable->Partition[i].StartingSector) & 0xc0) << 2) +
          PartitionTable->Partition[i].StartingCylinder,
		    PartitionTable->Partition[i].EndingHead,
		    PartitionTable->Partition[i].EndingSector,
		    PartitionTable->Partition[i].EndingCylinder,
		    PartitionTable->Partition[i].StartingBlock,
		    PartitionTable->Partition[i].SectorCount);
	  }
#endif

	if (ExtendedFound == FALSE);
	  {
	     LayoutBuffer->Signature = *((PULONG)(SectorBuffer + SIGNATURE_OFFSET));
	  }

	ExtendedFound = FALSE;

	for (i = 0; i < PARTITION_TBL_SIZE; i++)
	  {
	     if ((ReturnRecognizedPartitions == FALSE) ||
		 ((ReturnRecognizedPartitions == TRUE) &&
		  IsRecognizedPartition(PartitionTable->Partition[i].PartitionType)))
	       {
		  /* handle normal partition */
		  DPRINT("Partition %u: Normal Partition\n", i);
		  Count = LayoutBuffer->PartitionCount;
		  DPRINT("Logical Partition %u\n", Count);
		  if (PartitionTable->Partition[i].StartingBlock == 0)
		    {
		       LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart = 0;
		    }
		  else if (IsExtendedPartition(PartitionTable->Partition[i].PartitionType))
		    {
		       LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
			(ULONGLONG)PartitionOffset.QuadPart;
		    }
		  else
		    {
		       LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
			(ULONGLONG)PartitionOffset.QuadPart +
			((ULONGLONG)PartitionTable->Partition[i].StartingBlock * (ULONGLONG)SectorSize);
		    }
		  LayoutBuffer->PartitionEntry[Count].PartitionLength.QuadPart =
			(ULONGLONG)PartitionTable->Partition[i].SectorCount * (ULONGLONG)SectorSize;
		  LayoutBuffer->PartitionEntry[Count].HiddenSectors = 0;

		  if (IsRecognizedPartition(PartitionTable->Partition[i].PartitionType))
		    {
		       LayoutBuffer->PartitionEntry[Count].PartitionNumber = Number;
		       Number++;
		    }
		  else
		    {
		       LayoutBuffer->PartitionEntry[Count].PartitionNumber = 0;
		    }

		  LayoutBuffer->PartitionEntry[Count].PartitionType =
			PartitionTable->Partition[i].PartitionType;
		  LayoutBuffer->PartitionEntry[Count].BootIndicator =
			(PartitionTable->Partition[i].BootFlags & 0x80)?TRUE:FALSE;
		  LayoutBuffer->PartitionEntry[Count].RecognizedPartition =
			IsRecognizedPartition (PartitionTable->Partition[i].PartitionType);
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

#if 0
	     if (IsNormalPartition(PartitionTable->Partition[i].PartitionType))
	       {
		  PartitionOffset.QuadPart = (ULONGLONG)PartitionOffset.QuadPart +
		     (((ULONGLONG)PartitionTable->Partition[i].StartingBlock +
		       (ULONGLONG)PartitionTable->Partition[i].SectorCount)* (ULONGLONG)SectorSize);
	       }
#endif

	     if (IsExtendedPartition(PartitionTable->Partition[i].PartitionType))
	       {
		  ExtendedFound = TRUE;
                  if ((ULONGLONG) containerOffset.QuadPart == (ULONGLONG) 0)
                  {
                    containerOffset = PartitionOffset;
                  }
		  nextPartitionOffset.QuadPart = (ULONGLONG) containerOffset.QuadPart +
		     (ULONGLONG) PartitionTable->Partition[i].StartingBlock * 
                     (ULONGLONG) SectorSize;
	       }
	  }
	  PartitionOffset = nextPartitionOffset;
     }
   while (ExtendedFound == TRUE);

   *PartitionBuffer = LayoutBuffer;
   ExFreePool(SectorBuffer);

   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
xHalIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
			      IN ULONG SectorSize,
			      IN ULONG PartitionNumber,
			      IN ULONG PartitionType)
{
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS FASTCALL
xHalIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG SectorSize,
			  IN ULONG SectorsPerTrack,
			  IN ULONG NumberOfHeads,
			  IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
   return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
