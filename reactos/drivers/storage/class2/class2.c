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
/* $Id: class2.c,v 1.38 2003/07/12 13:10:45 ekohl Exp $
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
#include <ddk/scsi.h>
#include <ddk/class2.h>

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
ScsiClassDeviceDispatch(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

static NTSTATUS STDCALL
ScsiClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

static VOID
ScsiClassRetryRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp, PSCSI_REQUEST_BLOCK Srb, BOOLEAN Associated);

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


/*
 * @unimplemented
 */
NTSTATUS STDCALL
ScsiClassAsynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
				IN PIRP Irp,
				IN PVOID Context)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
ScsiClassBuildRequest(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
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
  Srb = ExAllocateFromNPagedLookasideList(&DeviceExtension->SrbLookasideListHead);

  Srb->SrbFlags = 0;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK); //SCSI_REQUEST_BLOCK_SIZE;
  Srb->OriginalRequest = Irp;
  Srb->PathId = DeviceExtension->PathId;
  Srb->TargetId = DeviceExtension->TargetId;
  Srb->Lun = DeviceExtension->Lun;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  //FIXME: NT4 DDK sample uses MmGetMdlVirtualAddress! Why shouldn't we?
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

  /* Set retry count */
  CurrentIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

  DPRINT("IoSetCompletionRoutine (Irp %p  Srb %p)\n", Irp, Srb);
  IoSetCompletionRoutine(Irp,
			 ScsiClassIoComplete,
			 Srb,
			 TRUE,
			 TRUE,
			 TRUE);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION NextStack;
  PIO_STACK_LOCATION Stack;
  ULONG IoControlCode;
  ULONG InputBufferLength;
  ULONG OutputBufferLength;
  ULONG ModifiedControlCode;
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;

  DPRINT("ScsiClassDeviceControl() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);

  IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
  InputBufferLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
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
      PSCSI_PASS_THROUGH ScsiPassThrough;

      DPRINT("IOCTL_SCSI_PASS_THROUGH/IOCTL_SCSI_PASS_THROUGH_DIRECT\n");

      /* Check input size */
      if (InputBufferLength < sizeof(SCSI_PASS_THROUGH))
	{
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	  return(STATUS_INVALID_PARAMETER);
	}

      /* Initialize next stack location for call to the port driver */
      NextStack = IoGetNextIrpStackLocation(Irp);

      ScsiPassThrough = Irp->AssociatedIrp.SystemBuffer;
      ScsiPassThrough->PathId = DeviceExtension->PathId;
      ScsiPassThrough->TargetId = DeviceExtension->TargetId;
      ScsiPassThrough->Lun = DeviceExtension->Lun;
      ScsiPassThrough->Cdb[1] |= DeviceExtension->Lun << 5;

      NextStack->Parameters = Stack->Parameters;
      NextStack->MajorFunction = Stack->MajorFunction;
      NextStack->MinorFunction = Stack->MinorFunction;

      /* Call port driver */
      return(IoCallDriver(DeviceExtension->PortDeviceObject,
			  Irp));
    }

  /* Allocate an SRB */
  Srb = ExAllocatePool (NonPagedPool,
			sizeof(SCSI_REQUEST_BLOCK));
  if (Srb == NULL)
    {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize the SRB */
  RtlZeroMemory(Srb,
		sizeof(SCSI_REQUEST_BLOCK));
  Cdb = (PCDB)Srb->Cdb;

  ModifiedControlCode = (IoControlCode & 0x0000FFFF) | (IOCTL_DISK_BASE << 16);
  switch (ModifiedControlCode)
    {
      case IOCTL_DISK_CHECK_VERIFY:
	DPRINT("IOCTL_DISK_CHECK_VERIFY\n");

	/* Initialize SRB operation */
	Srb->CdbLength = 6;
	Srb->TimeOutValue = DeviceExtension->TimeOutValue;
	Cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

	return(ScsiClassSendSrbAsynchronous(DeviceObject,
					    Srb,
					    Irp,
					    NULL,
					    0,
					    FALSE));

      default:
	DPRINT1("Unknown device io control code %lx\n",
		ModifiedControlCode);
	ExFreePool(Srb);

	/* Pass the IOCTL down to the port driver */
	NextStack = IoGetNextIrpStackLocation(Irp);
	NextStack->Parameters = Stack->Parameters;
	NextStack->MajorFunction = Stack->MajorFunction;
	NextStack->MinorFunction = Stack->MinorFunction;

	/* Call port driver */
	return(IoCallDriver(DeviceExtension->PortDeviceObject,
			    Irp));
    }

  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_UNSUCCESSFUL);
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiClassFindModePage(IN PCHAR ModeSenseBuffer,
		      IN ULONG Length,
		      IN UCHAR PageMode,
		      IN BOOLEAN Use6Byte)
{
  ULONG DescriptorLength;
  ULONG HeaderLength;
  PCHAR End;
  PCHAR Ptr;

  DPRINT("ScsiClassFindModePage() called\n");

  /* Get header length */
  HeaderLength = (Use6Byte) ? sizeof(MODE_PARAMETER_HEADER) : sizeof(MODE_PARAMETER_HEADER10);

  /* Check header length */
  if (Length < HeaderLength)
    return NULL;

  /* Get descriptor length */
  if (Use6Byte == TRUE)
    {
      DescriptorLength = ((PMODE_PARAMETER_HEADER)ModeSenseBuffer)->BlockDescriptorLength;
    }
  else
    {
      DescriptorLength = ((PMODE_PARAMETER_HEADER10)ModeSenseBuffer)->BlockDescriptorLength[1];
    }

  /* Set page pointers */
  Ptr = ModeSenseBuffer + HeaderLength + DescriptorLength;
  End = ModeSenseBuffer + Length;

  /* Search for page */
  while (Ptr < End)
    {
      /* Check page code */
      if (((PMODE_DISCONNECT_PAGE)Ptr)->PageCode == PageMode)
	return Ptr;

      /* Skip to next page */
      Ptr += ((PMODE_DISCONNECT_PAGE)Ptr)->PageLength;
    }

  return NULL;
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiClassFindUnclaimedDevices(IN PCLASS_INIT_DATA InitializationData,
			      IN PSCSI_ADAPTER_BUS_INFO AdapterInformation)
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


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassGetCapabilities(IN PDEVICE_OBJECT PortDeviceObject,
			 OUT PIO_SCSI_CAPABILITIES *PortCapabilities)
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


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassGetInquiryData(IN PDEVICE_OBJECT PortDeviceObject,
			IN PSCSI_ADAPTER_BUS_INFO *ConfigInfo)
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


/*
 * @implemented
 */
ULONG STDCALL
ScsiClassInitialize(IN PVOID Argument1,
		    IN PVOID Argument2,
		    IN PCLASS_INIT_DATA InitializationData)
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
  DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiClassInternalIoControl;
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
 *
 * @implemented
 */
VOID STDCALL
ScsiClassInitializeSrbLookasideList(IN PDEVICE_EXTENSION DeviceExtension,
				    IN ULONG NumberElements)
{
  ExInitializeNPagedLookasideList(&DeviceExtension->SrbLookasideListHead,
				  NULL,
				  NULL,
				  NonPagedPool,
				  sizeof(SCSI_REQUEST_BLOCK),
				  TAG_SRBT,
				  (USHORT)NumberElements);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
ScsiClassInternalIoControl(IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp)
{
  DPRINT1("ScsiClassInternalIoContol() called\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
BOOLEAN STDCALL
ScsiClassInterpretSenseInfo(IN PDEVICE_OBJECT DeviceObject,
			    IN PSCSI_REQUEST_BLOCK Srb,
			    IN UCHAR MajorFunctionCode,
			    IN ULONG IoDeviceCode,
			    IN ULONG RetryCount,
			    OUT NTSTATUS *Status)
{
  PDEVICE_EXTENSION DeviceExtension;
#if 0
  PIO_ERROR_LOG_PACKET LogPacket;
#endif
  PSENSE_DATA SenseData;
  NTSTATUS LogStatus;
  BOOLEAN LogError;
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
  LogError = FALSE;
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
	  case SCSI_SENSE_NO_SENSE:
	    DPRINT("SCSI_SENSE_NO_SENSE\n");
	    if (SenseData->IncorrectLength)
	      {
		DPRINT("Incorrect block length\n");
		*Status = STATUS_INVALID_BLOCK_LENGTH;
		Retry = FALSE;
	      }
	    else
	      {
		DPRINT("Unspecified error\n");
		*Status = STATUS_IO_DEVICE_ERROR;
		Retry = FALSE;
	      }
	    break;

	  case SCSI_SENSE_RECOVERED_ERROR:
	    DPRINT("SCSI_SENSE_RECOVERED_ERROR\n");
	    *Status = STATUS_SUCCESS;
	    Retry = FALSE;
	    break;

	  case SCSI_SENSE_NOT_READY:
	    DPRINT("SCSI_SENSE_NOT_READY\n");
	    *Status = STATUS_DEVICE_NOT_READY;
	    switch (SenseData->AdditionalSenseCode)
	      {
		case SCSI_ADSENSE_LUN_NOT_READY:
		  DPRINT("SCSI_ADSENSE_LUN_NOT_READY\n");
		  break;

		case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE:
		  DPRINT("SCSI_ADSENSE_NO_MEDIA_IN_DEVICE\n");
		  *Status = STATUS_NO_MEDIA_IN_DEVICE;
		  Retry = FALSE;
		  break;
	      }
	    break;

	  case SCSI_SENSE_MEDIUM_ERROR:
	    DPRINT("SCSI_SENSE_MEDIUM_ERROR\n");
	    *Status = STATUS_DEVICE_DATA_ERROR;
	    Retry = FALSE;
	    break;

	  case SCSI_SENSE_HARDWARE_ERROR:
	    DPRINT("SCSI_SENSE_HARDWARE_ERROR\n");
	    *Status = STATUS_IO_DEVICE_ERROR;
	    break;

	  case SCSI_SENSE_ILLEGAL_REQUEST:
	    DPRINT("SCSI_SENSE_ILLEGAL_REQUEST\n");
	    *Status = STATUS_INVALID_DEVICE_REQUEST;
	    switch (SenseData->AdditionalSenseCode)
	      {
		case SCSI_ADSENSE_ILLEGAL_COMMAND:
		  DPRINT("SCSI_ADSENSE_ILLEGAL_COMMAND\n");
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_ILLEGAL_BLOCK:
		  DPRINT("SCSI_ADSENSE_ILLEGAL_BLOCK\n");
		  *Status = STATUS_NONEXISTENT_SECTOR;
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_INVALID_LUN:
		  DPRINT("SCSI_ADSENSE_INVALID_LUN\n");
		  *Status = STATUS_NO_SUCH_DEVICE;
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_MUSIC_AREA:
		  DPRINT("SCSI_ADSENSE_MUSIC_AREA\n");
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_DATA_AREA:
		  DPRINT("SCSI_ADSENSE_DATA_AREA\n");
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_VOLUME_OVERFLOW:
		  DPRINT("SCSI_ADSENSE_VOLUME_OVERFLOW\n");
		  Retry = FALSE;
		  break;

		case SCSI_ADSENSE_INVALID_CDB:
		  DPRINT("SCSI_ADSENSE_INVALID_CDB\n");
		  Retry = FALSE;
		  break;
	      }
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

	  case SCSI_SENSE_DATA_PROTECT:
	    DPRINT("SCSI_SENSE_DATA_PROTECT\n");
	    *Status = STATUS_MEDIA_WRITE_PROTECTED;
	    Retry = FALSE;
	    break;

	  case SCSI_SENSE_ABORTED_COMMAND:
	    DPRINT("SCSI_SENSE_ABORTED_COMMAND\n");
	    *Status = STATUS_IO_DEVICE_ERROR;
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
	    LogError = TRUE;
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

  if (LogError == TRUE)
    {
#if 0
      /* Allocate error packet */
      LogPacket = IoAllocateErrorLogEntry (DeviceObject,
					   sizeof(IO_ERROR_LOG_PACKET) +
					     5 * sizeof(ULONG));
      if (LogPacket == NULL)
	{
	  DPRINT1 ("Failed to allocate a log packet!\n");
	  return Retry;
	}

      /* Initialize error packet */
      LogPacket->MajorFunctionCode = MajorFunctionCode;
      LogPacket->RetryCount = (UCHAR)RetryCount;
      LogPacket->DumpDataSize = 6 * sizeof(ULONG);
      LogPacket->ErrorCode = 0; /* FIXME */
      LogPacket->FinalStatus = *Status;
      LogPacket->IoControlCode = IoDeviceCode;
      LogPacket->DeviceOffset.QuadPart = 0; /* FIXME */
      LogPacket->DumpData[0] = Srb->PathId;
      LogPacket->DumpData[1] = Srb->TargetId;
      LogPacket->DumpData[2] = Srb->Lun;
      LogPacket->DumpData[3] = 0;
      LogPacket->DumpData[4] = (Srb->SrbStatus << 8) | Srb->ScsiStatus;
      if (SenseData != NULL)
	{
	  LogPacket->DumpData[5] = (SenseData->SenseKey << 16) |
				   (SenseData->AdditionalSenseCode << 8) |
				   SenseData->AdditionalSenseCodeQualifier;
	}

      /* Write error packet */
      IoWriteErrorLogEntry (LogPacket);
#endif
    }

  DPRINT("ScsiClassInterpretSenseInfo() done\n");

  return Retry;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassIoComplete(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp,
		    IN PVOID Context)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;
  BOOLEAN Retry;
  NTSTATUS Status;

  DPRINT("ScsiClassIoComplete(DeviceObject %p  Irp %p  Context %p) called\n",
	  DeviceObject, Irp, Context);

  DeviceExtension = DeviceObject->DeviceExtension;

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  /*
   * BUGBUG -> Srb = IrpStack->Parameters.Scsi.Srb;
   * Must pass Srb as Context arg!! See comment about Completion routines in 
   * IofCallDriver for more info.
   */

  Srb = (PSCSI_REQUEST_BLOCK)Context;

  DPRINT("Srb %p\n", Srb);

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
      if ((Retry) &&
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

  /* Free the SRB */
  ExFreeToNPagedLookasideList(&DeviceExtension->SrbLookasideListHead,
			      Srb);

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


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassIoCompleteAssociated(IN PDEVICE_OBJECT DeviceObject,
			      IN PIRP Irp,
			      IN PVOID Context)
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

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  /*
   * BUGBUG -> Srb = Srb = IrpStack->Parameters.Scsi.Srb;
   * Must pass Srb as Context arg!! See comment about Completion routines in 
   * IofCallDriver for more info.
   */

  Srb = (PSCSI_REQUEST_BLOCK)Context;
  
  DPRINT("Srb %p\n", Srb);

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

      if ((Retry) &&
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

  /* Free the SRB */
  ExFreeToNPagedLookasideList(&DeviceExtension->SrbLookasideListHead,
			      Srb);

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


/*
 * @implemented
 */
ULONG STDCALL
ScsiClassModeSense(IN PDEVICE_OBJECT DeviceObject,
		   IN PCHAR ModeSenseBuffer,
		   IN ULONG Length,
		   IN UCHAR PageMode)
{
  PDEVICE_EXTENSION DeviceExtension;
  SCSI_REQUEST_BLOCK Srb;
  ULONG RetryCount;
  PCDB Cdb;
  NTSTATUS Status;

  DPRINT("ScsiClassModeSense() called\n");

  DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
  RetryCount = 1;

  /* Initialize the SRB */
  RtlZeroMemory (&Srb,
		 sizeof(SCSI_REQUEST_BLOCK));
  Srb.CdbLength = 6;
  Srb.TimeOutValue = DeviceExtension->TimeOutValue;

  /* Initialize the CDB */
  Cdb = (PCDB)&Srb.Cdb;
  Cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  Cdb->MODE_SENSE.PageCode = PageMode;
  Cdb->MODE_SENSE.AllocationLength = (UCHAR)Length;

TryAgain:
  Status = ScsiClassSendSrbSynchronous (DeviceObject,
					&Srb,
					ModeSenseBuffer,
					Length,
					FALSE);
  if (Status == STATUS_VERIFY_REQUIRED)
    {
      if (RetryCount != 0)
	{
	  RetryCount--;
	  goto TryAgain;
	}
    }
  else if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
    {
      Status = STATUS_SUCCESS;
    }

  if (!NT_SUCCESS(Status))
    {
      return 0;
    }

  return Srb.DataTransferLength;
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiClassQueryTimeOutRegistryValue(IN PUNICODE_STRING RegistryPath)
{
  PRTL_QUERY_REGISTRY_TABLE Table;
  ULONG TimeOutValue;
  ULONG ZeroTimeOut;
  ULONG Size;
  PWSTR Path;
  NTSTATUS Status;

  if (RegistryPath == NULL)
    {
      return 0;
    }

  TimeOutValue = 0;
  ZeroTimeOut = 0;

  /* Allocate zero-terminated path string */
  Size = RegistryPath->Length + sizeof(WCHAR);
  Path = (PWSTR)ExAllocatePool (NonPagedPool,
				Size);
  if (Path == NULL)
    {
      return 0;
    }
  RtlZeroMemory (Path,
		 Size);
  RtlCopyMemory (Path,
		 RegistryPath->Buffer,
		 Size - sizeof(WCHAR));

  /* Allocate query table */
  Size = sizeof(RTL_QUERY_REGISTRY_TABLE) * 2;
  Table = (PRTL_QUERY_REGISTRY_TABLE)ExAllocatePool (NonPagedPool,
						     Size);
  if (Table == NULL)
    {
      ExFreePool (Path);
      return 0;
    }
  RtlZeroMemory (Table,
		 Size);

  Table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  Table[0].Name = L"TimeOutValue";
  Table[0].EntryContext = &TimeOutValue;
  Table[0].DefaultType = REG_DWORD;
  Table[0].DefaultData = &ZeroTimeOut;
  Table[0].DefaultLength = sizeof(ULONG);

  Status = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
				   Path,
				   Table,
				   NULL,
				   NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlQueryRegistryValue() failed (Status %lx)\n", Status);
      TimeOutValue = 0;
    }

  ExFreePool (Table);
  ExFreePool (Path);

  DPRINT("TimeOut: %lu\n", TimeOutValue);

  return TimeOutValue;
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
VOID STDCALL
ScsiClassReleaseQueue(IN PDEVICE_OBJECT DeviceObject)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ScsiClassSendSrbAsynchronous(PDEVICE_OBJECT DeviceObject,
			     PSCSI_REQUEST_BLOCK Srb,
			     PIRP Irp,
			     PVOID BufferAddress,
			     ULONG BufferLength,
			     BOOLEAN WriteToDevice)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;

  DPRINT("ScsiClassSendSrbAsynchronous() called\n");

  DeviceExtension = DeviceObject->DeviceExtension;

  /* Initialize the SRB */
  Srb->Length = SCSI_REQUEST_BLOCK_SIZE;
  Srb->PathId = DeviceExtension->PathId;
  Srb->TargetId = DeviceExtension->TargetId;
  Srb->Lun = DeviceExtension->Lun;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->Cdb[1] |= DeviceExtension->Lun << 5;

  Srb->SenseInfoBuffer = DeviceExtension->SenseData;
  Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

  Srb->DataBuffer = BufferAddress;
  Srb->DataTransferLength = BufferLength;

  Srb->ScsiStatus = 0;
  Srb->SrbStatus = 0;
  Srb->NextSrb = NULL;

  if (BufferAddress != NULL)
    {
      if (Irp->MdlAddress == NULL)
	{
	  /* Allocate an MDL */
	  if (!IoAllocateMdl(BufferAddress,
			     BufferLength,
			     FALSE,
			     FALSE,
			     Irp))
	    {
	      DPRINT1("Mdl-Allocation failed\n");
	      return(STATUS_INSUFFICIENT_RESOURCES);
	    }

	  MmBuildMdlForNonPagedPool(Irp->MdlAddress);
	}

      /* Set data direction */
      Srb->SrbFlags = (WriteToDevice) ? SRB_FLAGS_DATA_OUT : SRB_FLAGS_DATA_IN;
    }
  else
    {
      /* Set data direction */
      Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    }

  /* Set the retry counter */
  Stack = IoGetCurrentIrpStackLocation(Irp);
  Stack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

  /* Set the completion routine */
  IoSetCompletionRoutine(Irp,
			 ScsiClassIoComplete,
			 Srb,
			 TRUE,
			 TRUE,
			 TRUE);

  /* Attach Srb to the Irp */
  Stack = IoGetNextIrpStackLocation(Irp);
  Stack->MajorFunction = IRP_MJ_SCSI;
  Stack->Parameters.Scsi.Srb = Srb;
  Srb->OriginalRequest = Irp;

  /* Call the port driver */
  return(IoCallDriver(DeviceExtension->PortDeviceObject,
		      Irp));
}


/*
 * @implemented
 */
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
	  RequestType = IOCTL_SCSI_EXECUTE_IN;	// needs _in_ to the device
	  Srb->SrbFlags = SRB_FLAGS_DATA_OUT;	// needs _out_ from the caller
	}
      else
	{
	  RequestType = IOCTL_SCSI_EXECUTE_OUT;
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


/*
 * @implemented
 */
VOID STDCALL
ScsiClassSplitRequest(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp,
		      IN ULONG MaximumBytes)
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
  IrpStack->Parameters.Read.ByteOffset.QuadPart += 
    (DeviceExtension->StartingOffset.QuadPart + DeviceExtension->DMByteSkew);

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

      if (MaximumTransferLength > CurrentTransferPages * PAGE_SIZE)
	  MaximumTransferLength = CurrentTransferPages * PAGE_SIZE;

      if (MaximumTransferLength == 0)
	  MaximumTransferLength = PAGE_SIZE;

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

  if (CurrentIrpStack->MajorFunction != IRP_MJ_READ &&
      CurrentIrpStack->MajorFunction != IRP_MJ_WRITE)
    {
      /* We shouldn't setup the buffer pointer and transfer length on read/write requests. */
      if (Irp->MdlAddress != NULL)
        {
          TransferLength = Irp->MdlAddress->ByteCount;
        }
      else
        {
          TransferLength = 0;
        }

      Srb->DataBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
      Srb->DataTransferLength = TransferLength;
    }

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
