/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002, 2003 ReactOS Team
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
/* $Id: disk.c,v 1.28 2003/04/29 18:06:26 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/disk/disk.c
 * PURPOSE:         disk class driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/scsi.h>
#include <ddk/class2.h>
#include <ddk/ntddscsi.h>

#define NDEBUG
#include <debug.h>

#define VERSION  "0.0.1"


typedef struct _DISK_DATA
{
  PDEVICE_EXTENSION NextPartition;
  ULONG Signature;
  ULONG MbrCheckSum;
  ULONG HiddenSectors;
  ULONG PartitionNumber;
  ULONG PartitionOrdinal;
  UCHAR PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN DriveNotReady;
} DISK_DATA, *PDISK_DATA;


BOOLEAN STDCALL
DiskClassFindDevices(PDRIVER_OBJECT DriverObject,
		     PUNICODE_STRING RegistryPath,
		     PCLASS_INIT_DATA InitializationData,
		     PDEVICE_OBJECT PortDeviceObject,
		     ULONG PortNumber);

BOOLEAN STDCALL
DiskClassCheckDevice(IN PINQUIRYDATA InquiryData);

NTSTATUS STDCALL
DiskClassCheckReadWrite(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp);


static NTSTATUS
DiskClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			    IN PUNICODE_STRING RegistryPath, /* what's this used for? */
			    IN PDEVICE_OBJECT PortDeviceObject,
			    IN ULONG PortNumber,
			    IN ULONG DiskNumber,
			    IN PIO_SCSI_CAPABILITIES Capabilities,
			    IN PSCSI_INQUIRY_DATA InquiryData,
			    IN PCLASS_INIT_DATA InitializationData);

NTSTATUS STDCALL
DiskClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

NTSTATUS STDCALL
DiskClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

static BOOLEAN
ScsiDiskSearchForDisk(IN PDEVICE_EXTENSION DeviceExtension,
		      IN HANDLE BusKey,
		      OUT PULONG DetectedDiskNumber);

static VOID
DiskClassUpdatePartitionDeviceObjects (IN PDEVICE_OBJECT DeviceObject,
				       IN PIRP Irp);

static VOID
ScsiDiskUpdateFixedDiskGeometry(IN PDEVICE_EXTENSION DeviceExtension);

static BOOLEAN
ScsiDiskCalcMbrCheckSum(IN PDEVICE_EXTENSION DeviceExtension,
			OUT PULONG Checksum);


/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver, locates and claims 
 *	hardware resources, and creates various NT objects needed
 *	to process I/O requests.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver
 *
 *	RegistryPath
 *		Name of registry driver service key
 *
 * RETURN VALUE
 *	Status
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  CLASS_INIT_DATA InitData;

  DPRINT("Disk Class Driver %s\n",
	 VERSION);
  DPRINT("RegistryPath '%wZ'\n",
	 RegistryPath);

  RtlZeroMemory(&InitData,
		sizeof(CLASS_INIT_DATA));

  InitData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
  InitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION) + sizeof(DISK_DATA);
  InitData.DeviceType = FILE_DEVICE_DISK;
  InitData.DeviceCharacteristics = 0;

  InitData.ClassError = NULL;	// DiskClassProcessError;
  InitData.ClassReadWriteVerification = DiskClassCheckReadWrite;
  InitData.ClassFindDeviceCallBack = DiskClassCheckDevice;
  InitData.ClassFindDevices = DiskClassFindDevices;
  InitData.ClassDeviceControl = DiskClassDeviceControl;
  InitData.ClassShutdownFlush = DiskClassShutdownFlush;
  InitData.ClassCreateClose = NULL;
  InitData.ClassStartIo = NULL;

  ScsiClassInitialize(DriverObject,
		      RegistryPath,
		      &InitData);

  DPRINT1("*** System stopped ***\n");
  for(;;);

  return(ScsiClassInitialize(DriverObject,
			     RegistryPath,
			     &InitData));
}


/**********************************************************************
 * NAME							EXPORTED
 *	DiskClassFindDevices
 *
 * DESCRIPTION
 *	This function searches for device that are attached to the
 *	given scsi port.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver
 *
 *	RegistryPath
 *		Name of registry driver service key
 *
 *	InitializationData
 *		Pointer to the main initialization data
 *
 *	PortDeviceObject
 *		Pointer to the port Device Object
 *
 *	PortNumber
 *		Port number
 *
 * RETURN VALUE
 *	TRUE: At least one disk drive was found
 *	FALSE: No disk drive found
 */

BOOLEAN STDCALL
DiskClassFindDevices(PDRIVER_OBJECT DriverObject,
		     PUNICODE_STRING RegistryPath,
		     PCLASS_INIT_DATA InitializationData,
		     PDEVICE_OBJECT PortDeviceObject,
		     ULONG PortNumber)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  PIO_SCSI_CAPABILITIES PortCapabilities;
  PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
  PSCSI_INQUIRY_DATA UnitInfo;
  PINQUIRYDATA InquiryData;
  PCHAR Buffer;
  ULONG Bus;
  ULONG DeviceCount;
  BOOLEAN FoundDevice;
  NTSTATUS Status;

  DPRINT("DiskClassFindDevices() called.\n");

  /* Get port capabilities */
  Status = ScsiClassGetCapabilities(PortDeviceObject,
				    &PortCapabilities);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetCapabilities() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  DPRINT("PortCapabilities: %p\n", PortCapabilities);
  DPRINT("MaximumTransferLength: %lu\n", PortCapabilities->MaximumTransferLength);
  DPRINT("MaximumPhysicalPages: %lu\n", PortCapabilities->MaximumPhysicalPages);

  /* Get inquiry data */
  Status = ScsiClassGetInquiryData(PortDeviceObject,
				   (PSCSI_ADAPTER_BUS_INFO *)&Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetInquiryData() failed! (Status %x)\n", Status);
      return(FALSE);
    }

  /* Check whether there are unclaimed devices */
  AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;
  DeviceCount = ScsiClassFindUnclaimedDevices(InitializationData,
					      AdapterBusInfo);
  if (DeviceCount == 0)
    {
      DPRINT("No unclaimed devices!\n");
      return(FALSE);
    }

  DPRINT("Found %lu unclaimed devices!\n", DeviceCount);

  ConfigInfo = IoGetConfigurationInformation();

  /* Search each bus of this adapter */
  for (Bus = 0; Bus < (ULONG)AdapterBusInfo->NumberOfBuses; Bus++)
    {
      DPRINT("Searching bus %lu\n", Bus);

      UnitInfo = (PSCSI_INQUIRY_DATA)(Buffer + AdapterBusInfo->BusData[Bus].InquiryDataOffset);

      while (AdapterBusInfo->BusData[Bus].InquiryDataOffset)
	{
	  InquiryData = (PINQUIRYDATA)UnitInfo->InquiryData;

	  if (((InquiryData->DeviceType == DIRECT_ACCESS_DEVICE) ||
	       (InquiryData->DeviceType == OPTICAL_DEVICE)) &&
	      (InquiryData->DeviceTypeQualifier == 0) &&
	      (UnitInfo->DeviceClaimed == FALSE))
	    {
	      DPRINT("Vendor: '%.24s'\n",
		     InquiryData->VendorId);

	      /* Create device objects for disk */
	      Status = DiskClassCreateDeviceObject(DriverObject,
						   RegistryPath,
						   PortDeviceObject,
						   PortNumber,
						   ConfigInfo->DiskCount,
						   PortCapabilities,
						   UnitInfo,
						   InitializationData);
	      if (NT_SUCCESS(Status))
		{
		  ConfigInfo->DiskCount++;
		  FoundDevice = TRUE;
		}
	    }

	  if (UnitInfo->NextInquiryDataOffset == 0)
	    break;

	  UnitInfo = (PSCSI_INQUIRY_DATA)(Buffer + UnitInfo->NextInquiryDataOffset);
	}
    }

  ExFreePool(Buffer);

  DPRINT("DiskClassFindDevices() done\n");

  return(FoundDevice);
}


/**********************************************************************
 * NAME							EXPORTED
 *	DiskClassCheckDevice
 *
 * DESCRIPTION
 *	This function checks the InquiryData for the correct device
 *	type and qualifier.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	InquiryData
 *		Pointer to the inquiry data for the device in question.
 *
 * RETURN VALUE
 *	TRUE: A disk device was found.
 *	FALSE: Otherwise.
 */

BOOLEAN STDCALL
DiskClassCheckDevice(IN PINQUIRYDATA InquiryData)
{
  return((InquiryData->DeviceType == DIRECT_ACCESS_DEVICE ||
	  InquiryData->DeviceType == OPTICAL_DEVICE) &&
	 InquiryData->DeviceTypeQualifier == 0);
}


/**********************************************************************
 * NAME							EXPORTED
 *	DiskClassCheckReadWrite
 *
 * DESCRIPTION
 *	This function checks the given IRP for correct data.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to the device.
 *
 *	Irp
 *		Irp to check.
 *
 * RETURN VALUE
 *	STATUS_SUCCESS: The IRP matches the requirements of the given device.
 *	Others: Failure.
 */

NTSTATUS STDCALL
DiskClassCheckReadWrite(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PDISK_DATA DiskData;

  DPRINT("DiskClassCheckReadWrite() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  DiskData = (PDISK_DATA)(DeviceExtension + 1);

  if (DiskData->DriveNotReady == TRUE)
    {
      Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
      IoSetHardErrorOrVerifyDevice(Irp,
				   DeviceObject);
      return(STATUS_INVALID_PARAMETER);
    }

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	DiskClassCreateDeviceObject
 *
 * DESCRIPTION
 *	Create the raw device and any partition devices on this drive
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		The system created driver object
 *	RegistryPath
 *	PortDeviceObject
 *	PortNumber
 *	DiskNumber
 *	Capabilities
 *	InquiryData
 *	InitialzationData
 *
 * RETURN VALUE
 *	STATUS_SUCCESS: Device objects for disk and partitions were created.
 *	Others: Failure.
 */

static NTSTATUS
DiskClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			    IN PUNICODE_STRING RegistryPath, /* what's this used for? */
			    IN PDEVICE_OBJECT PortDeviceObject,
			    IN ULONG PortNumber,
			    IN ULONG DiskNumber,
			    IN PIO_SCSI_CAPABILITIES Capabilities,
			    IN PSCSI_INQUIRY_DATA InquiryData,
			    IN PCLASS_INIT_DATA InitializationData)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeDeviceDirName;
  WCHAR NameBuffer[80];
  CHAR NameBuffer2[80];
  PDEVICE_OBJECT DiskDeviceObject;
  PDEVICE_OBJECT PartitionDeviceObject;
  PDEVICE_EXTENSION DiskDeviceExtension; /* defined in class2.h */
  PDEVICE_EXTENSION PartitionDeviceExtension; /* defined in class2.h */
  PDRIVE_LAYOUT_INFORMATION PartitionList = NULL;
  HANDLE Handle;
  PPARTITION_INFORMATION PartitionEntry;
  PDISK_DATA DiskData;
  ULONG PartitionNumber;
  PVOID MbrBuffer;
  NTSTATUS Status;

  DPRINT("DiskClassCreateDeviceObject() called\n");

  /* Create the harddisk device directory */
  swprintf(NameBuffer,
	   L"\\Device\\Harddisk%lu",
	   DiskNumber);
  RtlInitUnicodeString(&UnicodeDeviceDirName,
		       NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeDeviceDirName,
			     0,
			     NULL,
			     NULL);
  Status = ZwCreateDirectoryObject(&Handle,
				   0,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not create device dir object\n");
      return(Status);
    }

  /* Claim the disk device */
  Status = ScsiClassClaimDevice(PortDeviceObject,
				InquiryData,
				FALSE,
				&PortDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not claim disk device\n");

      ZwMakeTemporaryObject(Handle);
      ZwClose(Handle);

      return(Status);
    }

  /* Create disk device (Partition 0) */
  sprintf(NameBuffer2,
	  "\\Device\\Harddisk%lu\\Partition0",
	  DiskNumber);

  Status = ScsiClassCreateDeviceObject(DriverObject,
				       NameBuffer2,
				       NULL,
				       &DiskDeviceObject,
				       InitializationData);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassCreateDeviceObject() failed (Status %x)\n", Status);

      /* Release (unclaim) the disk */
      ScsiClassClaimDevice(PortDeviceObject,
			   InquiryData,
			   TRUE,
			   NULL);

      /* Delete the harddisk device directory */
      ZwMakeTemporaryObject(Handle);
      ZwClose(Handle);

      return(Status);
    }

  DiskDeviceObject->Flags |= DO_DIRECT_IO;
  if (((PINQUIRYDATA)InquiryData->InquiryData)->RemovableMedia)
    {
      DiskDeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
    }
  DiskDeviceObject->StackSize = (CCHAR)PortDeviceObject->StackSize + 1;

  if (PortDeviceObject->AlignmentRequirement > DiskDeviceObject->AlignmentRequirement)
    {
      DiskDeviceObject->AlignmentRequirement = PortDeviceObject->AlignmentRequirement;
    }

  DiskDeviceExtension = DiskDeviceObject->DeviceExtension;
  DiskDeviceExtension->LockCount = 0;
  DiskDeviceExtension->DeviceNumber = DiskNumber;
  DiskDeviceExtension->DeviceObject = DiskDeviceObject;
  DiskDeviceExtension->PortDeviceObject = PortDeviceObject;
  DiskDeviceExtension->PhysicalDevice = DiskDeviceObject;
  DiskDeviceExtension->PortCapabilities = Capabilities;
  DiskDeviceExtension->StartingOffset.QuadPart = 0;
  DiskDeviceExtension->PortNumber = (UCHAR)PortNumber;
  DiskDeviceExtension->PathId = InquiryData->PathId;
  DiskDeviceExtension->TargetId = InquiryData->TargetId;
  DiskDeviceExtension->Lun = InquiryData->Lun;

  /* Initialize the lookaside list for SRBs */
  ScsiClassInitializeSrbLookasideList(DiskDeviceExtension,
				      4);

  /* zero-out disk data */
  DiskData = (PDISK_DATA)(DiskDeviceExtension + 1);
  RtlZeroMemory(DiskData,
		sizeof(DISK_DATA));

  /* Get disk geometry */
  DiskDeviceExtension->DiskGeometry = ExAllocatePool(NonPagedPool,
						     sizeof(DISK_GEOMETRY));
  if (DiskDeviceExtension->DiskGeometry == NULL)
    {
      DPRINT("Failed to allocate geometry buffer!\n");

      ExDeleteNPagedLookasideList(&DiskDeviceExtension->SrbLookasideListHead);

      IoDeleteDevice(DiskDeviceObject);

      /* Release (unclaim) the disk */
      ScsiClassClaimDevice(PortDeviceObject,
			   InquiryData,
			   TRUE,
			   NULL);

      /* Delete the harddisk device directory */
      ZwMakeTemporaryObject(Handle);
      ZwClose(Handle);

      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Read the drive's capacity */
  Status = ScsiClassReadDriveCapacity(DiskDeviceObject);
  if (!NT_SUCCESS(Status) &&
      (DiskDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) == 0)
    {
      DPRINT1("Failed to retrieve drive capacity!\n");
      return(STATUS_SUCCESS);
    }
  else
    {
      /* Clear the verify flag for removable media drives. */
      DiskDeviceObject->Flags &= ~DO_VERIFY_VOLUME;
    }

  DPRINT("SectorSize: %lu\n", DiskDeviceExtension->DiskGeometry->BytesPerSector);

  /* Check disk for presence of a disk manager */
  HalExamineMBR(DiskDeviceObject,
		DiskDeviceExtension->DiskGeometry->BytesPerSector,
		0x54,
		&MbrBuffer);
  if (MbrBuffer != NULL)
    {
      /* Start disk at sector 63 if the Ontrack Disk Manager was found */
      DPRINT("Found 'Ontrack Disk Manager'!\n");

      DiskDeviceExtension->DMSkew = 63;
      DiskDeviceExtension->DMByteSkew =
	63 * DiskDeviceExtension->DiskGeometry->BytesPerSector;
      DiskDeviceExtension->DMActive = TRUE;

      ExFreePool(MbrBuffer);
      MbrBuffer = NULL;
    }

  if ((DiskDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) &&
      (DiskDeviceExtension->DiskGeometry->MediaType == RemovableMedia))
    {
      /* Allocate a partition list for a single entry. */
      PartitionList = ExAllocatePool(NonPagedPool,
				     sizeof(DRIVE_LAYOUT_INFORMATION));
      if (PartitionList != NULL)
	{
	  RtlZeroMemory(PartitionList,
			sizeof(DRIVE_LAYOUT_INFORMATION));
	  PartitionList->PartitionCount = 1;

	  DiskData->DriveNotReady = TRUE;
	  Status = STATUS_SUCCESS;
	}
    }
  else
    {
      /* Read partition table */
      Status = IoReadPartitionTable(DiskDeviceObject,
				    DiskDeviceExtension->DiskGeometry->BytesPerSector,
				    TRUE,
				    &PartitionList);

      DPRINT("IoReadPartitionTable(): Status: %lx\n", Status);

      if ((!NT_SUCCESS(Status) || PartitionList->PartitionCount == 0) &&
	  DiskDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
	{
	  if (!NT_SUCCESS(Status))
	    {
	      /* Drive is not ready. */
	      DPRINT("Drive not ready\n");
	      DiskData->DriveNotReady = TRUE;
	    }
	  else
	    {
	      ExFreePool(PartitionList);
	    }

	  /* Allocate a partition list for a single entry. */
	  PartitionList = ExAllocatePool(NonPagedPool,
					 sizeof(DRIVE_LAYOUT_INFORMATION));
	  if (PartitionList != NULL)
	    {
	      RtlZeroMemory(PartitionList,
			    sizeof(DRIVE_LAYOUT_INFORMATION));
	      PartitionList->PartitionCount = 1;

	      Status = STATUS_SUCCESS;
	    }
	}
    }

  if (NT_SUCCESS(Status))
    {
      DPRINT("Read partition table!\n");
      DPRINT("  Number of partitions: %u\n", PartitionList->PartitionCount);

      /* Set disk signature */
      DiskData->Signature = PartitionList->Signature;

      /* Calculate MBR checksum if disk got no signature */
      if (DiskData->Signature == 0)
	{
	  if (!ScsiDiskCalcMbrCheckSum(DiskDeviceExtension,
				       &DiskData->MbrCheckSum))
	    {
	      DPRINT1("MBR checksum calculation failed for disk %lu\n",
		      DiskDeviceExtension->DeviceNumber);
	    }
	  else
	    {
	      DPRINT1("MBR checksum for disk %lu is %lx\n",
		      DiskDeviceExtension->DeviceNumber,
		      DiskData->MbrCheckSum);
	    }
	}
      else
	{
	  DPRINT1("Signature on disk %lu is %lx\n",
		  DiskDeviceExtension->DeviceNumber,
		  DiskData->Signature);
	}

      /* Update disk geometry if disk is visible to the BIOS */
      ScsiDiskUpdateFixedDiskGeometry(DiskDeviceExtension);

      for (PartitionNumber = 0; PartitionNumber < PartitionList->PartitionCount; PartitionNumber++)
	{
	  PartitionEntry = &PartitionList->PartitionEntry[PartitionNumber];

	  DPRINT("Partition %02ld: nr: %d boot: %1x type: %x offset: %I64d size: %I64d\n",
		 PartitionNumber,
		 PartitionEntry->PartitionNumber,
		 PartitionEntry->BootIndicator,
		 PartitionEntry->PartitionType,
		 PartitionEntry->StartingOffset.QuadPart / 512 /*DrvParms.BytesPerSector*/,
		 PartitionEntry->PartitionLength.QuadPart / 512 /* DrvParms.BytesPerSector*/);

	  /* Create partition device object */
	  sprintf(NameBuffer2,
		  "\\Device\\Harddisk%lu\\Partition%lu",
		  DiskNumber,
		  PartitionNumber + 1);

	  Status = ScsiClassCreateDeviceObject(DriverObject,
					       NameBuffer2,
					       DiskDeviceObject,
					       &PartitionDeviceObject,
					       InitializationData);
	  DPRINT("ScsiClassCreateDeviceObject(): Status %x\n", Status);
	  if (NT_SUCCESS(Status))
	    {
	      PartitionDeviceObject->Flags = DiskDeviceObject->Flags;
	      PartitionDeviceObject->Characteristics = DiskDeviceObject->Characteristics;
	      PartitionDeviceObject->StackSize = DiskDeviceObject->StackSize;
	      PartitionDeviceObject->AlignmentRequirement = DiskDeviceObject->AlignmentRequirement;

	      PartitionDeviceExtension = PartitionDeviceObject->DeviceExtension;
	      PartitionDeviceExtension->LockCount = 0;
	      PartitionDeviceExtension->DeviceNumber = DiskNumber;
	      PartitionDeviceExtension->DeviceObject = PartitionDeviceObject;
	      PartitionDeviceExtension->PortDeviceObject = PortDeviceObject;
	      PartitionDeviceExtension->DiskGeometry = DiskDeviceExtension->DiskGeometry;
	      PartitionDeviceExtension->PhysicalDevice = DiskDeviceExtension->PhysicalDevice;
	      PartitionDeviceExtension->PortCapabilities = Capabilities;
	      PartitionDeviceExtension->StartingOffset.QuadPart =
		PartitionEntry->StartingOffset.QuadPart;
	      PartitionDeviceExtension->PartitionLength.QuadPart =
		PartitionEntry->PartitionLength.QuadPart;
	      PartitionDeviceExtension->DMSkew = DiskDeviceExtension->DMSkew;
	      PartitionDeviceExtension->DMByteSkew = DiskDeviceExtension->DMByteSkew;
	      PartitionDeviceExtension->DMActive = DiskDeviceExtension->DMActive;
	      PartitionDeviceExtension->PortNumber = (UCHAR)PortNumber;
	      PartitionDeviceExtension->PathId = InquiryData->PathId;
	      PartitionDeviceExtension->TargetId = InquiryData->TargetId;
	      PartitionDeviceExtension->Lun = InquiryData->Lun;
	      PartitionDeviceExtension->SectorShift = DiskDeviceExtension->SectorShift;

	      /* Initialize lookaside list for SRBs */
	      ScsiClassInitializeSrbLookasideList(PartitionDeviceExtension,
						  8);

	      /* Link current partition device extension to previous disk data */
	      DiskData->NextPartition = PartitionDeviceExtension;

	      /* Initialize current disk data */
	      DiskData = (PDISK_DATA)(PartitionDeviceExtension + 1);
	      DiskData->NextPartition = NULL;
	      DiskData->PartitionType = PartitionEntry->PartitionType;
	      DiskData->PartitionNumber = PartitionNumber + 1;
	      DiskData->PartitionOrdinal = PartitionNumber + 1;
	      DiskData->HiddenSectors = PartitionEntry->HiddenSectors;
	      DiskData->BootIndicator = PartitionEntry->BootIndicator;
	      DiskData->DriveNotReady = FALSE;
	    }
	  else
	    {
	      DPRINT1("ScsiClassCreateDeviceObject() failed to create partition device object (Status %x)\n", Status);

	      break;
	    }
	}
    }

  if (PartitionList != NULL)
    ExFreePool(PartitionList);

  DPRINT("DiskClassCreateDeviceObjects() done\n");

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	DiskClassDeviceControl
 *
 * DESCRIPTION
 *	Answer requests for device control calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	Status
 */

NTSTATUS STDCALL
DiskClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  ULONG ControlCode, InputLength, OutputLength;
  PDISK_DATA DiskData;
  ULONG Information;
  NTSTATUS Status;

  DPRINT("DiskClassDeviceControl() called!\n");

  Status = STATUS_INVALID_DEVICE_REQUEST;
  Information = 0;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
  InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  DiskData = (PDISK_DATA)(DeviceExtension + 1);

  switch (ControlCode)
    {
      case IOCTL_DISK_GET_DRIVE_GEOMETRY:
	DPRINT("IOCTL_DISK_GET_DRIVE_GEOMETRY\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
	  {
	    Status = STATUS_INVALID_PARAMETER;
	  }
	else
	  {
	    PDISK_GEOMETRY Geometry;

	    if (DeviceExtension->DiskGeometry == NULL)
	      {
		DPRINT("No disk geometry available!\n");
		DeviceExtension->DiskGeometry = ExAllocatePool(NonPagedPool,
							       sizeof(DISK_GEOMETRY));
	      }
	    Status = ScsiClassReadDriveCapacity(DeviceObject);
	    DPRINT("ScsiClassReadDriveCapacity() returned (Status %lx)\n", Status);
	    if (NT_SUCCESS(Status))
	      {
		Geometry = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;
		RtlMoveMemory(Geometry,
			      DeviceExtension->DiskGeometry,
			      sizeof(DISK_GEOMETRY));

		Status = STATUS_SUCCESS;
		Information = sizeof(DISK_GEOMETRY);
	      }
	  }
	break;

      case IOCTL_DISK_GET_PARTITION_INFO:
	DPRINT("IOCTL_DISK_GET_PARTITION_INFO\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
	    sizeof(PARTITION_INFORMATION))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else if (DiskData->PartitionNumber == 0)
	  {
	    Status = STATUS_INVALID_DEVICE_REQUEST;
	  }
	else
	  {
	    PPARTITION_INFORMATION PartitionInfo;

	    PartitionInfo = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	    PartitionInfo->PartitionType = DiskData->PartitionType;
	    PartitionInfo->StartingOffset = DeviceExtension->StartingOffset;
	    PartitionInfo->PartitionLength = DeviceExtension->PartitionLength;
	    PartitionInfo->HiddenSectors = DiskData->HiddenSectors;
	    PartitionInfo->PartitionNumber = DiskData->PartitionNumber;
	    PartitionInfo->BootIndicator = DiskData->BootIndicator;
	    PartitionInfo->RewritePartition = FALSE;
	    PartitionInfo->RecognizedPartition =
	      IsRecognizedPartition(DiskData->PartitionType);

	    Status = STATUS_SUCCESS;
	    Information = sizeof(PARTITION_INFORMATION);
	  }
	break;

      case IOCTL_DISK_SET_PARTITION_INFO:
	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength <
	    sizeof(SET_PARTITION_INFORMATION))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else if (DiskData->PartitionNumber == 0)
	  {
	    Status = STATUS_INVALID_DEVICE_REQUEST;
	  }
	else
	  {
	    PSET_PARTITION_INFORMATION PartitionInfo;

	    PartitionInfo = (PSET_PARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	    Status = IoSetPartitionInformation(DeviceExtension->PhysicalDevice,
					       DeviceExtension->DiskGeometry->BytesPerSector,
					       DiskData->PartitionOrdinal,
					       PartitionInfo->PartitionType);
	    if (NT_SUCCESS(Status))
	      {
		DiskData->PartitionType = PartitionInfo->PartitionType;
	      }
	  }
	break;

      case IOCTL_DISK_GET_DRIVE_LAYOUT:
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
	    sizeof(DRIVE_LAYOUT_INFORMATION))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    PDRIVE_LAYOUT_INFORMATION PartitionList;

	    Status = IoReadPartitionTable(DeviceExtension->PhysicalDevice,
					  DeviceExtension->DiskGeometry->BytesPerSector,
					  FALSE,
					  &PartitionList);
	    if (NT_SUCCESS(Status))
	      {
		ULONG BufferSize;

		BufferSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
					  PartitionEntry[0]);
		BufferSize += PartitionList->PartitionCount * sizeof(PARTITION_INFORMATION);

		if (BufferSize > IrpStack->Parameters.DeviceIoControl.OutputBufferLength)
		  {
		    Status = STATUS_BUFFER_TOO_SMALL;
		  }
		else
		  {
		    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
				  PartitionList,
				  BufferSize);
		    Status = STATUS_SUCCESS;
		    Information = BufferSize;
		  }
		ExFreePool(PartitionList);
	      }
	  }
	break;

      case IOCTL_DISK_SET_DRIVE_LAYOUT:
	if (IrpStack->Parameters.DeviceIoControl.InputBufferLength <
	    sizeof(DRIVE_LAYOUT_INFORMATION))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else if (DeviceExtension->PhysicalDevice->DeviceExtension != DeviceExtension)
	  {
	    Status = STATUS_INVALID_PARAMETER;
	  }
	else
	  {
	    PDRIVE_LAYOUT_INFORMATION PartitionList;
	    ULONG TableSize;

	    PartitionList = Irp->AssociatedIrp.SystemBuffer;
	    TableSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
			((PartitionList->PartitionCount - 1) * sizeof(PARTITION_INFORMATION));

	    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < TableSize)
	      {
		Status = STATUS_BUFFER_TOO_SMALL;
	      }
	    else
	      {
		/* Update partition device objects */
		DiskClassUpdatePartitionDeviceObjects (DeviceObject,
						       Irp);

		/* Write partition table */
		Status = IoWritePartitionTable(DeviceExtension->PhysicalDevice,
					       DeviceExtension->DiskGeometry->BytesPerSector,
					       DeviceExtension->DiskGeometry->SectorsPerTrack,
					       DeviceExtension->DiskGeometry->TracksPerCylinder,
					       PartitionList);
		if (NT_SUCCESS(Status))
		  {
		    Information = TableSize;
		  }
	      }
	  }
	break;

      case IOCTL_DISK_VERIFY:
      case IOCTL_DISK_FORMAT_TRACKS:
      case IOCTL_DISK_PERFORMANCE:
      case IOCTL_DISK_IS_WRITABLE:
      case IOCTL_DISK_LOGGING:
      case IOCTL_DISK_FORMAT_TRACKS_EX:
      case IOCTL_DISK_HISTOGRAM_STRUCTURE:
      case IOCTL_DISK_HISTOGRAM_DATA:
      case IOCTL_DISK_HISTOGRAM_RESET:
      case IOCTL_DISK_REQUEST_STRUCTURE:
      case IOCTL_DISK_REQUEST_DATA:
	/* If we get here, something went wrong. Inform the requestor */
	DPRINT1("Unhandled control code: %lx\n", ControlCode);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	Information = 0;
	break;

      default:
	/* Call the common device control function */
	return(ScsiClassDeviceControl(DeviceObject, Irp));
    }

  /* Verify the device if the user caused the error */
  if (!NT_SUCCESS(Status) && IoIsErrorUserInduced(Status))
    {
      IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	DiskClassShutdownFlush
 *
 * DESCRIPTION
 *	Answer requests for shutdown and flush calls.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to the device.
 *
 *	Irp
 *		Pointer to the IRP
 *
 * RETURN VALUE
 *	Status
 */

NTSTATUS STDCALL
DiskClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("DiskClassShutdownFlush() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;

  /* Allocate SRB */
  Srb = ExAllocatePool(NonPagedPool,
		       sizeof(SCSI_REQUEST_BLOCK));
  if (Srb == NULL)
    {
      Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize SRB */
  RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

  /* Set device IDs */
  Srb->PathId = DeviceExtension->PathId;
  Srb->TargetId = DeviceExtension->TargetId;
  Srb->Lun = DeviceExtension->Lun;

  /* Flush write cache */
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
  Srb->CdbLength = 10;
  Srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;
  ScsiClassSendSrbSynchronous(DeviceObject,
			      Srb,
			      NULL,
			      0,
			      TRUE);

  /* Get current stack location */
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  /* FIXME: Unlock removable media upon shutdown */


  /* No retry */
  IrpStack->Parameters.Others.Argument4 = (PVOID)0;

  /* Send shutdown or flush request to the port driver */
  Srb->CdbLength = 0;
  if (IrpStack->MajorFunction == IRP_MJ_SHUTDOWN)
    Srb->Function = SRB_FUNCTION_SHUTDOWN;
  else
    Srb->Function = SRB_FUNCTION_FLUSH;

  /* Init completion routine */
  IoSetCompletionRoutine(Irp,
			 ScsiClassIoComplete,
			 Srb,
			 TRUE,
			 TRUE,
			 TRUE);

  /* Prepare next stack location for a call to the port driver */
  IrpStack = IoGetNextIrpStackLocation(Irp);
  IrpStack->MajorFunction = IRP_MJ_SCSI;
  IrpStack->Parameters.Scsi.Srb = Srb;
  Srb->OriginalRequest = Irp;

  /* Call port driver */
  return(IoCallDriver(DeviceExtension->PortDeviceObject, Irp));
}


/**********************************************************************
 * NAME							INTERNAL
 *	DiskClassUpdatePartitionDeviceObjects
 *
 * DESCRIPTION
 *	Deletes, modifies or creates partition device objects.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to the device.
 *
 *	Irp
 *		Pointer to the IRP
 *
 * RETURN VALUE
 *	None
 */

static VOID
DiskClassUpdatePartitionDeviceObjects(IN PDEVICE_OBJECT DiskDeviceObject,
				      IN PIRP Irp)
{
  PDRIVE_LAYOUT_INFORMATION PartitionList;
  PPARTITION_INFORMATION PartitionEntry;
  PDEVICE_EXTENSION DeviceExtension;
  PDEVICE_EXTENSION DiskDeviceExtension;
  PDISK_DATA DiskData;
  ULONG PartitionCount;
  ULONG PartitionOrdinal;
  ULONG PartitionNumber;
  ULONG LastPartitionNumber;
  ULONG i;
  BOOLEAN Found;
  WCHAR NameBuffer[MAX_PATH];
  UNICODE_STRING DeviceName;
  PDEVICE_OBJECT DeviceObject;
  NTSTATUS Status;

  DPRINT("ScsiDiskUpdatePartitionDeviceObjects() called\n");

  /* Get partition list */
  PartitionList = Irp->AssociatedIrp.SystemBuffer;

  /* Round partition count up by 4 */
  PartitionCount = ((PartitionList->PartitionCount + 3) / 4) * 4;

  /* Remove the partition numbers from the partition list */
  for (i = 0; i < PartitionCount; i++)
    {
      PartitionList->PartitionEntry[i].PartitionNumber = 0;
    }

  DiskDeviceExtension = DiskDeviceObject->DeviceExtension;

  /* Traverse on-disk partition list */
  LastPartitionNumber = 0;
  DeviceExtension = DiskDeviceExtension;
  DiskData = (PDISK_DATA)(DeviceExtension + 1);
  while (TRUE)
    {
      DeviceExtension = DiskData->NextPartition;
      if (DeviceExtension == NULL)
	break;

      /* Get disk data */
      DiskData = (PDISK_DATA)(DeviceExtension + 1);

      /* Update last partition number */
      if (DiskData->PartitionNumber > LastPartitionNumber)
	LastPartitionNumber = DiskData->PartitionNumber;

      /* Ignore unused on-disk partitions */
      if (DeviceExtension->PartitionLength.QuadPart == 0ULL)
	continue;

      Found = FALSE;
      PartitionOrdinal = 0;
      for (i = 0; i < PartitionCount; i++)
	{
	  /* Get current partition entry */
	  PartitionEntry = &PartitionList->PartitionEntry[i];

	  /* Ignore empty (aka unused) or extended partitions */
	  if (PartitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
	      IsContainerPartition (PartitionEntry->PartitionType))
	    continue;

	  PartitionOrdinal++;

	  /* Check for matching partition start offset and length */
	  if ((PartitionEntry->StartingOffset.QuadPart !=
	       DeviceExtension->StartingOffset.QuadPart) ||
	      (PartitionEntry->PartitionLength.QuadPart !=
	       DeviceExtension->PartitionLength.QuadPart))
	    continue;

	  DPRINT1("Found matching partition entry for partition %lu\n",
		  DiskData->PartitionNumber);

	  /* Found matching partition */
	  Found = TRUE;

	  /* Update partition number in partition list */
	  PartitionEntry->PartitionNumber = DiskData->PartitionNumber;
	  break;
	}

      if (Found == TRUE)
	{
	  /* Get disk data for current partition */
	  DiskData = (PDISK_DATA)(DeviceExtension + 1);

	  /* Update partition type if partiton will be rewritten */
	  if (PartitionEntry->RewritePartition == TRUE)
	    DiskData->PartitionType = PartitionEntry->PartitionType;

	  /* Assign new partiton ordinal */
	  DiskData->PartitionOrdinal = PartitionOrdinal;

	  DPRINT("Partition ordinal %lu was assigned to partition %lu\n",
		 DiskData->PartitionOrdinal,
		 DiskData->PartitionNumber);
	}
      else
	{
	  /* Delete this partition */
	  DeviceExtension->PartitionLength.QuadPart = 0ULL;

	  DPRINT("Deleting partition %lu\n",
		 DiskData->PartitionNumber);
	}
    }

  /* Traverse partiton list and create new partiton devices */
  PartitionOrdinal = 0;
  for (i = 0; i < PartitionCount; i++)
    {
      /* Get current partition entry */
      PartitionEntry = &PartitionList->PartitionEntry[i];

      /* Ignore empty (aka unused) or extended partitions */
      if (PartitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
	  IsContainerPartition (PartitionEntry->PartitionType))
	continue;

      PartitionOrdinal++;

      /* Ignore unchanged partition entries */
      if (PartitionEntry->RewritePartition == FALSE)
	continue;

      /* Check for an unused device object */
      PartitionNumber = 0;
      DeviceExtension = DiskDeviceExtension;
      DiskData = (PDISK_DATA)(DeviceExtension + 1);
      while (TRUE)
	{
	  DeviceExtension = DiskData->NextPartition;
	  if (DeviceExtension == NULL)
	    break;

	  /* Get partition disk data */
	  DiskData = (PDISK_DATA)(DeviceExtension + 1);

	  /* Found a free (unused) partition (device object) */
	  if (DeviceExtension->PartitionLength.QuadPart == 0ULL)
	    {
	      PartitionNumber = DiskData->PartitionNumber;
	      break;
	    }
	}

      if (PartitionNumber == 0)
	{
	  /* Create a new partition device object */
	  DPRINT("Create new partition device object\n");

	  /* Get new partiton number */
	  LastPartitionNumber++;
	  PartitionNumber = LastPartitionNumber;

	  /* Create partition device object */
	  swprintf(NameBuffer,
		   L"\\Device\\Harddisk%lu\\Partition%lu",
		   DiskDeviceExtension->DeviceNumber,
		   PartitionNumber);
	  RtlInitUnicodeString(&DeviceName,
			       NameBuffer);

	  Status = IoCreateDevice(DiskDeviceObject->DriverObject,
				  sizeof(DEVICE_EXTENSION) + sizeof(DISK_DATA),
				  &DeviceName,
				  FILE_DEVICE_DISK,
				  0,
				  FALSE,
				  &DeviceObject);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("IoCreateDevice() failed (Status %lx)\n", Status);
	      continue;
	    }

	  DeviceObject->Flags |= DO_DIRECT_IO;
	  DeviceObject->StackSize = DiskDeviceObject->StackSize;
	  DeviceObject->Characteristics = DiskDeviceObject->Characteristics;
	  DeviceObject->AlignmentRequirement = DiskDeviceObject->AlignmentRequirement;

	  /* Initialize device extension */
	  DeviceExtension = DeviceObject->DeviceExtension;
	  RtlCopyMemory(DeviceExtension,
			DiskDeviceObject->DeviceExtension,
			sizeof(DEVICE_EXTENSION));
	  DeviceExtension->DeviceObject = DeviceObject;

	  /* Initialize lookaside list for SRBs */
	  ScsiClassInitializeSrbLookasideList(DeviceExtension,
					      8);

	  /* Link current partition device extension to previous disk data */
	  DiskData->NextPartition = DeviceExtension;
	  DiskData = (PDISK_DATA)(DeviceExtension + 1);
	  DiskData->NextPartition = NULL;
	}
      else
	{
	  /* Reuse an existing partition device object */
	  DPRINT("Reuse an exisiting partition device object\n");
	  DiskData = (PDISK_DATA)(DeviceExtension + 1);
	}

      /* Update partition data and device extension */
      DiskData->PartitionNumber = PartitionNumber;
      DiskData->PartitionOrdinal = PartitionOrdinal;
      DiskData->PartitionType = PartitionEntry->PartitionType;
      DiskData->BootIndicator = PartitionEntry->BootIndicator;
      DiskData->HiddenSectors = PartitionEntry->HiddenSectors;
      DeviceExtension->StartingOffset = PartitionEntry->StartingOffset;
      DeviceExtension->PartitionLength = PartitionEntry->PartitionLength;

      /* Update partition number in the partition list */
      PartitionEntry->PartitionNumber = PartitionNumber;

      DPRINT("Partition ordinal %lu was assigned to partition %lu\n",
	     DiskData->PartitionOrdinal,
	     DiskData->PartitionNumber);
    }

  DPRINT("ScsiDiskUpdatePartitionDeviceObjects() done\n");
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiDiskSearchForDisk
 *
 * DESCRIPTION
 *	Searches the hardware tree for the given disk.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Disk device extension.
 *
 *	BusKey
 *		Handle to the hardware bus key.
 *
 *	DetectedDiskNumber
 *		Returned disk number.
 *
 * RETURN VALUE
 *	TRUE: Disk was found.
 *	FALSE: Search failed.
 */

static BOOLEAN
ScsiDiskSearchForDisk(IN PDEVICE_EXTENSION DeviceExtension,
		      IN HANDLE BusKey,
		      OUT PULONG DetectedDiskNumber)
{
  PKEY_VALUE_FULL_INFORMATION ValueData;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PDISK_DATA DiskData;
  UNICODE_STRING IdentifierString;
  UNICODE_STRING NameString;
  HANDLE BusInstanceKey;
  HANDLE ControllerKey;
  HANDLE DiskKey;
  HANDLE DiskInstanceKey;
  ULONG BusNumber;
  ULONG ControllerNumber;
  ULONG DiskNumber;
  ULONG Length;
  WCHAR Buffer[32];
  BOOLEAN DiskFound;
  NTSTATUS Status;

  DPRINT("ScsiDiskSearchForDiskData() called\n");

  DiskFound = FALSE;

  /* Enumerate buses */
  for (BusNumber = 0; ; BusNumber++)
    {
      /* Open bus instance subkey */
      swprintf(Buffer,
	       L"%lu",
	       BusNumber);

      RtlInitUnicodeString(&NameString,
			   Buffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &NameString,
				 OBJ_CASE_INSENSITIVE,
				 BusKey,
				 NULL);

      Status = ZwOpenKey(&BusInstanceKey,
			 KEY_READ,
			 &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  break;
	}

      /* Open 'DiskController' subkey */
      RtlInitUnicodeString(&NameString,
			   L"DiskController");

      InitializeObjectAttributes(&ObjectAttributes,
				 &NameString,
				 OBJ_CASE_INSENSITIVE,
				 BusInstanceKey,
				 NULL);

      Status = ZwOpenKey(&ControllerKey,
			 KEY_READ,
			 &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  ZwClose(BusInstanceKey);
	  continue;
	}

      /* Enumerate controllers */
      for (ControllerNumber = 0; ; ControllerNumber++)
	{
	  /* Open 'DiskPeripheral' subkey */
	  swprintf(Buffer,
		   L"%lu\\DiskPeripheral",
		   ControllerNumber);

	  RtlInitUnicodeString(&NameString,
			       Buffer);

	  InitializeObjectAttributes(&ObjectAttributes,
				     &NameString,
				     OBJ_CASE_INSENSITIVE,
				     ControllerKey,
				     NULL);

	  Status = ZwOpenKey(&DiskKey,
			     KEY_READ,
			     &ObjectAttributes);
	  if (!NT_SUCCESS(Status))
	    {
	      break;
	    }

	  /* Enumerate disks */
	  for (DiskNumber = 0; ; DiskNumber++)
	    {
	      /* Open disk instance subkey */
	      swprintf(Buffer,
		       L"%lu",
		       DiskNumber);

	      RtlInitUnicodeString(&NameString,
				   Buffer);

	      InitializeObjectAttributes(&ObjectAttributes,
					 &NameString,
					 OBJ_CASE_INSENSITIVE,
					 DiskKey,
					 NULL);

	      Status = ZwOpenKey(&DiskInstanceKey,
				 KEY_READ,
				 &ObjectAttributes);
	      if (!NT_SUCCESS(Status))
		{
		  break;
		}

	      DPRINT("Found disk key: bus %lu  controller %lu  disk %lu\n",
		     BusNumber,
		     ControllerNumber,
		     DiskNumber);

	      /* Allocate data buffer */
	      ValueData = ExAllocatePool(PagedPool,
					 2048);
	      if (ValueData == NULL)
		{
		  ZwClose(DiskInstanceKey);
		  continue;
		}

	      /* Get the 'Identifier' value */
	      RtlInitUnicodeString(&NameString,
				   L"Identifier");
	      Status = ZwQueryValueKey(DiskInstanceKey,
				       &NameString,
				       KeyValueFullInformation,
				       ValueData,
				       2048,
				       &Length);

	      ZwClose(DiskInstanceKey);
	      if (!NT_SUCCESS(Status))
		{
		  ExFreePool(ValueData);
		  continue;
		}

	      IdentifierString.Buffer =
		(PWSTR)((PUCHAR)ValueData + ValueData->DataOffset);
	      IdentifierString.Length = (USHORT)ValueData->DataLength - 2;
	      IdentifierString.MaximumLength = (USHORT)ValueData->DataLength;

	      DPRINT("DiskIdentifier: %wZ\n",
		     &IdentifierString);

	      DiskData = (PDISK_DATA)(DeviceExtension + 1);
	      if (DiskData->Signature != 0)
		{
		  /* Comapre disk signature */
		  swprintf(Buffer,
			   L"%08lx",
			   DiskData->Signature);
		  if (!_wcsnicmp(Buffer, &IdentifierString.Buffer[9], 8))
		    {
		      DPRINT("Found disk %lu\n", DiskNumber);
		      DiskFound = TRUE;
		      *DetectedDiskNumber = DiskNumber;
		    }
		}
	      else
		{
		  /* Comapre mbr checksum */
		  swprintf(Buffer,
			   L"%08lx",
			   DiskData->MbrCheckSum);
		  if (!_wcsnicmp(Buffer, &IdentifierString.Buffer[0], 8))
		    {
		      DPRINT("Found disk %lu\n", DiskNumber);
		      DiskFound = TRUE;
		      *DetectedDiskNumber = DiskNumber;
		    }
		}

	      ExFreePool(ValueData);

	      ZwClose(DiskInstanceKey);

	      if (DiskFound == TRUE)
		break;
	    }

	  ZwClose(DiskKey);
	}

      ZwClose(ControllerKey);
      ZwClose(BusInstanceKey);
    }

  DPRINT("ScsiDiskSearchForDisk() done\n");

  return DiskFound;
}


/**********************************************************************
 * NAME							INTERNAL
 *	DiskClassUpdateFixedDiskGeometry
 *
 * DESCRIPTION
 *	Updated the geometry of a disk if the disk can be accessed
 *	by the BIOS.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Disk device extension.
 *
 * RETURN VALUE
 *	None
 */

static VOID
ScsiDiskUpdateFixedDiskGeometry(IN PDEVICE_EXTENSION DeviceExtension)
{
  PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
  PCM_INT13_DRIVE_PARAMETER DriveParameters;
  PKEY_VALUE_FULL_INFORMATION ValueBuffer;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE SystemKey;
  HANDLE BusKey;
  ULONG DiskNumber;
  ULONG Length;
  ULONG i;
  ULONG Cylinders;
  ULONG Sectors;
  ULONG SectorsPerTrack;
  ULONG TracksPerCylinder;
  NTSTATUS Status;

  DPRINT("ScsiDiskUpdateFixedDiskGeometry() called\n");

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\Hardware\\Description\\System");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  /* Open the adapter key */
  Status = ZwOpenKey(&SystemKey,
		     KEY_READ,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwOpenKey() failed (Status %lx)\n", Status);
      return;
    }

  /* Allocate value buffer */
  ValueBuffer = ExAllocatePool(PagedPool,
			       1024);
  if (ValueBuffer == NULL)
    {
      DPRINT1("Failed to allocate value buffer\n");
      ZwClose(SystemKey);
      return;
    }

  RtlInitUnicodeString(&ValueName,
		       L"Configuration Data");

  /* Query 'Configuration Data' value */
  Status = ZwQueryValueKey(SystemKey,
			   &ValueName,
			   KeyValueFullInformation,
			   ValueBuffer,
			   1024,
			   &Length);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwQueryValueKey() failed (Status %lx)\n", Status);
      ExFreePool(ValueBuffer);
      ZwClose(SystemKey);
      return;
    }

  /* Open the 'MultifunctionAdapter' subkey */
  RtlInitUnicodeString(&KeyName,
		       L"MultifunctionAdapter");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     SystemKey,
			     NULL);

  Status = ZwOpenKey(&BusKey,
		     KEY_READ,
		     &ObjectAttributes);
  ZwClose(SystemKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwQueryValueKey() failed (Status %lx)\n", Status);
      ExFreePool(ValueBuffer);
      return;
    }

  if (!ScsiDiskSearchForDisk(DeviceExtension, BusKey, &DiskNumber))
    {
      DPRINT1("ScsiDiskSearchForDisk() failed\n");
      ZwClose(BusKey);
      ExFreePool(ValueBuffer);
      return;
    }

  ZwClose(BusKey);

  ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
    ((PUCHAR)ValueBuffer + ValueBuffer->DataOffset);

  DriveParameters = (PCM_INT13_DRIVE_PARAMETER)
    ((PUCHAR)ResourceDescriptor + sizeof(CM_FULL_RESOURCE_DESCRIPTOR));

#if 0
  for (i = 0; i< DriveParameters[0].NumberDrives; i++)
    {
      DPRINT1("Drive %lu: %lu Cylinders  %hu Heads  %hu Sectors\n",
	      i,
	      DriveParameters[i].MaxCylinders,
	      DriveParameters[i].MaxHeads,
	      DriveParameters[i].SectorsPerTrack);
    }
#endif

  Cylinders = DriveParameters[DiskNumber].MaxCylinders + 1;
  TracksPerCylinder = DriveParameters[DiskNumber].MaxHeads +1;
  SectorsPerTrack = DriveParameters[DiskNumber].SectorsPerTrack;

  DPRINT1("BIOS geometry: %lu Cylinders  %hu Heads  %hu Sectors\n",
	  Cylinders,
	  TracksPerCylinder,
	  SectorsPerTrack);

  Sectors = (ULONG)
    (DeviceExtension->PartitionLength.QuadPart >> DeviceExtension->SectorShift);

  DPRINT("Physical sectors: %lu\n",
	 Sectors);

  Length = TracksPerCylinder * SectorsPerTrack;
  if (Length == 0)
    {
      DPRINT1("Invalid track length 0\n");
      ExFreePool(ValueBuffer);
      return;
    }

  Cylinders = Sectors / Length;

  DPRINT1("Logical geometry: %lu Cylinders  %hu Heads  %hu Sectors\n",
	  Cylinders,
	  TracksPerCylinder,
	  SectorsPerTrack);

  /* Update the disk geometry */
  DeviceExtension->DiskGeometry->SectorsPerTrack = SectorsPerTrack;
  DeviceExtension->DiskGeometry->TracksPerCylinder = TracksPerCylinder;
  DeviceExtension->DiskGeometry->Cylinders.QuadPart = (ULONGLONG)Cylinders;

  if (DeviceExtension->DMActive)
    {
      DPRINT1("FIXME: Update geometry with respect to the installed disk manager!\n");

      /* FIXME: Update geometry for disk managers */

    }

  ExFreePool(ValueBuffer);

  DPRINT("ScsiDiskUpdateFixedDiskGeometry() done\n");
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiDiskCalcMbrCheckSum
 *
 * DESCRIPTION
 *	Calculates the Checksum from drives MBR.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Disk device extension.
 *
 *	Checksum
 *		Pointer to the caller supplied cecksum variable.
 *
 * RETURN VALUE
 *	TRUE: Checksum was calculated.
 *	FALSE: Calculation failed.
 */

static BOOLEAN
ScsiDiskCalcMbrCheckSum(IN PDEVICE_EXTENSION DeviceExtension,
			OUT PULONG Checksum)
{
  IO_STATUS_BLOCK IoStatusBlock;
  LARGE_INTEGER SectorOffset;
  ULONG SectorSize;
  PULONG MbrBuffer;
  KEVENT Event;
  PIRP Irp;
  ULONG i;
  ULONG Sum;
  NTSTATUS Status;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  /* Get the disk sector size */
  SectorSize = DeviceExtension->DiskGeometry->BytesPerSector;
  if (SectorSize < 512)
    {
      SectorSize = 512;
    }

  /* Allocate MBR buffer */
  MbrBuffer = ExAllocatePool(NonPagedPool,
			     SectorSize);
  if (MbrBuffer == NULL)
    {
      return FALSE;
    }

  /* Allocate an IRP */
  SectorOffset.QuadPart = 0ULL;
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     DeviceExtension->DeviceObject,
				     MbrBuffer,
				     SectorSize,
				     &SectorOffset,
				     &Event,
				     &IoStatusBlock);
  if (Irp == NULL)
    {
      ExFreePool(MbrBuffer);
      return FALSE;
    }

  /* Call the miniport driver */
  Status = IoCallDriver(DeviceExtension->DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Suspended,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = IoStatusBlock.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      ExFreePool(MbrBuffer);
      return FALSE;
    }

  /* Calculate MBR checksum */
  Sum = 0;
  for (i = 0; i < 128; i++)
    {
      Sum += MbrBuffer[i];
    }
  *Checksum = ~Sum + 1;

  ExFreePool(MbrBuffer);

  return TRUE;
}

/* EOF */
