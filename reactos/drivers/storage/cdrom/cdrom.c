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
/* $Id: cdrom.c,v 1.20 2003/07/13 12:40:15 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/cdrom/cdrom.c
 * PURPOSE:         cdrom class driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/*
 * TODO:
 *  - Add io timer routine for autorun support.
 *  - Add cdaudio support (cd player).
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/scsi.h>
#include <ddk/class2.h>
#include <ddk/ntddscsi.h>

#define NDEBUG
#include <debug.h>

#define VERSION "0.0.1"


#define SCSI_CDROM_TIMEOUT 10		/* Default timeout: 10 seconds */


typedef struct _ERROR_RECOVERY_DATA
{
  MODE_PARAMETER_HEADER Header;
  MODE_PARAMETER_BLOCK BlockDescriptor;
  MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA, *PERROR_RECOVERY_DATA;


typedef struct _ERROR_RECOVERY_DATA10
{
  MODE_PARAMETER_HEADER10 Header10;
  MODE_PARAMETER_BLOCK BlockDescriptor10;
  MODE_READ_RECOVERY_PAGE ReadRecoveryPage10;
} ERROR_RECOVERY_DATA10, *PERROR_RECOVERY_DATA10;


typedef struct _CDROM_DATA
{
  BOOLEAN PlayActive;
  BOOLEAN RawAccess;
  USHORT XaFlags;

  union
    {
      ERROR_RECOVERY_DATA;
      ERROR_RECOVERY_DATA10;
    };

} CDROM_DATA, *PCDROM_DATA;

/* CDROM_DATA.XaFlags */
#define XA_USE_6_BYTE		0x0001
#define XA_USE_10_BYTE		0x0002
#define XA_USE_READ_CD		0x0004
#define XA_NOT_SUPPORTED	0x0008


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

static VOID
CdromClassCreateMediaChangeEvent(IN PDEVICE_EXTENSION DeviceExtension,
				 IN ULONG DeviceNumber);

static NTSTATUS
CdromClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath,
			     IN PDEVICE_OBJECT PortDeviceObject,
			     IN ULONG PortNumber,
			     IN ULONG DeviceNumber,
			     IN PIO_SCSI_CAPABILITIES Capabilities,
			     IN PSCSI_INQUIRY_DATA InquiryData,
			     IN PCLASS_INIT_DATA InitializationData);


NTSTATUS STDCALL
CdromClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp);

VOID STDCALL
CdromClassStartIo (IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

VOID STDCALL
CdromTimerRoutine(IN PDEVICE_OBJECT DeviceObject,
		  IN PVOID Context);


/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION:
 *	This function initializes the driver, locates and claims 
 *	hardware resources, and creates various NT objects needed
 *	to process I/O requests.
 *
 * RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS:
 *	DriverObject
 *		System allocated Driver Object for this driver
 *	RegistryPath
 *		Name of registry driver service key
 *
 * RETURNS:
 *	Status.
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  CLASS_INIT_DATA InitData;

  DPRINT("CD-ROM Class Driver %s\n",
	 VERSION);
  DPRINT("RegistryPath '%wZ'\n",
	 RegistryPath);

  InitData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
  InitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION) + sizeof(CDROM_DATA);
  InitData.DeviceType = FILE_DEVICE_CD_ROM;
  InitData.DeviceCharacteristics = FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE;

  InitData.ClassError = NULL;
  InitData.ClassReadWriteVerification = CdromClassCheckReadWrite;
  InitData.ClassFindDeviceCallBack = CdromClassCheckDevice;
  InitData.ClassFindDevices = CdromClassFindDevices;
  InitData.ClassDeviceControl = CdromClassDeviceControl;
  InitData.ClassShutdownFlush = NULL;
  InitData.ClassCreateClose = NULL;
  InitData.ClassStartIo = CdromClassStartIo;

  return(ScsiClassInitialize(DriverObject,
			     RegistryPath,
			     &InitData));
}


/**********************************************************************
 * NAME							EXPORTED
 *	CdromClassFindDevices
 *
 * DESCRIPTION:
 *	This function searches for device that are attached to the
 *	given scsi port.
 *
 * RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS:
 *	DriverObject
 *		System allocated Driver Object for this driver
 *	RegistryPath
 *		Name of registry driver service key.
 *	InitializationData
 *		Pointer to the main initialization data
 *	PortDeviceObject
 *		Scsi port device object
 *	PortNumber
 *		Port number
 *
 * RETURNS:
 *	TRUE: At least one disk drive was found
 *	FALSE: No disk drive found
 */

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

  DPRINT("PortCapabilities: %p\n", PortCapabilities);
  DPRINT("MaximumTransferLength: %lu\n", PortCapabilities->MaximumTransferLength);
  DPRINT("MaximumPhysicalPages: %lu\n", PortCapabilities->MaximumPhysicalPages);

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
      DPRINT("No unclaimed devices!\n");
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

  DPRINT("CdromClassFindDevices() done\n");

  return(FoundDevice);
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


static VOID
CdromClassCreateMediaChangeEvent(IN PDEVICE_EXTENSION DeviceExtension,
				 IN ULONG DeviceNumber)
{
  WCHAR NameBuffer[MAX_PATH];
  UNICODE_STRING Name;

  swprintf (NameBuffer,
	    L"\\Device\\MediaChangeEvent%lu",
	    DeviceNumber);
  RtlInitUnicodeString (&Name,
			NameBuffer);

  DeviceExtension->MediaChangeEvent =
    IoCreateSynchronizationEvent (&Name,
				  &DeviceExtension->MediaChangeEventHandle);

  KeClearEvent (DeviceExtension->MediaChangeEvent);
}


/**********************************************************************
 * NAME							EXPORTED
 *	CdromClassCreateDeviceObject
 *
 * DESCRIPTION:
 *	Create the raw device and any partition devices on this drive
 *
 * RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS:
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *	RegistryPath
 *		Name of registry driver service key.
 *	PortDeviceObject
 *	PortNumber
 *	DeviceNumber
 *	Capabilities
 *	InquiryData
 *	InitializationData
 *
 * RETURNS:
 *	Status.
 */

static NTSTATUS
CdromClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath,
			     IN PDEVICE_OBJECT PortDeviceObject,
			     IN ULONG PortNumber,
			     IN ULONG DeviceNumber,
			     IN PIO_SCSI_CAPABILITIES Capabilities,
			     IN PSCSI_INQUIRY_DATA InquiryData,
			     IN PCLASS_INIT_DATA InitializationData)
{
  PDEVICE_EXTENSION DiskDeviceExtension; /* defined in class2.h */
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeDeviceDirName;
  PDEVICE_OBJECT DiskDeviceObject;
  SCSI_REQUEST_BLOCK Srb;
  PCDROM_DATA CdromData;
  CHAR NameBuffer[80];
  HANDLE Handle;
  PUCHAR Buffer;
  ULONG Length;
  PCDB Cdb;
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
  DiskDeviceExtension->PortCapabilities = Capabilities;
  DiskDeviceExtension->StartingOffset.QuadPart = 0;
  DiskDeviceExtension->PortNumber = (UCHAR)PortNumber;
  DiskDeviceExtension->PathId = InquiryData->PathId;
  DiskDeviceExtension->TargetId = InquiryData->TargetId;
  DiskDeviceExtension->Lun = InquiryData->Lun;

  /* zero-out disk data */
  CdromData = (PCDROM_DATA)(DiskDeviceExtension + 1);
  RtlZeroMemory(CdromData,
		sizeof(CDROM_DATA));

  DiskDeviceExtension->SenseData = ExAllocatePool(NonPagedPool,
						  sizeof(SENSE_DATA));
  if (DiskDeviceExtension->SenseData == NULL)
    {
      DPRINT1("Failed to allocate sense data buffer!\n");

      IoDeleteDevice(DiskDeviceObject);

      /* Release (unclaim) the disk */
      ScsiClassClaimDevice(PortDeviceObject,
			   InquiryData,
			   TRUE,
			   NULL);

      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Get timeout value */
  DiskDeviceExtension->TimeOutValue =
    ScsiClassQueryTimeOutRegistryValue(RegistryPath);
  if (DiskDeviceExtension->TimeOutValue == 0)
    DiskDeviceExtension->TimeOutValue = SCSI_CDROM_TIMEOUT;

  /* Initialize lookaside list for SRBs */
  ScsiClassInitializeSrbLookasideList(DiskDeviceExtension,
				      4);

  /* Get disk geometry */
  DiskDeviceExtension->DiskGeometry = ExAllocatePool(NonPagedPool,
						     sizeof(DISK_GEOMETRY));
  if (DiskDeviceExtension->DiskGeometry == NULL)
    {
      DPRINT1("Failed to allocate geometry buffer!\n");

      ExDeleteNPagedLookasideList(&DiskDeviceExtension->SrbLookasideListHead);

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
      DiskDeviceExtension->DiskGeometry->MediaType = RemovableMedia;
      DiskDeviceExtension->SectorShift = 11;
      DiskDeviceExtension->PartitionLength.QuadPart = (ULONGLONG)0x7fffffff;
    }
  else
    {
      /* Make sure the BytesPerSector value is a power of 2 */
//      DiskDeviceExtension->DiskGeometry->BytesPerSector = 2048;
    }

  DPRINT("SectorSize: %lu\n", DiskDeviceExtension->DiskGeometry->BytesPerSector);

  /* Initialize media change support */
  CdromClassCreateMediaChangeEvent (DiskDeviceExtension,
				    DeviceNumber);
  if (DiskDeviceExtension->MediaChangeEvent != NULL)
    {
      DPRINT("Allocated media change event!\n");

      /* FIXME: Allocate media change IRP and SRB */
    }

  /* Use 6 byte xa commands by default */
  CdromData->XaFlags |= XA_USE_6_BYTE;

  /* Read 'error recovery page' to get additional drive capabilities */
  Length = sizeof(MODE_READ_RECOVERY_PAGE) +
	   MODE_BLOCK_DESC_LENGTH +
	   MODE_HEADER_LENGTH;

  RtlZeroMemory (&Srb,
		 sizeof(SCSI_REQUEST_BLOCK));
  Srb.CdbLength = 6;
  Srb.TimeOutValue = DiskDeviceExtension->TimeOutValue;

  Cdb = (PCDB)Srb.Cdb;
  Cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  Cdb->MODE_SENSE.PageCode = 0x01;
  Cdb->MODE_SENSE.AllocationLength = (UCHAR)Length;

  Buffer = ExAllocatePool (NonPagedPool,
			   sizeof(MODE_READ_RECOVERY_PAGE) +
			     MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH10);
  if (Buffer == NULL)
    {
      DPRINT1("Allocating recovery page buffer failed!\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = ScsiClassSendSrbSynchronous (DiskDeviceObject,
					&Srb,
					Buffer,
					Length,
					FALSE);
  if (!NT_SUCCESS (Status))
    {
      DPRINT("MODE_SENSE(6) failed\n");

      /* Try the 10 byte version */
      Length = sizeof(MODE_READ_RECOVERY_PAGE) +
	       MODE_BLOCK_DESC_LENGTH +
	       MODE_HEADER_LENGTH10;

      RtlZeroMemory (&Srb,
		     sizeof(SCSI_REQUEST_BLOCK));
      Srb.CdbLength = 10;
      Srb.TimeOutValue = DiskDeviceExtension->TimeOutValue;

      Cdb = (PCDB)Srb.Cdb;
      Cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
      Cdb->MODE_SENSE10.PageCode = 0x01;
      Cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(Length >> 8);
      Cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(Length && 0xFF);

      Status = ScsiClassSendSrbSynchronous (DiskDeviceObject,
					    &Srb,
					    Buffer,
					    Length,
					    FALSE);
      if (Status == STATUS_DATA_OVERRUN)
	{
	  DPRINT1("Data overrun\n");

	  /* FIXME */
	}
      else if (NT_SUCCESS (Status))
	{
	  DPRINT("Use 10 byte commands\n");
	  RtlCopyMemory (&CdromData->Header,
			 Buffer,
			 sizeof (ERROR_RECOVERY_DATA10));
	  CdromData->Header.ModeDataLength = 0;

	  CdromData->XaFlags &= XA_USE_6_BYTE;
	  CdromData->XaFlags |= XA_USE_10_BYTE;
	}
      else
	{
	  DPRINT("XA not supported\n");
	  CdromData->XaFlags |= XA_NOT_SUPPORTED;
	}
    }
  else
    {
      DPRINT("Use 6 byte commands\n");
      RtlCopyMemory (&CdromData->Header,
		     Buffer,
		     sizeof (ERROR_RECOVERY_DATA));
      CdromData->Header.ModeDataLength = 0;
    }
  ExFreePool (Buffer);

  /* Initialize device timer */
  IoInitializeTimer(DiskDeviceObject,
		    CdromTimerRoutine,
		    NULL);
  IoStartTimer(DiskDeviceObject);

  DPRINT("CdromClassCreateDeviceObjects() done\n");

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME
 *	CdromClassReadTocEntry
 *
 * ARGUMENTS:
 *      DeviceObject
 *      TrackNo
 *      Buffer
 *      Length
 *
 * RETURNS:
 *	Status.
 */

static NTSTATUS
CdromClassReadTocEntry (PDEVICE_OBJECT DeviceObject,
			UINT TrackNo,
			PVOID Buffer,
			UINT Length)
{
  PDEVICE_EXTENSION DeviceExtension;
  SCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

  RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));
  Srb.CdbLength = 10;
  Srb.TimeOutValue = DeviceExtension->TimeOutValue;

  Cdb = (PCDB)Srb.Cdb;
  Cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;
  Cdb->READ_TOC.StartingTrack = TrackNo;
  Cdb->READ_TOC.Format = 0;
  Cdb->READ_TOC.AllocationLength[0] = Length >> 8;
  Cdb->READ_TOC.AllocationLength[1] = Length & 0xff;
  Cdb->READ_TOC.Msf = 1;

  return(ScsiClassSendSrbSynchronous(DeviceObject,
				     &Srb,
				     Buffer,
				     Length,
				     FALSE));
}


static NTSTATUS
CdromClassReadLastSession (PDEVICE_OBJECT DeviceObject,
			   UINT TrackNo,
			   PVOID Buffer,
			   UINT Length)
{
  PDEVICE_EXTENSION DeviceExtension;
  SCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

  RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));
  Srb.CdbLength = 10;
  Srb.TimeOutValue = DeviceExtension->TimeOutValue;

  Cdb = (PCDB)Srb.Cdb;
  Cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;
  Cdb->READ_TOC.StartingTrack = TrackNo;
  Cdb->READ_TOC.Format = 1;
  Cdb->READ_TOC.AllocationLength[0] = Length >> 8;
  Cdb->READ_TOC.AllocationLength[1] = Length & 0xff;
  Cdb->READ_TOC.Msf = 0;

  return(ScsiClassSendSrbSynchronous(DeviceObject,
				     &Srb,
				     Buffer,
				     Length,
				     FALSE));
}


/**********************************************************************
 * NAME							EXPORTED
 *	CdromClassDeviceControl
 *
 * DESCRIPTION:
 *	Answer requests for device control calls
 *
 * RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS:
 *	DeviceObject
 *	Irp
 *		Standard dispatch arguments
 *
 * RETURNS:
 *	Status.
 */

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
      case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
	DPRINT("IOCTL_CDROM_GET_DRIVE_GEOMETRY\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
	  {
	    Status = STATUS_INVALID_PARAMETER;
	  }
	else
	  {
	    PDISK_GEOMETRY Geometry;

	    if (DeviceExtension->DiskGeometry == NULL)
	      {
		DPRINT("No cdrom geometry available!\n");
		DeviceExtension->DiskGeometry = ExAllocatePool(NonPagedPool,
							       sizeof(DISK_GEOMETRY));
	      }
	    Status = ScsiClassReadDriveCapacity(DeviceObject);
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

      case IOCTL_CDROM_READ_TOC:
	DPRINT("IOCTL_CDROM_READ_TOC\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CDROM_TOC))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else
	  {
	    PCDROM_TOC TocBuffer;
	    USHORT Length;

	    TocBuffer = Irp->AssociatedIrp.SystemBuffer;

	    /* First read the lead out */
	    Length = 4 + sizeof(TRACK_DATA);
	    Status = CdromClassReadTocEntry(DeviceObject,
					    0xAA,
					    TocBuffer,
					    Length);
	    if (NT_SUCCESS(Status))
	      {
		if (TocBuffer->FirstTrack == 0xaa)
		  {
		    /* there is an empty cd */
		    Information = Length;
		  }
		else
		  {
		    /* read the toc */
		    Length = 4 + sizeof(TRACK_DATA) * (TocBuffer->LastTrack - TocBuffer->FirstTrack + 2);
		    Status = CdromClassReadTocEntry(DeviceObject,
						    TocBuffer->FirstTrack,
						    TocBuffer, Length);
		    if (NT_SUCCESS(Status))
		      {
			Information = Length;
		      }
		  }
	      }
	  }
	break;

      case IOCTL_CDROM_GET_LAST_SESSION:
	DPRINT("IOCTL_CDROM_GET_LAST_SESSION\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < 4 + sizeof(TRACK_DATA))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else
	  {
	    PCDROM_TOC TocBuffer;
	    USHORT Length;

	    TocBuffer = Irp->AssociatedIrp.SystemBuffer;
	    Length = 4 + sizeof(TRACK_DATA);
	    Status = CdromClassReadLastSession(DeviceObject,
					       0,
					       TocBuffer,
					       Length);
	    if (NT_SUCCESS(Status))
	      {
		Information = Length;
	      }
	  }
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

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME
 *	CdromClassStartIo
 *
 * DESCRIPTION:
 *	Starts IRP processing.
 *
 * RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS:
 *	DeviceObject
 *	Irp
 *		Standard dispatch arguments
 *
 * RETURNS:
 *	None.
 */
VOID STDCALL
CdromClassStartIo (IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  ULONG MaximumTransferLength;
  ULONG TransferPages;

  DPRINT("CdromClassStartIo() called!\n");

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IrpStack = IoGetCurrentIrpStackLocation (Irp);

  MaximumTransferLength = DeviceExtension->PortCapabilities->MaximumTransferLength;

  if (IrpStack->MajorFunction == IRP_MJ_READ)
    {
      DPRINT("  IRP_MJ_READ\n");

      TransferPages =
	ADDRESS_AND_SIZE_TO_SPAN_PAGES (MmGetMdlVirtualAddress(Irp->MdlAddress),
					IrpStack->Parameters.Read.Length);

      /* Check transfer size */
      if ((IrpStack->Parameters.Read.Length > MaximumTransferLength) ||
	  (TransferPages > DeviceExtension->PortCapabilities->MaximumPhysicalPages))
	{
	  /* Transfer size is too large - split it */
	  TransferPages =
	    DeviceExtension->PortCapabilities->MaximumPhysicalPages - 1;

	  /* Adjust transfer size */
	  if (MaximumTransferLength > TransferPages * PAGE_SIZE)
	    MaximumTransferLength = TransferPages * PAGE_SIZE;

	  if (MaximumTransferLength == 0)
	    MaximumTransferLength = PAGE_SIZE;

	  /* Split the transfer */
	  ScsiClassSplitRequest (DeviceObject,
				 Irp,
				 MaximumTransferLength);
	  return;
	}
      else
	{
	  /* Build SRB */
	  ScsiClassBuildRequest (DeviceObject,
				 Irp);
	}
    }
  else if (IrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
    {
      DPRINT1("  IRP_MJ_IRP_MJ_DEVICE_CONTROL\n");

      UNIMPLEMENTED;
#if 0
      switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
	{

	  default:
	    IoCompleteRequest (Irp,
			       IO_NO_INCREMENT);
	    return;
	}
#endif
    }

  /* Call the SCSI port driver */
  IoCallDriver (DeviceExtension->PortDeviceObject,
		Irp);
}


VOID STDCALL
CdromTimerRoutine(PDEVICE_OBJECT DeviceObject,
		  PVOID Context)
{
  DPRINT("CdromTimerRoutine() called\n");

}

/* EOF */
