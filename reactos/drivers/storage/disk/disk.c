/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
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
/* $Id: disk.c,v 1.19 2002/09/19 16:18:14 ekohl Exp $
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



/* FUNCTIONS ****************************************************************/

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver, locates and claims 
//    hardware resources, and creates various NT objects needed
//    to process I/O requests.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS

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
 * NAME							EXPORTED
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
  DiskDeviceExtension->PortDeviceObject = PortDeviceObject;
  DiskDeviceExtension->PhysicalDevice = DiskDeviceObject;
  DiskDeviceExtension->PortCapabilities = Capabilities;
  DiskDeviceExtension->StartingOffset.QuadPart = 0;
  DiskDeviceExtension->PortNumber = (UCHAR)PortNumber;
  DiskDeviceExtension->PathId = InquiryData->PathId;
  DiskDeviceExtension->TargetId = InquiryData->TargetId;
  DiskDeviceExtension->Lun = InquiryData->Lun;

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
	      PartitionDeviceExtension->PortDeviceObject = PortDeviceObject;
	      PartitionDeviceExtension->DiskGeometry = DiskDeviceExtension->DiskGeometry;
	      PartitionDeviceExtension->PhysicalDevice = DiskDeviceExtension->PhysicalDevice;
	      PartitionDeviceExtension->PortCapabilities = Capabilities;
	      PartitionDeviceExtension->StartingOffset.QuadPart =
		PartitionEntry->StartingOffset.QuadPart;
	      PartitionDeviceExtension->PartitionLength.QuadPart =
		PartitionEntry->PartitionLength.QuadPart;
	      PartitionDeviceExtension->PortNumber = (UCHAR)PortNumber;
	      PartitionDeviceExtension->PathId = InquiryData->PathId;
	      PartitionDeviceExtension->TargetId = InquiryData->TargetId;
	      PartitionDeviceExtension->Lun = InquiryData->Lun;
	      PartitionDeviceExtension->SectorShift = DiskDeviceExtension->SectorShift;

	      DiskData = (PDISK_DATA)(PartitionDeviceExtension + 1);
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
		Status = IoWritePartitionTable(DeviceExtension->DeviceObject,
					       DeviceExtension->DiskGeometry->BytesPerSector,
					       DeviceExtension->DiskGeometry->SectorsPerTrack,
					       DeviceExtension->DiskGeometry->TracksPerCylinder,
					       PartitionList);
		if (NT_SUCCESS(Status))
		  {
		    /* FIXME: Update partition device objects */

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


  /* FIXME: Flush write cache */


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

/* EOF */
