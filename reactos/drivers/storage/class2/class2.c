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
/* $Id: class2.c,v 1.24 2002/09/08 10:22:22 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/class2/class2.c
 * PURPOSE:         SCSI class driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/*
 * TODO:
 *   - finish ScsiClassDeviceControl().
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include "../include/scsi.h"
#include "../include/class2.h"

#define NDEBUG
#include <debug.h>


#define VERSION "0.0.1"

#define TAG_SRBT  TAG('S', 'r', 'b', 'T')

#define INQUIRY_DATA_SIZE 2048


static NTSTATUS STDCALL
ScsiClassCreateClose(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS STDCALL
ScsiClassReadWrite(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

static NTSTATUS STDCALL
ScsiClassScsiDispatch(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS STDCALL
ScsiClassDeviceDispatch(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

static NTSTATUS STDCALL
ScsiClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

static VOID
ScsiClassRetryRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PSCSI_REQUEST_BLOCK Srb,
		      BOOLEAN Associated);

/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURNS
 *	Status
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DPRINT("Class Driver %s\n", VERSION);
  return(STATUS_SUCCESS);
}


VOID
ScsiClassDebugPrint(IN ULONG DebugPrintLevel,
		    IN PCHAR DebugMessage,
		    ...)
{
  char Buffer[256];
  va_list ap;

#if 0
  if (DebugPrintLevel > InternalDebugLevel)
    return;
#endif

  va_start(ap, DebugMessage);
  vsprintf(Buffer, DebugMessage, ap);
  va_end(ap);

  DbgPrint(Buffer);
}


NTSTATUS STDCALL
ScsiClassAsynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
				IN PIRP Irp,
				IN PVOID Context)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiClassBuildRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION CurrentIrpStack;
  PIO_STACK_LOCATION NextIrpStack;
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER StartingBlock;
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;
  ULONG LogicalBlockAddress;
  USHORT TransferBlocks;

  DeviceExtension = DeviceObject->DeviceExtension;
  CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);
  NextIrpStack = IoGetNextIrpStackLocation(Irp);
  StartingOffset = CurrentIrpStack->Parameters.Read.ByteOffset;

  /* Calculate logical block address */
  StartingBlock.QuadPart = StartingOffset.QuadPart >> DeviceExtension->SectorShift;
  LogicalBlockAddress = (ULONG)StartingBlock.u.LowPart;

  DPRINT("Logical block address: %lu\n", LogicalBlockAddress);

  /* Allocate and initialize an SRB */
  /* FIXME: use lookaside list instead */
  Srb = ExAllocatePool(NonPagedPool,
		       sizeof(SCSI_REQUEST_BLOCK));

  Srb->SrbFlags = 0;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK); //SCSI_REQUEST_BLOCK_SIZE;
  Srb->OriginalRequest = Irp;
  Srb->PathId = DeviceExtension->PathId;
  Srb->TargetId = DeviceExtension->TargetId;
  Srb->Lun = DeviceExtension->Lun;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->DataBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  Srb->DataTransferLength = CurrentIrpStack->Parameters.Read.Length;
  Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
  Srb->QueueSortKey = LogicalBlockAddress;

  Srb->SenseInfoBuffer = DeviceExtension->SenseData;
  Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

  Srb->TimeOutValue =
    ((Srb->DataTransferLength + 0xFFFF) >> 16) * DeviceExtension->TimeOutValue;

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  Srb->ScsiStatus = 0;
  Srb->NextSrb = 0;

  Srb->CdbLength = 10;
  Cdb = (PCDB)Srb->Cdb;

  /* Initialize ATAPI packet (12 bytes) */
  RtlZeroMemory(Cdb,
		MAXIMUM_CDB_SIZE);

  Cdb->CDB10.LogicalUnitNumber = DeviceExtension->Lun;
  TransferBlocks = (USHORT)(CurrentIrpStack->Parameters.Read.Length >> DeviceExtension->SectorShift);

  /* Copy little endian values into CDB in big endian format */
  Cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte3;
  Cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte2;
  Cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte1;
  Cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte0;

  Cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&TransferBlocks)->Byte1;
  Cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&TransferBlocks)->Byte0;


  if (CurrentIrpStack->MajorFunction == IRP_MJ_READ)
    {
      DPRINT("ScsiClassBuildRequest: Read Command\n");

      Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
      Cdb->CDB10.OperationCode = SCSIOP_READ;
    }
  else
    {
      DPRINT("ScsiClassBuildRequest: Write Command\n");

      Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
      Cdb->CDB10.OperationCode = SCSIOP_WRITE;
    }

#if 0
  /* if this is not a write-through request, then allow caching */
  if (!(CurrentIrpStack->Flags & SL_WRITE_THROUGH))
    {
      Srb->SrbFlags |= SRB_FLAGS_ADAPTER_CACHE_ENABLE;
    }
  else if (DeviceExtension->DeviceFlags & DEV_WRITE_CACHE)
    {
      /* if write caching is enable then force media access in the cdb */
      Cdb->CDB10.ForceUnitAccess = TRUE;
    }
#endif

  /* Update srb flags */
  Srb->SrbFlags |= DeviceExtension->SrbFlags;

  /* Initialize next stack location */
  NextIrpStack->MajorFunction = IRP_MJ_SCSI;
  NextIrpStack->Parameters.Scsi.Srb = Srb;
  NextIrpStack->DeviceObject = DeviceObject;

  /* Set retry count */
  NextIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

  DPRINT("IoSetCompletionRoutine (Irp %p  Srb %p)\n", Irp, Srb);
  IoSetCompletionRoutine(Irp,
			 ScsiClassIoComplete,
			 Srb,
			 TRUE,
			 TRUE,
			 TRUE);
}


NTSTATUS STDCALL
ScsiClassClaimDevice(PDEVICE_OBJECT PortDeviceObject,
		     PSCSI_INQUIRY_DATA LunInfo,
		     BOOLEAN Release,
		     PDEVICE_OBJECT *NewPortDeviceObject OPTIONAL)
{
  PIO_STACK_LOCATION IoStack;
  IO_STATUS_BLOCK IoStatusBlock;
  SCSI_REQUEST_BLOCK Srb;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("ScsiClassClaimDevice() called\n");

  if (NewPortDeviceObject != NULL)
    *NewPortDeviceObject = NULL;

  /* initialize an SRB */
  RtlZeroMemory(&Srb,
		sizeof(SCSI_REQUEST_BLOCK));
  Srb.Length = SCSI_REQUEST_BLOCK_SIZE;
  Srb.PathId = LunInfo->PathId;
  Srb.TargetId = LunInfo->TargetId;
  Srb.Lun = LunInfo->Lun;
  Srb.Function =
    (Release == TRUE) ? SRB_FUNCTION_RELEASE_DEVICE : SRB_FUNCTION_CLAIM_DEVICE;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_NONE,
				      PortDeviceObject,
				      NULL,
				      0,
				      NULL,
				      0,
				      TRUE,
				      &Event,
				      &IoStatusBlock);
  if (Irp == NULL)
    {
      DPRINT("Failed to allocate Irp!\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Link Srb and Irp */
  IoStack = IoGetNextIrpStackLocation(Irp);
  IoStack->Parameters.Scsi.Srb = &Srb;
  Srb.OriginalRequest = Irp;

  /* Call SCSI port driver */
  Status = IoCallDriver(PortDeviceObject,
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

  if (Release == TRUE)
    {
      ObDereferenceObject(PortDeviceObject);
      return(STATUS_SUCCESS);
    }

//  Status = ObReferenceObjectByPointer(Srb.DataBuffer,
  Status = ObReferenceObjectByPointer(PortDeviceObject,
				      0,
				      NULL,
				      KernelMode);

  if (NewPortDeviceObject != NULL)
    {
//      *NewPortDeviceObject = Srb.DataBuffer;
      *NewPortDeviceObject = PortDeviceObject;
    }

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
ScsiClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			    IN PCCHAR ObjectNameBuffer,
			    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
			    IN OUT PDEVICE_OBJECT *DeviceObject,
			    IN PCLASS_INIT_DATA InitializationData)
{
  PDEVICE_OBJECT InternalDeviceObject;
  PDEVICE_EXTENSION DeviceExtension;
  ANSI_STRING AnsiName;
  UNICODE_STRING DeviceName;
  NTSTATUS Status;

  DPRINT("ScsiClassCreateDeviceObject() called\n");

  *DeviceObject = NULL;

  RtlInitAnsiString(&AnsiName,
		    ObjectNameBuffer);

  Status = RtlAnsiStringToUnicodeString(&DeviceName,
					&AnsiName,
					TRUE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DPRINT("Device name: '%wZ'\n", &DeviceName);

  Status = IoCreateDevice(DriverObject,
			  InitializationData->DeviceExtensionSize,
			  &DeviceName,
			  InitializationData->DeviceType,
			  InitializationData->DeviceCharacteristics,
			  FALSE,
			  &InternalDeviceObject);
  if (NT_SUCCESS(Status))
    {
      DeviceExtension = InternalDeviceObject->DeviceExtension;

      DeviceExtension->ClassError = InitializationData->ClassError;
      DeviceExtension->ClassReadWriteVerification = InitializationData->ClassReadWriteVerification;
      DeviceExtension->ClassFindDevices = InitializationData->ClassFindDevices;
      DeviceExtension->ClassDeviceControl = InitializationData->ClassDeviceControl;
      DeviceExtension->ClassShutdownFlush = InitializationData->ClassShutdownFlush;
      DeviceExtension->ClassCreateClose = InitializationData->ClassCreateClose;
      DeviceExtension->ClassStartIo = InitializationData->ClassStartIo;

      DeviceExtension->MediaChangeCount = 0;

      if (PhysicalDeviceObject != NULL)
	{
	  DeviceExtension->PhysicalDevice = PhysicalDeviceObject;
	}
      else
	{
	  DeviceExtension->PhysicalDevice = InternalDeviceObject;
        }

      *DeviceObject = InternalDeviceObject;
    }

  RtlFreeUnicodeString(&DeviceName);

  return(Status);
}


NTSTATUS STDCALL
ScsiClassDeviceControl(PDEVICE_OBJECT DeviceObject,
		       PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;
  ULONG IoControlCode;
  ULONG OutputBufferLength;

  DPRINT1("ScsiClassDeviceControl() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);

  IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
  OutputBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;

  if (IoControlCode == IOCTL_SCSI_GET_ADDRESS)
    {
      PSCSI_ADDRESS ScsiAddress;

      if (OutputBufferLength < sizeof(SCSI_ADDRESS))
	{
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);

	  return(STATUS_BUFFER_TOO_SMALL);
	}

      ScsiAddress = Irp->AssociatedIrp.SystemBuffer;
      ScsiAddress->Length = sizeof(SCSI_ADDRESS);
      ScsiAddress->PortNumber = DeviceExtension->PortNumber;
      ScsiAddress->PathId = DeviceExtension->PathId;
      ScsiAddress->TargetId = DeviceExtension->TargetId;
      ScsiAddress->Lun = DeviceExtension->Lun;

      Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
      Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(STATUS_SUCCESS);
    }

  if (IoControlCode == IOCTL_SCSI_PASS_THROUGH ||
      IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT)
    {
      DPRINT1("Fixme: IOCTL_SCSI_PASS_THROUGH/IOCTL_SCSI_PASS_THROUGH_DIRECT\n");


      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(STATUS_UNSUCCESSFUL);
    }

  DPRINT1("Fixme: unknown device io control code\n");

  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_UNSUCCESSFUL);
}


PVOID STDCALL
ScsiClassFindModePage(PCHAR ModeSenseBuffer,
		      ULONG Length,
		      UCHAR PageMode,
		      BOOLEAN Use6Byte)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiClassFindUnclaimedDevices(PCLASS_INIT_DATA InitializationData,
			      PSCSI_ADAPTER_BUS_INFO AdapterInformation)
{
  PSCSI_INQUIRY_DATA UnitInfo;
  PINQUIRYDATA InquiryData;
  PUCHAR Buffer;
  ULONG Bus;
  ULONG UnclaimedDevices = 0;
  NTSTATUS Status;

  DPRINT("ScsiClassFindUnclaimedDevices() called\n");

  DPRINT("NumberOfBuses: %lu\n",AdapterInformation->NumberOfBuses);
  Buffer = (PUCHAR)AdapterInformation;
  for (Bus = 0; Bus < (ULONG)AdapterInformation->NumberOfBuses; Bus++)
    {
      DPRINT("Searching bus %lu\n", Bus);

      UnitInfo = (PSCSI_INQUIRY_DATA)(Buffer + AdapterInformation->BusData[Bus].InquiryDataOffset);

      while (AdapterInformation->BusData[Bus].InquiryDataOffset)
	{
	  InquiryData = (PINQUIRYDATA)UnitInfo->InquiryData;

	  DPRINT("Device: '%.8s'\n", InquiryData->VendorId);

	  if ((InitializationData->ClassFindDeviceCallBack(InquiryData) == TRUE) &&
	      (UnitInfo->DeviceClaimed == FALSE))
	    {
	      UnclaimedDevices++;
	    }

	  if (UnitInfo->NextInquiryDataOffset == 0)
	    break;

	  UnitInfo = (PSCSI_INQUIRY_DATA) (Buffer + UnitInfo->NextInquiryDataOffset);
	}
    }

  return(UnclaimedDevices);
}


NTSTATUS STDCALL
ScsiClassGetCapabilities(PDEVICE_OBJECT PortDeviceObject,
			 PIO_SCSI_CAPABILITIES *PortCapabilities)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_CAPABILITIES,
				      PortDeviceObject,
				      NULL,
				      0,
				      PortCapabilities,
				      sizeof(PVOID),
				      FALSE,
				      &Event,
				      &IoStatusBlock);
  if (Irp == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = IoCallDriver(PortDeviceObject,
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

  DPRINT("PortCapabilities at %p\n", *PortCapabilities);

  return(Status);
}


NTSTATUS STDCALL
ScsiClassGetInquiryData(PDEVICE_OBJECT PortDeviceObject,
			PSCSI_ADAPTER_BUS_INFO *ConfigInfo)
{
  PSCSI_ADAPTER_BUS_INFO Buffer;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  DPRINT("ScsiClassGetInquiryData() called\n");

  *ConfigInfo = NULL;
  Buffer = ExAllocatePool(NonPagedPool,
			  INQUIRY_DATA_SIZE);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_INQUIRY_DATA,
				      PortDeviceObject,
				      NULL,
				      0,
				      Buffer,
				      INQUIRY_DATA_SIZE,
				      FALSE,
				      &Event,
				      &IoStatusBlock);
  if (Irp == NULL)
    {
      ExFreePool(Buffer);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = IoCallDriver(PortDeviceObject,
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
      ExFreePool(Buffer);
    }
  else
    {
      *ConfigInfo = Buffer;
    }

  DPRINT("ScsiClassGetInquiryData() done\n");

  return(Status);
}


ULONG STDCALL
ScsiClassInitialize(PVOID Argument1,
		    PVOID Argument2,
		    PCLASS_INIT_DATA InitializationData)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  PDRIVER_OBJECT DriverObject = Argument1;
  WCHAR NameBuffer[80];
  UNICODE_STRING PortName;
  ULONG PortNumber;
  PDEVICE_OBJECT PortDeviceObject;
  PFILE_OBJECT FileObject;
  BOOLEAN DiskFound = FALSE;
  NTSTATUS Status;

  DPRINT("ScsiClassInitialize() called!\n");

  DriverObject->MajorFunction[IRP_MJ_CREATE] = ScsiClassCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiClassCreateClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = ScsiClassReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = ScsiClassReadWrite;
  DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiClassScsiDispatch;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiClassDeviceDispatch;
  DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ScsiClassShutdownFlush;
  DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = ScsiClassShutdownFlush;
  if (InitializationData->ClassStartIo)
    {
      DriverObject->DriverStartIo = InitializationData->ClassStartIo;
    }

  ConfigInfo = IoGetConfigurationInformation();

  DPRINT("ScsiPorts: %lu\n", ConfigInfo->ScsiPortCount);

  /* look for ScsiPortX scsi port devices */
  for (PortNumber = 0; PortNumber < ConfigInfo->ScsiPortCount; PortNumber++)
    {
      swprintf(NameBuffer,
	       L"\\Device\\ScsiPort%lu",
	        PortNumber);
      RtlInitUnicodeString(&PortName,
			   NameBuffer);
      DPRINT("Checking scsi port %ld\n", PortNumber);
      Status = IoGetDeviceObjectPointer(&PortName,
					FILE_READ_ATTRIBUTES,
					&FileObject,
					&PortDeviceObject);
      DPRINT("Status 0x%08lX\n", Status);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("ScsiPort%lu found.\n", PortNumber);

	  /* check scsi port for attached disk drives */
	  if (InitializationData->ClassFindDevices(DriverObject,
						   Argument2,
						   InitializationData,
						   PortDeviceObject,
						   PortNumber))
	    {
	      DiskFound = TRUE;
	    }
	}
      else
	{
	  DbgPrint("Couldn't find ScsiPort%lu (Status %lx)\n", PortNumber, Status);
	}
    }

  DPRINT("ScsiClassInitialize() done!\n");

  return((DiskFound == TRUE) ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiClassInitializeSrbLookasideList
 *
 * DESCRIPTION
 *	Initializes a lookaside list for SRBs.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Class specific device extension.
 *
 *	NumberElements
 *		Maximum number of elements of the lookaside list.
 *
 * RETURN VALUE
 *	None.
 */

VOID STDCALL
ScsiClassInitializeSrbLookasideList(PDEVICE_EXTENSION DeviceExtension,
				    ULONG NumberElements)
{
  ExInitializeNPagedLookasideList(&DeviceExtension->SrbLookasideListHead,
				  NULL,
				  NULL,
				  NonPagedPool,
				  sizeof(SCSI_REQUEST_BLOCK),
				  TAG_SRBT,
				  (USHORT)NumberElements);
}


NTSTATUS STDCALL
ScsiClassInternalIoControl(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp)
{
  UNIMPLEMENTED;
}


BOOLEAN STDCALL
ScsiClassInterpretSenseInfo(PDEVICE_OBJECT DeviceObject,
			    PSCSI_REQUEST_BLOCK Srb,
			    UCHAR MajorFunctionCode,
			    ULONG IoDeviceCode,
			    ULONG RetryCount,
			    NTSTATUS *Status)
{
  PDEVICE_EXTENSION DeviceExtension;
  PSENSE_DATA SenseData;
  BOOLEAN Retry;

  DPRINT("ScsiClassInterpretSenseInfo() called\n");

  DPRINT("Srb->SrbStatus %lx\n", Srb->SrbStatus);

  if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_PENDING)
    {
      *Status = STATUS_SUCCESS;
      return(FALSE);
    }

  DeviceExtension = DeviceObject->DeviceExtension;
  SenseData = Srb->SenseInfoBuffer;
  Retry = TRUE;

  if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
      (Srb->SenseInfoBufferLength > 0))
    {
      /* Got valid sense data, interpret them */

      DPRINT("ErrorCode: %x\n", SenseData->ErrorCode);
      DPRINT("SenseKey: %x\n", SenseData->SenseKey);
      DPRINT("SenseCode: %x\n", SenseData->AdditionalSenseCode);

      switch (SenseData->SenseKey & 0xf)
	{
	  /* FIXME: add more sense key codes */

	  case SCSI_SENSE_NOT_READY:
	    DPRINT("SCSI_SENSE_NOT_READY\n");
	    *Status = STATUS_DEVICE_NOT_READY;
	    break;

	  case SCSI_SENSE_UNIT_ATTENTION:
	    DPRINT("SCSI_SENSE_UNIT_ATTENTION\n");
	    if ((DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) &&
		(DeviceObject->Vpb->Flags & VPB_MOUNTED))
	      {
		DeviceObject->Flags |= DO_VERIFY_VOLUME;
		*Status = STATUS_VERIFY_REQUIRED;
		Retry = FALSE;
	      }
	    else
	      {
		*Status = STATUS_IO_DEVICE_ERROR;
	      }
	    break;

	  case SCSI_SENSE_ILLEGAL_REQUEST:
	    DPRINT("SCSI_SENSE_ILLEGAL_REQUEST\n");
	    *Status = STATUS_INVALID_DEVICE_REQUEST;
	    Retry = FALSE;
	    break;


	  default:
	    DPRINT1("SCSI error (sense key: %x)\n",
		    SenseData->SenseKey & 0xf);
	    *Status = STATUS_IO_DEVICE_ERROR;
	    break;
	}
    }
  else
    {
      /* Got no or invalid sense data, return generic error codes */
      switch (SRB_STATUS(Srb->SrbStatus))
	{
	  /* FIXME: add more srb status codes */

	  case SRB_STATUS_INVALID_PATH_ID:
	  case SRB_STATUS_INVALID_TARGET_ID:
	  case SRB_STATUS_INVALID_LUN:
	  case SRB_STATUS_NO_DEVICE:
	  case SRB_STATUS_NO_HBA:
	    *Status = STATUS_NO_SUCH_DEVICE;
	    Retry = FALSE;
	    break;

	  case SRB_STATUS_BUSY:
	    *Status = STATUS_DEVICE_BUSY;
	    Retry = TRUE;
	    break;

	  case SRB_STATUS_DATA_OVERRUN:
	    *Status = STATUS_DATA_OVERRUN;
	    Retry = FALSE;
	    break;

	  default:
	    DPRINT1("SCSI error (SRB status: %x)\n",
		    SRB_STATUS(Srb->SrbStatus));
	    *Status = STATUS_IO_DEVICE_ERROR;
	    break;
	}
    }

  /* Call the class driver specific error function */
  if (DeviceExtension->ClassError != NULL)
    {
      DeviceExtension->ClassError(DeviceObject,
				  Srb,
				  Status,
				  &Retry);
    }

  /* FIXME: log severe errors */

  DPRINT("ScsiClassInterpretSenseInfo() done\n");

  return(Retry);
}


NTSTATUS STDCALL
ScsiClassIoComplete(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp,
		    PVOID Context)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;
  BOOLEAN Retry;
  NTSTATUS Status;

  DPRINT("ScsiClassIoComplete(DeviceObject %p  Irp %p  Context %p) called\n",
	  DeviceObject, Irp, Context);

  DeviceExtension = DeviceObject->DeviceExtension;
  Srb = (PSCSI_REQUEST_BLOCK)Context;
  DPRINT("Srb %p\n", Srb);

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
      Status = STATUS_SUCCESS;
    }
  else
    {
      Retry = ScsiClassInterpretSenseInfo(DeviceObject,
					  Srb,
					  IrpStack->MajorFunction,
					  0,
					  MAXIMUM_RETRIES - ((ULONG)IrpStack->Parameters.Others.Argument4),
					  &Status);
      if ((Retry == TRUE) &&
	  ((ULONG)IrpStack->Parameters.Others.Argument4 > 0))
	{
	  ((ULONG)IrpStack->Parameters.Others.Argument4)--;

	  ScsiClassRetryRequest(DeviceObject,
				Irp,
				Srb,
				FALSE);
	  return(STATUS_MORE_PROCESSING_REQUIRED);
	}
    }

  /* FIXME: use lookaside list instead */
  DPRINT("Freed SRB %p\n", IrpStack->Parameters.Scsi.Srb);
  ExFreePool(IrpStack->Parameters.Scsi.Srb);

  Irp->IoStatus.Status = Status;
  if (!NT_SUCCESS(Status))
    {
      Irp->IoStatus.Information = 0;
      if (IoIsErrorUserInduced(Status))
	{
	  IoSetHardErrorOrVerifyDevice(Irp,
				       DeviceObject);
	}
    }

  if (DeviceExtension->ClassStartIo != NULL)
    {
      if (IrpStack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
	  IoStartNextPacket(DeviceObject,
			    FALSE);
	}
    }

  DPRINT("ScsiClassIoComplete() done (Status %lx)\n", Status);

  return(Status);
}


NTSTATUS STDCALL
ScsiClassIoCompleteAssociated(PDEVICE_OBJECT DeviceObject,
			      PIRP Irp,
			      PVOID Context)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;
  PIRP MasterIrp;
  BOOLEAN Retry;
  LONG RequestCount;
  NTSTATUS Status;

  DPRINT("ScsiClassIoCompleteAssociated(DeviceObject %p  Irp %p  Context %p) called\n",
	 DeviceObject, Irp, Context);

  MasterIrp = Irp->AssociatedIrp.MasterIrp;
  DeviceExtension = DeviceObject->DeviceExtension;
  Srb = (PSCSI_REQUEST_BLOCK)Context;
  DPRINT("Srb %p\n", Srb);

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
      Status = STATUS_SUCCESS;
    }
  else
    {
      /* Get more detailed status information */
      Retry = ScsiClassInterpretSenseInfo(DeviceObject,
					  Srb,
					  IrpStack->MajorFunction,
					  0,
					  MAXIMUM_RETRIES - ((ULONG)IrpStack->Parameters.Others.Argument4),
					  &Status);

      DPRINT1("Retry count: %lu\n", (ULONG)IrpStack->Parameters.Others.Argument4);

      if ((Retry == TRUE) &&
	  ((ULONG)IrpStack->Parameters.Others.Argument4 > 0))
	{
	  ((ULONG)IrpStack->Parameters.Others.Argument4)--;

	  ScsiClassRetryRequest(DeviceObject,
				Irp,
				Srb,
				TRUE);
	  return(STATUS_MORE_PROCESSING_REQUIRED);
	}
    }

  /* FIXME: use lookaside list instead */
  DPRINT("Freed SRB %p\n", IrpStack->Parameters.Scsi.Srb);
  ExFreePool(IrpStack->Parameters.Scsi.Srb);

  Irp->IoStatus.Status = Status;

  IrpStack = IoGetNextIrpStackLocation(MasterIrp);
  if (!NT_SUCCESS(Status))
    {
      MasterIrp->IoStatus.Status = Status;
      MasterIrp->IoStatus.Information = 0;

      if (IoIsErrorUserInduced(Status))
	{
	  IoSetHardErrorOrVerifyDevice(MasterIrp,
				       DeviceObject);
	}
    }

  /* Decrement the request counter in the Master IRP */
  RequestCount = InterlockedDecrement((PLONG)&IrpStack->Parameters.Others.Argument1);

  if (RequestCount == 0)
    {
      /* Complete the Master IRP */
      IoCompleteRequest(MasterIrp,
			IO_DISK_INCREMENT);

      if (DeviceExtension->ClassStartIo)
	{
	  IoStartNextPacket(DeviceObject,
			    FALSE);
	}
    }

  /* Free the current IRP */
  IoFreeIrp(Irp);

  return(STATUS_MORE_PROCESSING_REQUIRED);
}


ULONG STDCALL
ScsiClassModeSense(PDEVICE_OBJECT DeviceObject,
		   CHAR ModeSenseBuffer,
		   ULONG Length,
		   UCHAR PageMode)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiClassQueryTimeOutRegistryValue(IN PUNICODE_STRING RegistryPath)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassReadDriveCapacity(IN PDEVICE_OBJECT DeviceObject)
{
  PDEVICE_EXTENSION DeviceExtension;
  PREAD_CAPACITY_DATA CapacityBuffer;
  SCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;
  NTSTATUS Status;
  ULONG LastSector;
  ULONG SectorSize;

  DPRINT("ScsiClassReadDriveCapacity() called\n");

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

  CapacityBuffer = ExAllocatePool(NonPagedPool,
				  sizeof(READ_CAPACITY_DATA));
  if (CapacityBuffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

  Srb.CdbLength = 10;
  Srb.TimeOutValue = DeviceExtension->TimeOutValue;

  Cdb = (PCDB)Srb.Cdb;
  Cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;


  Status = ScsiClassSendSrbSynchronous(DeviceObject,
				       &Srb,
				       CapacityBuffer,
				       sizeof(READ_CAPACITY_DATA),
				       FALSE);
  DPRINT("Status: %lx\n", Status);
  DPRINT("Srb: %p\n", &Srb);
  if (NT_SUCCESS(Status))
    {
      SectorSize = (((PUCHAR)&CapacityBuffer->BytesPerBlock)[0] << 24) |
		   (((PUCHAR)&CapacityBuffer->BytesPerBlock)[1] << 16) |
		   (((PUCHAR)&CapacityBuffer->BytesPerBlock)[2] << 8) |
		    ((PUCHAR)&CapacityBuffer->BytesPerBlock)[3];


      LastSector = (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[0] << 24) |
		   (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[1] << 16) |
		   (((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[2] << 8) |
		    ((PUCHAR)&CapacityBuffer->LogicalBlockAddress)[3];

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
      DeviceExtension->DiskGeometry->Cylinders.QuadPart = (LONGLONG)((LastSector + 1)/(32 * 64));
      DeviceExtension->DiskGeometry->SectorsPerTrack = 32;
      DeviceExtension->DiskGeometry->TracksPerCylinder = 64;

      DPRINT("SectorSize: %lu  SectorCount: %lu\n", SectorSize, LastSector + 1);
    }
  else
    {
      /* Use default values if disk geometry cannot be read */
      RtlZeroMemory(DeviceExtension->DiskGeometry,
		    sizeof(DISK_GEOMETRY));
      DeviceExtension->DiskGeometry->BytesPerSector = 512;
      DeviceExtension->SectorShift = 9;
      DeviceExtension->PartitionLength.QuadPart = 0;

      if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
	{
	  DeviceExtension->DiskGeometry->MediaType = RemovableMedia;
	}
      else
	{
	  DeviceExtension->DiskGeometry->MediaType = FixedMedia;
	}

      DPRINT("SectorSize: 512  SectorCount: 0\n");
    }

  ExFreePool(CapacityBuffer);

  DPRINT("ScsiClassReadDriveCapacity() done\n");

  return(Status);
}


VOID STDCALL
ScsiClassReleaseQueue(IN PDEVICE_OBJECT DeviceObject)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassSendSrbAsynchronous(PDEVICE_OBJECT DeviceObject,
			     PSCSI_REQUEST_BLOCK Srb,
			     PIRP Irp,
			     PVOID BufferAddress,
			     ULONG BufferLength,
			     BOOLEAN WriteToDevice)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassSendSrbSynchronous(PDEVICE_OBJECT DeviceObject,
			    PSCSI_REQUEST_BLOCK Srb,
			    PVOID BufferAddress,
			    ULONG BufferLength,
			    BOOLEAN WriteToDevice)
{
  PDEVICE_EXTENSION DeviceExtension;
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION IrpStack;
  ULONG RequestType;
  BOOLEAN Retry;
  ULONG RetryCount;
  PKEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("ScsiClassSendSrbSynchronous() called\n");

  RetryCount = MAXIMUM_RETRIES;
  DeviceExtension = DeviceObject->DeviceExtension;

  Srb->Length = SCSI_REQUEST_BLOCK_SIZE;
  Srb->PathId = DeviceExtension->PathId;
  Srb->TargetId = DeviceExtension->TargetId;
  Srb->Lun = DeviceExtension->Lun;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

  Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
  Srb->SenseInfoBuffer = ExAllocatePool(NonPagedPool,
					SENSE_BUFFER_SIZE);
  if (Srb->SenseInfoBuffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  if (BufferAddress == NULL)
    {
        BufferLength = 0;
        RequestType = IOCTL_SCSI_EXECUTE_NONE;
        Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    }
  else
    {
      if (WriteToDevice == TRUE)
	{
	  RequestType = IOCTL_SCSI_EXECUTE_OUT;
	  Srb->SrbFlags = SRB_FLAGS_DATA_OUT;
	}
      else
	{
	  RequestType = IOCTL_SCSI_EXECUTE_IN;
	  Srb->SrbFlags = SRB_FLAGS_DATA_IN;
	}
    }

  Srb->DataTransferLength = BufferLength;
  Srb->DataBuffer = BufferAddress;

  Event = ExAllocatePool(NonPagedPool,
			 sizeof(KEVENT));
TryAgain:
  KeInitializeEvent(Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(RequestType,
				      DeviceExtension->PortDeviceObject,
				      NULL,
				      0,
				      BufferAddress,
				      BufferLength,
				      TRUE,
				      Event,
				      &IoStatusBlock);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest() failed\n");
      ExFreePool(Srb->SenseInfoBuffer);
      ExFreePool(Event);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Attach Srb to the Irp */
  IrpStack = IoGetNextIrpStackLocation(Irp);
  IrpStack->Parameters.Scsi.Srb = Srb;
  Srb->OriginalRequest = Irp;

  /* Call the SCSI port driver */
  Status = IoCallDriver(DeviceExtension->PortDeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(Event,
			    Suspended,
			    KernelMode,
			    FALSE,
			    NULL);
    }

  if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
    {
      Retry = ScsiClassInterpretSenseInfo(DeviceObject,
					  Srb,
					  IRP_MJ_SCSI,
					  0,
					  MAXIMUM_RETRIES - RetryCount,
					  &Status);
      if (Retry == TRUE)
	{
	  DPRINT("Try again (RetryCount %lu)\n", RetryCount);

	  /* FIXME: Wait a little if we got a timeout error */

	  if (RetryCount--)
	    goto TryAgain;
	}
    }
  else
    {
      Status = STATUS_SUCCESS;
    }

  ExFreePool(Srb->SenseInfoBuffer);
  ExFreePool(Event);

  DPRINT("ScsiClassSendSrbSynchronous() done\n");

  return(Status);
}


VOID STDCALL
ScsiClassSplitRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      ULONG MaximumBytes)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION CurrentStack;
  PIO_STACK_LOCATION NextStack;
  PIO_STACK_LOCATION NewStack;
  PSCSI_REQUEST_BLOCK Srb;
  LARGE_INTEGER Offset;
  PIRP NewIrp;
  PVOID DataBuffer;
  ULONG TransferLength;
  ULONG RequestCount;
  ULONG DataLength;
  ULONG i;

  DPRINT("ScsiClassSplitRequest(DeviceObject %lx  Irp %lx  MaximumBytes %lu)\n",
	 DeviceObject, Irp, MaximumBytes);

  DeviceExtension = DeviceObject->DeviceExtension;
  CurrentStack = IoGetCurrentIrpStackLocation(Irp);
  NextStack = IoGetNextIrpStackLocation(Irp);
  DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

  /* Initialize transfer data for first request */
  Offset = CurrentStack->Parameters.Read.ByteOffset;
  TransferLength = CurrentStack->Parameters.Read.Length;
  DataLength = MaximumBytes;
  RequestCount = ROUND_UP(TransferLength, MaximumBytes) / MaximumBytes;

  /* Save request count in the original IRP */
  NextStack->Parameters.Others.Argument1 = (PVOID)RequestCount;

  DPRINT("RequestCount %lu\n", RequestCount);

  for (i = 0; i < RequestCount; i++)
    {
      /* Create a new IRP */
      NewIrp = IoAllocateIrp(DeviceObject->StackSize,
			     FALSE);
      if (NewIrp == NULL)
	{
	  Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	  Irp->IoStatus.Information = 0;

	  if (i == 0)
	    IoCompleteRequest(Irp,
			      IO_NO_INCREMENT);
	  return;
	}

      /* Initialize the new IRP */
      NewIrp->MdlAddress = Irp->MdlAddress;

      IoSetNextIrpStackLocation(NewIrp);
      NewStack = IoGetCurrentIrpStackLocation(NewIrp);

      NewStack->MajorFunction = CurrentStack->MajorFunction;
      NewStack->Parameters.Read.ByteOffset = Offset;
      NewStack->Parameters.Read.Length = DataLength;
      NewStack->DeviceObject = DeviceObject;

      ScsiClassBuildRequest(DeviceObject,
			    NewIrp);

      NewStack = IoGetNextIrpStackLocation(NewIrp);
      Srb = NewStack->Parameters.Others.Argument1;
      Srb->DataBuffer = DataBuffer;

      NewIrp->AssociatedIrp.MasterIrp = Irp;

      /* Initialize completion routine */
      IoSetCompletionRoutine(NewIrp,
			     ScsiClassIoCompleteAssociated,
			     Srb,
			     TRUE,
			     TRUE,
			     TRUE);

      /* Send the new IRP down to the port driver */
      IoCallDriver(DeviceExtension->PortDeviceObject,
		   NewIrp);

      /* Adjust transfer data for next request */
      DataBuffer = (PCHAR)DataBuffer + MaximumBytes;
      TransferLength -= MaximumBytes;
      DataLength = (TransferLength > MaximumBytes) ? MaximumBytes : TransferLength;
      Offset.QuadPart = Offset.QuadPart + MaximumBytes;
    }
}


/* INTERNAL FUNCTIONS *******************************************************/

static NTSTATUS STDCALL
ScsiClassCreateClose(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiClassCreateClose() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;

  if (DeviceExtension->ClassCreateClose)
    return(DeviceExtension->ClassCreateClose(DeviceObject,
					     Irp));

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassReadWrite(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  ULONG MaximumTransferLength;
  ULONG CurrentTransferLength;
  ULONG MaximumTransferPages;
  ULONG CurrentTransferPages;
  NTSTATUS Status;

  DPRINT("ScsiClassReadWrite() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  IrpStack  = IoGetCurrentIrpStackLocation(Irp);

  DPRINT("Relative Offset: %I64u  Length: %lu\n",
	 IrpStack->Parameters.Read.ByteOffset.QuadPart,
	 IrpStack->Parameters.Read.Length);

  MaximumTransferLength = DeviceExtension->PortCapabilities->MaximumTransferLength;
  MaximumTransferPages = DeviceExtension->PortCapabilities->MaximumPhysicalPages;

  CurrentTransferLength = IrpStack->Parameters.Read.Length;

  if ((DeviceObject->Flags & DO_VERIFY_VOLUME) &&
      !(IrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME))
    {
      IoSetHardErrorOrVerifyDevice(Irp,
				   DeviceObject);

      Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_VERIFY_REQUIRED);
    }

  /* Class driver verifies the IRP */
  Status = DeviceExtension->ClassReadWriteVerification(DeviceObject,
						       Irp);
  if (!NT_SUCCESS(Status))
    {
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(Status);
    }
  else if (Status == STATUS_PENDING)
    {
      IoMarkIrpPending(Irp);
      return(STATUS_PENDING);
    }

  /* Finish a zero-byte transfer */
  if (CurrentTransferLength == 0)
    {
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_SUCCESS);
    }

  if (DeviceExtension->ClassStartIo != NULL)
    {
      DPRINT("ScsiClassReadWrite() starting packet\n");

      IoMarkIrpPending(Irp);
      IoStartPacket(DeviceObject,
		    Irp,
		    NULL,
		    NULL);

      return(STATUS_PENDING);
    }

  /* Adjust partition-relative starting offset to absolute offset */
  IrpStack->Parameters.Read.ByteOffset.QuadPart += DeviceExtension->StartingOffset.QuadPart;

  /* Calculate number of pages in this transfer */
  CurrentTransferPages =
    ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Irp->MdlAddress),
				   IrpStack->Parameters.Read.Length);

  if (CurrentTransferLength > MaximumTransferLength ||
      CurrentTransferPages > MaximumTransferPages)
    {
       DPRINT("Split current request: MaximumTransferLength %lu  CurrentTransferLength %lu\n",
	      MaximumTransferLength, CurrentTransferLength);

      /* Adjust the maximum transfer length */
      CurrentTransferPages = DeviceExtension->PortCapabilities->MaximumPhysicalPages;

      if (MaximumTransferLength > CurrentTransferPages * PAGESIZE)
	  MaximumTransferLength = CurrentTransferPages * PAGESIZE;

      if (MaximumTransferLength == 0)
	  MaximumTransferLength = PAGESIZE;

      IoMarkIrpPending(Irp);

      /* Split current request */
      ScsiClassSplitRequest(DeviceObject,
			    Irp,
			    MaximumTransferLength);

      return(STATUS_PENDING);
    }

  ScsiClassBuildRequest(DeviceObject,
			Irp);

  DPRINT("ScsiClassReadWrite() done\n");

  /* Call the port driver */
  return(IoCallDriver(DeviceExtension->PortDeviceObject,
		      Irp));
}


static NTSTATUS STDCALL
ScsiClassScsiDispatch(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
  DPRINT1("ScsiClassScsiDispatch() called\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassDeviceDispatch(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiClassDeviceDispatch() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  if (DeviceExtension->ClassDeviceControl)
    {
      return(DeviceExtension->ClassDeviceControl(DeviceObject, Irp));
    }

  Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_INVALID_DEVICE_REQUEST);
}


static NTSTATUS STDCALL
ScsiClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiClassShutdownFlush() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  if (DeviceExtension->ClassShutdownFlush)
    {
      return(DeviceExtension->ClassShutdownFlush(DeviceObject, Irp));
    }

  Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_INVALID_DEVICE_REQUEST);
}


static VOID
ScsiClassRetryRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PSCSI_REQUEST_BLOCK Srb,
		      BOOLEAN Associated)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION CurrentIrpStack;
  PIO_STACK_LOCATION NextIrpStack;
  ULONG TransferLength;

  DPRINT("ScsiPortRetryRequest() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);
  NextIrpStack = IoGetNextIrpStackLocation(Irp);

  if (CurrentIrpStack->MajorFunction == IRP_MJ_READ ||
      CurrentIrpStack->MajorFunction == IRP_MJ_WRITE)
    {
      TransferLength = CurrentIrpStack->Parameters.Read.Length;
    }
  else if (Irp->MdlAddress != NULL)
    {
      TransferLength = Irp->MdlAddress->ByteCount;
    }
  else
    {
      TransferLength = 0;
    }

  Srb->DataTransferLength = TransferLength;
  Srb->SrbStatus = 0;
  Srb->ScsiStatus = 0;

  /* Don't modify the flags */
//  Srb->Flags = 
//  Srb->QueueTag = SP_UNTAGGED;

  NextIrpStack->MajorFunction = IRP_MJ_SCSI;
  NextIrpStack->Parameters.Scsi.Srb = Srb;

  if (Associated == FALSE)
    {
      IoSetCompletionRoutine(Irp,
			     ScsiClassIoComplete,
			     Srb,
			     TRUE,
			     TRUE,
			     TRUE);
    }
  else
    {
      IoSetCompletionRoutine(Irp,
			     ScsiClassIoCompleteAssociated,
			     Srb,
			     TRUE,
			     TRUE,
			     TRUE);
    }

  IoCallDriver(DeviceExtension->PortDeviceObject,
	       Irp);

  DPRINT("ScsiPortRetryRequest() done\n");
}

/* EOF */
