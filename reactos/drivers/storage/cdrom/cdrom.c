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
/* $Id: cdrom.c,v 1.27 2004/02/29 12:26:09 hbirr Exp $
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
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#define VERSION "0.0.1"


#define SCSI_CDROM_TIMEOUT 10		/* Default timeout: 10 seconds */


typedef struct _ERROR_RECOVERY_DATA6
{
  MODE_PARAMETER_HEADER Header;
  MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA6, *PERROR_RECOVERY_DATA6;


typedef struct _ERROR_RECOVERY_DATA10
{
  MODE_PARAMETER_HEADER10 Header;
  MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA10, *PERROR_RECOVERY_DATA10;

typedef struct _MODE_CAPABILITIES_DATA6
{
  MODE_PARAMETER_HEADER Header;
  MODE_CAPABILITIES_PAGE2 CababilitiesPage;
} MODE_CAPABILITIES_DATA6, *PMODE_CAPABILITIES_DATA6;

typedef struct _MODE_CAPABILITIES_DATA10
{
  MODE_PARAMETER_HEADER10 Header;
  MODE_CAPABILITIES_PAGE2 CababilitiesPage;
} MODE_CAPABILITIES_DATA10, *PMODE_CAPABILITIES_DATA10;

typedef struct _CDROM_DATA
{
  BOOLEAN PlayActive;
  BOOLEAN RawAccess;
  USHORT XaFlags;

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

NTSTATUS STDCALL
CdromDeviceControlCompletion (IN PDEVICE_OBJECT DeviceObject,
			      IN PIRP Irp,
			      IN PVOID Context);

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
						    ConfigInfo->CdRomCount,
						    PortCapabilities,
						    UnitInfo,
						    InitializationData);
	      if (NT_SUCCESS(Status))
		{
		  ConfigInfo->CdRomCount++;
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
  PDEVICE_OBJECT DiskDeviceObject;
  PCDROM_DATA CdromData;
  CHAR NameBuffer[80];
  SCSI_REQUEST_BLOCK Srb;
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
  DiskDeviceExtension->DeviceObject = DiskDeviceObject;
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
  Length = sizeof(MODE_READ_RECOVERY_PAGE) + MODE_HEADER_LENGTH;

  RtlZeroMemory (&Srb,
		 sizeof(SCSI_REQUEST_BLOCK));
  Srb.CdbLength = 6;
  Srb.TimeOutValue = DiskDeviceExtension->TimeOutValue;

  Cdb = (PCDB)Srb.Cdb;
  Cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  Cdb->MODE_SENSE.PageCode = 0x01;
  Cdb->MODE_SENSE.AllocationLength = (UCHAR)Length;

  Buffer = ExAllocatePool (NonPagedPool,
                           max(sizeof(ERROR_RECOVERY_DATA6), 
			       max(sizeof(ERROR_RECOVERY_DATA10), 
			           max(sizeof(MODE_CAPABILITIES_DATA6), 
				       sizeof(MODE_CAPABILITIES_DATA10)))));
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
      Length = sizeof(MODE_READ_RECOVERY_PAGE) + MODE_HEADER_LENGTH10;

      RtlZeroMemory (&Srb, 
		     sizeof(SCSI_REQUEST_BLOCK));
      Srb.CdbLength = 10;
      Srb.TimeOutValue = DiskDeviceExtension->TimeOutValue;

      Cdb = (PCDB)Srb.Cdb;
      Cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
      Cdb->MODE_SENSE10.PageCode = 0x01;
      Cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(Length >> 8);
      Cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(Length & 0xFF);

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
    }

  /* Read 'capabilities & mechanical status page' to get additional drive capabilities */
  Length = sizeof(MODE_READ_RECOVERY_PAGE) + MODE_HEADER_LENGTH;

  if (!(CdromData->XaFlags & XA_NOT_SUPPORTED))
    {
      RtlZeroMemory (&Srb, sizeof(SCSI_REQUEST_BLOCK));
      Srb.CdbLength = 10;
      Srb.TimeOutValue = DiskDeviceExtension->TimeOutValue;
      Cdb = (PCDB)Srb.Cdb;

      if (CdromData->XaFlags & XA_USE_10_BYTE)
        {
          /* Try the 10 byte version */
          Length = sizeof(MODE_CAPABILITIES_PAGE2) + MODE_HEADER_LENGTH10;

          Cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
          Cdb->MODE_SENSE10.PageCode = 0x2a;
          Cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(Length >> 8);
          Cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(Length & 0xFF);
	}
      else
        {
          Length = sizeof(MODE_CAPABILITIES_PAGE2) + MODE_HEADER_LENGTH;

          Cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
          Cdb->MODE_SENSE.PageCode = 0x2a;
          Cdb->MODE_SENSE.AllocationLength = (UCHAR)Length;
        }
      Status = ScsiClassSendSrbSynchronous (DiskDeviceObject,
					    &Srb,
					    Buffer,
					    Length,
					    FALSE);
      if (NT_SUCCESS (Status))
	{
#if 0
          PMODE_CAPABILITIES_PAGE2 CapabilitiesData;
	  if (CdromData->XaFlags & XA_USE_10_BYTE)
	    {
	      CapabilitiesData = (PMODE_CAPABILITIES_PAGE2)(Buffer + sizeof(MODE_PARAMETER_HEADER10));
	    }
	  else
	    {
	      CapabilitiesData = (PMODE_CAPABILITIES_PAGE2)(Buffer + sizeof(MODE_PARAMETER_HEADER));
	    }

	  DbgPrint("Capabilities for '%s':\n", NameBuffer);
	  if (CapabilitiesData->Reserved2[0] & 0x20)
	    {
	      DbgPrint("  Drive supports reading of DVD-RAM discs\n");
	    }
	  if (CapabilitiesData->Reserved2[0] & 0x10)
	    {
	      DbgPrint("  Drive supports reading of DVD-R discs\n");
	    }
	  if (CapabilitiesData->Reserved2[0] & 0x08)
	    {
	      DbgPrint("  Drive supports reading of DVD-ROM discs\n");
	    }
	  if (CapabilitiesData->Reserved2[0] & 0x04)
	    {
	      DbgPrint("  Drive supports reading CD-R discs with addressing method 2\n");
	    }
	  if (CapabilitiesData->Reserved2[0] & 0x02)
	    {
	      DbgPrint("  Drive can read from CD-R/W (CD-E) discs (orange book, part III)\n");
	    }
	  if (CapabilitiesData->Reserved2[0] & 0x01)
	    {
	      DbgPrint("  Drive supports read from CD-R discs (orange book, part II)\n");
	    }
	  DPRINT("CapabilitiesData.Reserved2[1] %x\n", CapabilitiesData->Reserved2[1]);
	  if (CapabilitiesData->Reserved2[1] & 0x01)
	    {
	      DbgPrint("  Drive can write to CD-R discs (orange book, part II)\n");
	    }
	  if (CapabilitiesData->Reserved2[1] & 0x02)
	    {
	      DbgPrint("  Drive can write to CD-R/W (CD-E) discs (orange book, part III)\n");
	    }
	  if (CapabilitiesData->Reserved2[1] & 0x04)
	    {
	      DbgPrint("  Drive can fake writes\n");
	    }
	  if (CapabilitiesData->Reserved2[1] & 0x10)
	    {
	      DbgPrint("  Drive can write DVD-R discs\n");
	    }
	  if (CapabilitiesData->Reserved2[1] & 0x20)
	    {
	      DbgPrint("  Drive can write DVD-RAM discs\n");
	    }
	  DPRINT("CapabilitiesData.Capabilities[0] %x\n", CapabilitiesData->Capabilities[0]);
	  if (CapabilitiesData->Capabilities[0] & 0x01)
	    {
	      DbgPrint("  Drive supports audio play operations\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x02)
	    {
	      DbgPrint("  Drive can deliver a composite audio/video data stream\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x04)
	    {
	      DbgPrint("  Drive supports digital output on port 1\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x08)
	    {
	      DbgPrint("  Drive supports digital output on port 2\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x10)
	    {
	      DbgPrint("  Drive can read mode 2, form 1 (XA) data\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x20)
	    {
	      DbgPrint("  Drive can read mode 2, form 2 data\n");
	    }
	  if (CapabilitiesData->Capabilities[0] & 0x40)
	    {
	      DbgPrint("  Drive can read multisession discs\n");
	    }
	  DPRINT("CapabilitiesData.Capabilities[1] %x\n", CapabilitiesData->Capabilities[1]);
	  if (CapabilitiesData->Capabilities[1] & 0x01)
	    {
	      DbgPrint("  Drive can read Red Book audio data\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x02)
	    {
	      DbgPrint("  Drive can continue a read cdda operation from a loss of streaming\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x04)
	    {
	      DbgPrint("  Subchannel reads can return combined R-W information\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x08)
	    {
	      DbgPrint("  R-W data will be returned deinterleaved and error corrected\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x10)
	    {
	      DbgPrint("  Drive supports C2 error pointers\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x20)
	    {
	      DbgPrint("  Drive can return International Standard Recording Code info\n");
	    }
	  if (CapabilitiesData->Capabilities[1] & 0x40)
	    {
	      DbgPrint("  Drive can return Media Catalog Number (UPC) info\n");
	    }
	  DPRINT("CapabilitiesData.Capabilities[2] %x\n", CapabilitiesData->Capabilities[2]);
	  if (CapabilitiesData->Capabilities[2] & 0x01)
	    {
	      DbgPrint("  Drive can lock the door\n");
	    }
	  if (CapabilitiesData->Capabilities[2] & 0x02)
	    {
	      DbgPrint("  The door is locked\n");
	    }
	  if (CapabilitiesData->Capabilities[2] & 0x04)
	    {
	    }
	  if (CapabilitiesData->Capabilities[2] & 0x08)
	    {
	      DbgPrint("  Drive can eject a disc or changer cartridge\n");
	    }
	  if (CapabilitiesData->Capabilities[2] & 0x10)
	    {
	      DbgPrint("  Drive supports C2 error pointers\n");
	    }
	  switch (CapabilitiesData->Capabilities[2] >> 5)
	    {
	      case 0:
	        DbgPrint("  Drive use a caddy type loading mechanism\n");
		break;
	      case 1:
	        DbgPrint("  Drive use a tray type loading mechanism\n");
		break;
	      case 2:
	        DbgPrint("  Drive use a pop-up type loading mechanism\n");
		break;
	      case 4:
	        DbgPrint("  Drive is a changer with individually changeable discs\n");
		break;
	      case 5:
	        DbgPrint("  Drive is a changer with cartridge mechanism\n");
		break;
	    }
	  DPRINT("CapabilitiesData.Capabilities[3] %x\n", CapabilitiesData->Capabilities[3]);
	  if (CapabilitiesData->Capabilities[3] & 0x01)
	    {
	      DbgPrint("  Audio level for each channel can be controlled independently\n");
	    }
	  if (CapabilitiesData->Capabilities[3] & 0x02)
	    {
	      DbgPrint("  Audio for each channel can be muted independently\n");
	    }
	  if (CapabilitiesData->Capabilities[3] & 0x04)
	    {
	      DbgPrint("  Changer can report exact contents of slots\n");
	    }
	  if (CapabilitiesData->Capabilities[3] & 0x08)
	    {
	      DbgPrint("  Drive supports software slot selection\n");
	    }
	  DbgPrint("  Maximum speed is %d kB/s\n", 
	          (CapabilitiesData->MaximumSpeedSupported[0] << 8) 
		  | CapabilitiesData->MaximumSpeedSupported[1]);
	  DbgPrint("  Current speed is %d kB/s\n", 
	          (CapabilitiesData->CurrentSpeed[0] << 8) 
		  | CapabilitiesData->CurrentSpeed[1]);
	  DbgPrint("  Number of discrete volume levels is %d\n",
	          (CapabilitiesData->Reserved3 << 8) 
		  | CapabilitiesData->NumberVolumeLevels);
	  DbgPrint("  Buffer size is %d kB\n",
	          (CapabilitiesData->BufferSize[0] << 8) 
		  | CapabilitiesData->BufferSize[1]);
#endif
	}
      else
	{
	  DPRINT("XA not supported\n");
	  CdromData->XaFlags |= XA_NOT_SUPPORTED;
	}

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
	DPRINT ("CdromClassDeviceControl: IOCTL_CDROM_GET_DRIVE_GEOMETRY\n");
	if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	    break;
	  }
	IoMarkIrpPending (Irp);
	IoStartPacket (DeviceObject,
		       Irp,
		       NULL,
		       NULL);
	return STATUS_PENDING;

      case IOCTL_CDROM_CHECK_VERIFY:
	DPRINT ("CdromClassDeviceControl: IOCTL_CDROM_CHECK_VERIFY\n");
	if (OutputLength != 0 && OutputLength < sizeof (ULONG))
	  {
	    DPRINT1("Buffer too small\n");
	    Status = STATUS_BUFFER_TOO_SMALL;
	    break;
	  }
	IoMarkIrpPending (Irp);
	IoStartPacket (DeviceObject,
		       Irp,
		       NULL,
		       NULL);
	return STATUS_PENDING;

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
		    IO_DISK_INCREMENT);

  return Status;
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
  PIO_STACK_LOCATION SubIrpStack;
  ULONG MaximumTransferLength;
  ULONG TransferPages;
  PSCSI_REQUEST_BLOCK Srb;
  PIRP SubIrp;
  PUCHAR SenseBuffer;
  PVOID DataBuffer;
  PCDB Cdb;

  DPRINT("CdromClassStartIo() called!\n");

  IoMarkIrpPending (Irp);

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IrpStack = IoGetCurrentIrpStackLocation (Irp);

  MaximumTransferLength = DeviceExtension->PortCapabilities->MaximumTransferLength;

  if (DeviceObject->Flags & DO_VERIFY_VOLUME)
    {
      if (!(IrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME))
	{
	  DPRINT1 ("Verify required\n");

	  if (Irp->Tail.Overlay.Thread)
	    {
	      IoSetHardErrorOrVerifyDevice (Irp,
					    DeviceObject);
	    }
	  Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;

	  /* FIXME: Update drive capacity */

	  return;
	}
    }

  if (IrpStack->MajorFunction == IRP_MJ_READ)
    {
      DPRINT ("CdromClassStartIo: IRP_MJ_READ\n");

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
      DPRINT ("CdromClassStartIo: IRP_MJ_IRP_MJ_DEVICE_CONTROL\n");

      /* Allocate an IRP for sending requests to the port driver */
      SubIrp = IoAllocateIrp ((CCHAR)(DeviceObject->StackSize + 1),
			      FALSE);
      if (SubIrp == NULL)
	{
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	  IoCompleteRequest (Irp,
			     IO_DISK_INCREMENT);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  return;
	}

      /* Allocate an SRB */
      Srb = ExAllocatePool (NonPagedPool,
			    sizeof (SCSI_REQUEST_BLOCK));
      if (Srb == NULL)
	{
	  IoFreeIrp (SubIrp);
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	  IoCompleteRequest (Irp,
			     IO_DISK_INCREMENT);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  return;
	}

      /* Allocte a sense buffer */
      SenseBuffer = ExAllocatePool (NonPagedPoolCacheAligned,
				    SENSE_BUFFER_SIZE);
      if (SenseBuffer == NULL)
	{
	  ExFreePool (Srb);
	  IoFreeIrp (SubIrp);
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	  IoCompleteRequest (Irp,
			     IO_DISK_INCREMENT);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  return;
	}

      /* Initialize the IRP */
      IoSetNextIrpStackLocation (SubIrp);
      SubIrp->IoStatus.Information = 0;
      SubIrp->IoStatus.Status = STATUS_SUCCESS;
      SubIrp->Flags = 0;
      SubIrp->UserBuffer = NULL;

      SubIrpStack = IoGetCurrentIrpStackLocation (SubIrp);
      SubIrpStack->DeviceObject = DeviceExtension->DeviceObject;
      SubIrpStack->Parameters.Others.Argument2 = (PVOID)Irp;

      /* Initialize next stack location */
      SubIrpStack = IoGetNextIrpStackLocation (SubIrp);
      SubIrpStack->MajorFunction = IRP_MJ_SCSI;
      SubIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
      SubIrpStack->Parameters.Scsi.Srb = Srb;

      /* Initialize the SRB */
      RtlZeroMemory(Srb,
		    sizeof (SCSI_REQUEST_BLOCK));
      Srb->Length = sizeof (SCSI_REQUEST_BLOCK);
      Srb->PathId = DeviceExtension->PathId;
      Srb->TargetId = DeviceExtension->TargetId;
      Srb->Lun = DeviceExtension->Lun;
      Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
      Srb->SrbStatus = SRB_STATUS_SUCCESS;
      Srb->ScsiStatus = 0;
      Srb->NextSrb = 0;
      Srb->OriginalRequest = SubIrp;
      Srb->SenseInfoBuffer = SenseBuffer;
      Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

      /* Initialize the CDB */
      Cdb = (PCDB)Srb->Cdb;

      /* Set the completion routine */
      IoSetCompletionRoutine (SubIrp,
			      CdromDeviceControlCompletion,
			      Srb,
			      TRUE,
			      TRUE,
			      TRUE);

      switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	  case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
	    DPRINT ("CdromClassStartIo: IOCTL_CDROM_GET_DRIVE_GEOMETRY\n");
	    Srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);
	    Srb->CdbLength = 10;
	    Srb->TimeOutValue = DeviceExtension->TimeOutValue;
	    Srb->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN;
	    Cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

	    /* Allocate data buffer */
	    DataBuffer = ExAllocatePool (NonPagedPoolCacheAligned,
					 sizeof(READ_CAPACITY_DATA));
	    if (DataBuffer == NULL)
	      {
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest (Irp,
				   IO_DISK_INCREMENT);
		ExFreePool (SenseBuffer);
		ExFreePool (Srb);
		IoFreeIrp (SubIrp);
		IoStartNextPacket (DeviceObject,
				   FALSE);
		return;
	      }

	    /* Allocate an MDL for the data buffer */
	    SubIrp->MdlAddress = IoAllocateMdl (DataBuffer,
						sizeof(READ_CAPACITY_DATA),
						FALSE,
						FALSE,
						NULL);
	    if (SubIrp->MdlAddress == NULL)
	      {
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest (Irp,
				   IO_DISK_INCREMENT);
		ExFreePool (DataBuffer);
		ExFreePool (SenseBuffer);
		ExFreePool (Srb);
		IoFreeIrp (SubIrp);
		IoStartNextPacket (DeviceObject,
				   FALSE);
		return;
	      }

	    MmBuildMdlForNonPagedPool (SubIrp->MdlAddress);
	    Srb->DataBuffer = DataBuffer;

	    IoCallDriver (DeviceExtension->PortDeviceObject,
			  SubIrp);
	    return;

	  case IOCTL_CDROM_CHECK_VERIFY:
	    DPRINT ("CdromClassStartIo: IOCTL_CDROM_CHECK_VERIFY\n");
	    Srb->CdbLength = 6;
	    Srb->TimeOutValue = DeviceExtension->TimeOutValue * 2;
	    Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
	    Cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

	    IoCallDriver (DeviceExtension->PortDeviceObject,
			  SubIrp);
	    return;

	  default:
	    IoCompleteRequest (Irp,
			       IO_NO_INCREMENT);
	    return;
	}
    }

  /* Call the SCSI port driver */
  IoCallDriver (DeviceExtension->PortDeviceObject,
		Irp);
}


NTSTATUS STDCALL
CdromDeviceControlCompletion (IN PDEVICE_OBJECT DeviceObject,
			      IN PIRP Irp,
			      IN PVOID Context)
{
  PDEVICE_EXTENSION DeviceExtension;
  PDEVICE_EXTENSION PhysicalExtension;
  PIO_STACK_LOCATION IrpStack;
  PIO_STACK_LOCATION OrigCurrentIrpStack;
  PIO_STACK_LOCATION OrigNextIrpStack;
  PSCSI_REQUEST_BLOCK Srb;
  PIRP OrigIrp;
  BOOLEAN Retry;
  NTSTATUS Status;

  DPRINT ("CdromDeviceControlCompletion() called\n");

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  PhysicalExtension = (PDEVICE_EXTENSION)DeviceExtension->PhysicalDevice->DeviceExtension;
  Srb = (PSCSI_REQUEST_BLOCK) Context;

  IrpStack = IoGetCurrentIrpStackLocation (Irp);

  /* Get the original IRP */
  OrigIrp = (PIRP)IrpStack->Parameters.Others.Argument2;
  OrigCurrentIrpStack = IoGetCurrentIrpStackLocation (OrigIrp);
  OrigNextIrpStack = IoGetNextIrpStackLocation (OrigIrp);

  if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
      Status = STATUS_SUCCESS;
    }
  else
    {
      DPRINT ("SrbStatus %lx\n", Srb->SrbStatus);

      /* Interpret sense info */
      Retry = ScsiClassInterpretSenseInfo (DeviceObject,
					   Srb,
					   IrpStack->MajorFunction,
					   IrpStack->Parameters.DeviceIoControl.IoControlCode,
					   MAXIMUM_RETRIES - (ULONG)OrigNextIrpStack->Parameters.Others.Argument1,
					   &Status);
      DPRINT ("Retry %u\n", Retry);

      if (Retry == TRUE &&
	  (ULONG)OrigNextIrpStack->Parameters.Others.Argument1 > 0)
	{
	  DPRINT1 ("Try again (Retry count %lu)\n",
		   (ULONG)OrigNextIrpStack->Parameters.Others.Argument1);

	  (ULONG)OrigNextIrpStack->Parameters.Others.Argument1--;

	  /* Release 'old' buffers */
	  ExFreePool (Srb->SenseInfoBuffer);
	  if (Srb->DataBuffer)
	    ExFreePool(Srb->DataBuffer);
	  ExFreePool(Srb);

	  if (Irp->MdlAddress != NULL)
	    IoFreeMdl(Irp->MdlAddress);

	  IoFreeIrp(Irp);

	  /* Call the StartIo routine again */
	  CdromClassStartIo (DeviceObject,
			     OrigIrp);

	  return STATUS_MORE_PROCESSING_REQUIRED;
	}

      DPRINT ("Status %lx\n", Status);
    }

  if (NT_SUCCESS (Status))
    {
      switch (OrigCurrentIrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	  case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
	    {
	      PREAD_CAPACITY_DATA CapacityBuffer;
	      ULONG LastSector;
	      ULONG SectorSize;

	      DPRINT ("CdromClassControlCompletion: IOCTL_CDROM_GET_DRIVE_GEOMETRY\n");

	      CapacityBuffer = (PREAD_CAPACITY_DATA)Srb->DataBuffer;
	      SectorSize = (((PUCHAR)&CapacityBuffer->BytesPerBlock)[0] << 24) |
			   (((PUCHAR)&CapacityBuffer->BytesPerBlock)[1] << 16) |
			   (((PUCHAR)&CapacityBuffer->BytesPerBlock)[2] << 8) |
			    ((PUCHAR)&CapacityBuffer->BytesPerBlock)[3];

	      LastSector = (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[0] << 24) |
			   (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[1] << 16) |
			   (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[2] << 8) |
			    ((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[3];

	      if (SectorSize == 0)
		SectorSize = 2048;
	      DeviceExtension->DiskGeometry->BytesPerSector = SectorSize;

	      DeviceExtension->PartitionLength.QuadPart = (LONGLONG)(LastSector + 1);
	      WHICH_BIT(DeviceExtension->DiskGeometry->BytesPerSector,
			DeviceExtension->SectorShift);
	      DeviceExtension->PartitionLength.QuadPart =
		(DeviceExtension->PartitionLength.QuadPart << DeviceExtension->SectorShift);

	      if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		{
		  DeviceExtension->DiskGeometry->MediaType = RemovableMedia;
		}
	      else
		{
		  DeviceExtension->DiskGeometry->MediaType = FixedMedia;
		}
	      DeviceExtension->DiskGeometry->Cylinders.QuadPart =
		(LONGLONG)((LastSector + 1)/(32 * 64));
	      DeviceExtension->DiskGeometry->SectorsPerTrack = 32;
	      DeviceExtension->DiskGeometry->TracksPerCylinder = 64;

	      RtlCopyMemory (OrigIrp->AssociatedIrp.SystemBuffer,
			     DeviceExtension->DiskGeometry,
			     sizeof(DISK_GEOMETRY));
	      OrigIrp->IoStatus.Information = sizeof(DISK_GEOMETRY);
	    }
	    break;

	  case IOCTL_CDROM_CHECK_VERIFY:
	    DPRINT ("CdromDeviceControlCompletion: IOCTL_CDROM_CHECK_VERIFY\n");
	    if (OrigCurrentIrpStack->Parameters.DeviceIoControl.OutputBufferLength != 0)
	      {
		/* Return the media change counter */
		*((PULONG)(OrigIrp->AssociatedIrp.SystemBuffer)) =
		  PhysicalExtension->MediaChangeCount;
		OrigIrp->IoStatus.Information = sizeof(ULONG);
	      }
	    else
	      {
		OrigIrp->IoStatus.Information = 0;
	      }
	    break;

	  default:
	    OrigIrp->IoStatus.Information = 0;
	    Status = STATUS_INVALID_DEVICE_REQUEST;
	    break;
	}
    }

  /* Release the SRB and associated buffers */
  if (Srb != NULL)
    {
      DPRINT("Srb %p\n", Srb);

      if (Srb->DataBuffer != NULL)
	ExFreePool (Srb->DataBuffer);

      if (Srb->SenseInfoBuffer != NULL)
	ExFreePool (Srb->SenseInfoBuffer);

      ExFreePool (Srb);
    }

  if (OrigIrp->PendingReturned)
    {
      IoMarkIrpPending (OrigIrp);
    }

  /* Release the MDL */
  if (Irp->MdlAddress != NULL)
    {
      IoFreeMdl (Irp->MdlAddress);
    }

  /* Release the sub irp */
  IoFreeIrp (Irp);

  /* Set io status information */
  OrigIrp->IoStatus.Status = Status;
  if (!NT_SUCCESS(Status) && IoIsErrorUserInduced (Status))
    {
      IoSetHardErrorOrVerifyDevice (OrigIrp,
				    DeviceObject);
      OrigIrp->IoStatus.Information = 0;
    }

  /* Complete the original IRP */
  IoCompleteRequest (OrigIrp,
		     IO_DISK_INCREMENT);
  IoStartNextPacket (DeviceObject,
		     FALSE);

  DPRINT ("CdromDeviceControlCompletion() done\n");

  return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID STDCALL
CdromTimerRoutine(PDEVICE_OBJECT DeviceObject,
		  PVOID Context)
{
  DPRINT ("CdromTimerRoutine() called\n");

}

/* EOF */
