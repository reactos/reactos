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
/* $Id: cdrom.c,v 1.6 2002/03/22 23:05:44 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/cdrom/cdrom.c
 * PURPOSE:         cdrom class driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include "../include/scsi.h"
#include "../include/class2.h"
#include "../include/ntddscsi.h"

#define NDEBUG
#include <debug.h>

#define VERSION "0.0.1"


typedef struct _CDROM_DATA
{
  ULONG Dummy;
} CDROM_DATA, *PCDROM_DATA;



BOOLEAN STDCALL
CdromClassFindDevices(IN PDRIVER_OBJECT DriverObject,
		      IN PUNICODE_STRING RegistryPath,
		      IN PCLASS_INIT_DATA InitializationData,
		      IN PDEVICE_OBJECT PortDeviceObject,
		      IN ULONG PortNumber);

BOOLEAN STDCALL
CdromClassCheckDevice(IN PINQUIRYDATA InquiryData);

NTSTATUS STDCALL
CdromClassCheckReadWrite(IN PDEVICE_OBJECT DeviceObject,
			 IN PIRP Irp);

static NTSTATUS
CdromClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath, /* what's this used for? */
			     IN PDEVICE_OBJECT PortDeviceObject,
			     IN ULONG PortNumber,
			     IN ULONG DeviceNumber,
			     IN PIO_SCSI_CAPABILITIES Capabilities,
			     IN PSCSI_INQUIRY_DATA InquiryData,
			     IN PCLASS_INIT_DATA InitializationData);


NTSTATUS STDCALL
CdromClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp);

NTSTATUS STDCALL
CdromClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
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

  DbgPrint("CD-ROM Class Driver %s\n",
	   VERSION);
  DPRINT("RegistryPath '%wZ'\n",
	 RegistryPath);

  InitData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
  InitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION) + sizeof(CDROM_DATA);
  InitData.DeviceType = FILE_DEVICE_CD_ROM;
  InitData.DeviceCharacteristics = FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE;

  InitData.ClassError = NULL;				// CdromClassProcessError;
  InitData.ClassReadWriteVerification = CdromClassCheckReadWrite;
  InitData.ClassFindDeviceCallBack = CdromClassCheckDevice;
  InitData.ClassFindDevices = CdromClassFindDevices;
  InitData.ClassDeviceControl = CdromClassDeviceControl;
  InitData.ClassShutdownFlush = CdromClassShutdownFlush;
  InitData.ClassCreateClose = NULL;
  InitData.ClassStartIo = NULL;

  return(ScsiClassInitialize(DriverObject,
			     RegistryPath,
			     &InitData));
}


//    CdromClassFindDevices
//
//  DESCRIPTION:
//    This function searches for device that are attached to the given scsi port.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject        System allocated Driver Object for this driver
//    IN  PUNICODE_STRING  RegistryPath        Name of registry driver service key
//    IN  PCLASS_INIT_DATA InitializationData  Pointer to the main initialization data
//    IN PDEVICE_OBJECT    PortDeviceObject    Scsi port device object
//    IN ULONG             PortNumber          Port number
//
//  RETURNS:
//    TRUE: At least one disk drive was found
//    FALSE: No disk drive found
//

BOOLEAN STDCALL
CdromClassFindDevices(IN PDRIVER_OBJECT DriverObject,
		      IN PUNICODE_STRING RegistryPath,
		      IN PCLASS_INIT_DATA InitializationData,
		      IN PDEVICE_OBJECT PortDeviceObject,
		      IN ULONG PortNumber)
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

  DPRINT("CdromClassFindDevices() called.\n");

  /* Get port capabilities */
  Status = ScsiClassGetCapabilities(PortDeviceObject,
				    &PortCapabilities);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ScsiClassGetCapabilities() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  DPRINT("MaximumTransferLength: %lu\n", PortCapabilities->MaximumTransferLength);

  /* Get inquiry data */
  Status = ScsiClassGetInquiryData(PortDeviceObject,
				   (PSCSI_ADAPTER_BUS_INFO *)&Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ScsiClassGetInquiryData() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  /* Check whether there are unclaimed devices */
  AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;
  DeviceCount = ScsiClassFindUnclaimedDevices(InitializationData,
					      AdapterBusInfo);
  if (DeviceCount == 0)
    {
      DPRINT1("No unclaimed devices!\n");
      return(FALSE);
    }

  DPRINT("Found %lu unclaimed devices!\n", DeviceCount);

  ConfigInfo = IoGetConfigurationInformation();
  DPRINT("Number of SCSI ports: %lu\n", ConfigInfo->ScsiPortCount);

  /* Search each bus of this adapter */
  for (Bus = 0; Bus < (ULONG)AdapterBusInfo->NumberOfBuses; Bus++)
    {
      DPRINT("Searching bus %lu\n", Bus);

      UnitInfo = (PSCSI_INQUIRY_DATA)(Buffer + AdapterBusInfo->BusData[Bus].InquiryDataOffset);

      while (AdapterBusInfo->BusData[Bus].InquiryDataOffset)
	{
	  InquiryData = (PINQUIRYDATA)UnitInfo->InquiryData;

	  if ((InquiryData->DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) &&
	      (InquiryData->DeviceTypeQualifier == 0) &&
	      (UnitInfo->DeviceClaimed == FALSE))
	    {
	      DPRINT("Vendor: '%.24s'\n",
		     InquiryData->VendorId);

	      /* Create device objects for disk */
	      Status = CdromClassCreateDeviceObject(DriverObject,
						    RegistryPath,
						    PortDeviceObject,
						    PortNumber,
						    ConfigInfo->CDRomCount,
						    PortCapabilities,
						    UnitInfo,
						    InitializationData);
	      if (NT_SUCCESS(Status))
		{
		  ConfigInfo->CDRomCount++;
		  FoundDevice = TRUE;
		}
	    }

	  if (UnitInfo->NextInquiryDataOffset == 0)
	    break;

	  UnitInfo = (PSCSI_INQUIRY_DATA)(Buffer + UnitInfo->NextInquiryDataOffset);
	}
    }

  ExFreePool(Buffer);
  ExFreePool(PortCapabilities);

  DPRINT("CdromClassFindDevices() done\n");

  return(TRUE);
}


/**********************************************************************
 * NAME							EXPORTED
 *	CdromClassCheckDevice
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
CdromClassCheckDevice(IN PINQUIRYDATA InquiryData)
{
  return((InquiryData->DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) &&
	 (InquiryData->DeviceTypeQualifier == 0));
}


/**********************************************************************
 * NAME							EXPORTED
 *	CdromClassCheckReadWrite
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
CdromClassCheckReadWrite(IN PDEVICE_OBJECT DeviceObject,
			 IN PIRP Irp)
{
  DPRINT("CdromClassCheckReadWrite() called\n");

  return(STATUS_SUCCESS);
}


//    CdromClassCreateDeviceObject
//
//  DESCRIPTION:
//    Create the raw device and any partition devices on this drive
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT  DriverObject  The system created driver object
//    IN  PCONTROLLER_OBJECT         ControllerObject
//    IN  PIDE_CONTROLLER_EXTENSION  ControllerExtension
//                                      The IDE controller extension for
//                                      this device
//    IN  int             DriveIdx      The index of the drive on this
//                                      controller
//    IN  int             HarddiskIdx   The NT device number for this
//                                      drive
//
//  RETURNS:
//    TRUE   Drive exists and devices were created
//    FALSE  no devices were created for this device
//

static NTSTATUS
CdromClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath, /* what's this used for? */
			     IN PDEVICE_OBJECT PortDeviceObject,
			     IN ULONG PortNumber,
			     IN ULONG DeviceNumber,
			     IN PIO_SCSI_CAPABILITIES Capabilities,
			     IN PSCSI_INQUIRY_DATA InquiryData,
			     IN PCLASS_INIT_DATA InitializationData)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeDeviceDirName;
  CHAR NameBuffer[80];
  PDEVICE_OBJECT DiskDeviceObject;
  PDEVICE_EXTENSION DiskDeviceExtension; /* defined in class2.h */
  HANDLE Handle;
  PCDROM_DATA CdromData;
  NTSTATUS Status;

  DPRINT("CdromClassCreateDeviceObject() called\n");

  /* Claim the cdrom device */
  Status = ScsiClassClaimDevice(PortDeviceObject,
				InquiryData,
				FALSE,
				&PortDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not claim cdrom device\n");
      return(Status);
    }

  /* Create cdrom device */
  sprintf(NameBuffer,
	  "\\Device\\CdRom%lu",
	  DeviceNumber);

  Status = ScsiClassCreateDeviceObject(DriverObject,
				       NameBuffer,
				       NULL,
				       &DiskDeviceObject,
				       InitializationData);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ScsiClassCreateDeviceObject() failed (Status %x)\n", Status);

      /* Release (unclaim) the disk */
      ScsiClassClaimDevice(PortDeviceObject,
			   InquiryData,
			   TRUE,
			   NULL);

      return(Status);
    }

  DiskDeviceObject->Flags |= DO_DIRECT_IO;
  DiskDeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
  DiskDeviceObject->StackSize = (CCHAR)PortDeviceObject->StackSize + 1;

  if (PortDeviceObject->AlignmentRequirement > DiskDeviceObject->AlignmentRequirement)
    {
      DiskDeviceObject->AlignmentRequirement = PortDeviceObject->AlignmentRequirement;
    }

  DiskDeviceExtension = DiskDeviceObject->DeviceExtension;
  DiskDeviceExtension->LockCount = 0;
  DiskDeviceExtension->DeviceNumber = DeviceNumber;
  DiskDeviceExtension->PortDeviceObject = PortDeviceObject;
  DiskDeviceExtension->PhysicalDevice = DiskDeviceObject;

  /* FIXME: Not yet! Will cause pointer corruption! */
//  DiskDeviceExtension->PortCapabilities = PortCapabilities;

  DiskDeviceExtension->StartingOffset.QuadPart = 0;
  DiskDeviceExtension->PortNumber = (UCHAR)PortNumber;
  DiskDeviceExtension->PathId = InquiryData->PathId;
  DiskDeviceExtension->TargetId = InquiryData->TargetId;
  DiskDeviceExtension->Lun = InquiryData->Lun;

  /* zero-out disk data */
  CdromData = (PCDROM_DATA)(DiskDeviceExtension + 1);
  RtlZeroMemory(CdromData,
		sizeof(CDROM_DATA));

  /* Get disk geometry */
  DiskDeviceExtension->DiskGeometry = ExAllocatePool(NonPagedPool,
						     sizeof(DISK_GEOMETRY));
  if (DiskDeviceExtension->DiskGeometry == NULL)
    {
      DPRINT1("Failed to allocate geometry buffer!\n");

      IoDeleteDevice(DiskDeviceObject);

      /* Release (unclaim) the disk */
      ScsiClassClaimDevice(PortDeviceObject,
			   InquiryData,
			   TRUE,
			   NULL);

      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Read the drive's capacity */
  Status = ScsiClassReadDriveCapacity(DiskDeviceObject);
  if (!NT_SUCCESS(Status) ||
      DiskDeviceExtension->DiskGeometry->BytesPerSector == 0)
    {
      /* Set ISO9660 defaults */
      DiskDeviceExtension->DiskGeometry->BytesPerSector = 2048;
      DiskDeviceExtension->SectorShift = 11;
      DiskDeviceExtension->PartitionLength.QuadPart = (ULONGLONG)0x7fffffff;
    }
  else
    {
      /* Make sure the BytesPerSector value is a power of 2 */
//      DiskDeviceExtension->DiskGeometry->BytesPerSector = 2048;
    }

  DPRINT("SectorSize: %lu\n", DiskDeviceExtension->DiskGeometry->BytesPerSector);

  /* FIXME: initialize media change support */

  DPRINT("CdromClassCreateDeviceObjects() done\n");

  return(STATUS_SUCCESS);
}



//    CdromClassDeviceControl
//
//  DESCRIPTION:
//    Answer requests for device control calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS STDCALL
CdromClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  ULONG ControlCode, InputLength, OutputLength;
  PCDROM_DATA CdromData;
  ULONG Information;
  NTSTATUS Status;

  DPRINT("CdromClassDeviceControl() called!\n");

  Status = STATUS_INVALID_DEVICE_REQUEST;
  Information = 0;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
  InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  CdromData = (PCDROM_DATA)(DeviceExtension + 1);

  switch (ControlCode)
    {
      default:
	DPRINT1("Unhandled control code: %lx\n", ControlCode);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	Information = 0;
	break;
    }


  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


//    CdromClassShutdownFlush
//
//  DESCRIPTION:
//    Answer requests for shutdown and flush calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS STDCALL
CdromClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp)
{
  DPRINT("CdromClassShutdownFlush() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/* EOF */
