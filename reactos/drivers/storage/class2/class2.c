/* $Id: class2.c,v 1.3 2002/01/27 01:25:01 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/class2/class2.c
 * PURPOSE:         SCSI class driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/*
 * TODO:
 *	- ScsiClassClaimDevice() must send a claim/unclaim request to
 *	  scsiport.sys.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include "../include/scsi.h"
#include "../include/class2.h"

#define NDEBUG
#include <debug.h>


#define VERSION "0.0.1"

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


/* FUNCTIONS ****************************************************************/

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver.
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
  DbgPrint("Class Driver %s\n", VERSION);
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
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassClaimDevice(PDEVICE_OBJECT PortDeviceObject,
		     PSCSI_INQUIRY_DATA LunInfo,
		     BOOLEAN Release,
		     PDEVICE_OBJECT *NewPortDeviceObject OPTIONAL)
{
  NTSTATUS Status;

  DPRINT1("ScsiClassClaimDevice() called\n");

  if (NewPortDeviceObject != NULL)
    *NewPortDeviceObject = NULL;

  if (Release == TRUE)
    {
      ObDereferenceObject(PortDeviceObject);
      return(STATUS_SUCCESS);
    }

  ObReferenceObjectByPointer(PortDeviceObject,
			     0,
			     NULL,
			     KernelMode);

  if (NewPortDeviceObject != NULL)
    *NewPortDeviceObject = PortDeviceObject;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
ScsiClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
			    IN PCCHAR ObjectNameBuffer,
			    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
			    IN OUT PDEVICE_OBJECT *DeviceObject,
			    IN PCLASS_INIT_DATA InitializationData)
{
  DPRINT1("ScsiClassCreateDeviceObject() called\n");

#if 0
  Status = IoCreateDevice(DriverObject,
			  InitializationData->DeviceExtensionSize,
			  &UnicodeName,
			  InitializationData->DeviceType,
			  InitializationData->DeviceCharacteristics,
			  FALSE,
			  &InternalDeviceObject);
#endif


  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
ScsiClassDeviceControl(PDEVICE_OBJECT DeviceObject,
		       PIRP Irp)
{
  UNIMPLEMENTED;
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

  DPRINT("ScsiClassFindUnclaimedDevices() called!\n");

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
  PIO_SCSI_CAPABILITIES Buffer;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  *PortCapabilities = NULL;
  Buffer = ExAllocatePool(NonPagedPool, /* FIXME: use paged pool */
			  sizeof(IO_SCSI_CAPABILITIES));
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_CAPABILITIES,
				      PortDeviceObject,
				      NULL,
				      0,
				      Buffer,
				      sizeof(IO_SCSI_CAPABILITIES),
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
      *PortCapabilities = Buffer;
    }

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
	  DPRINT1("ScsiPort%lu found.\n", PortNumber);

	  /* Check scsi port for attached disk drives */
	  if (InitializationData->ClassFindDevices(DriverObject,
						   Argument2,
						   InitializationData,
						   PortDeviceObject,
						   PortNumber))
	    {
	      DiskFound = TRUE;
	    }
	}
    }

  DPRINT("ScsiClassInitialize() done!\n");
for(;;);

  return((DiskFound == TRUE) ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE);
}


VOID STDCALL
ScsiClassInitializeSrbLookasideList(PDEVICE_EXTENSION DeviceExtension,
				    ULONG NumberElements)
{
  UNIMPLEMENTED;
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
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassIoComplete(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp,
		    PVOID Context)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ScsiClassIoCompleteAssociated(PDEVICE_OBJECT DeviceObject,
			      PIRP Irp,
			      PVOID Context)
{
  UNIMPLEMENTED;
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
  UNIMPLEMENTED;
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
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiClassSplitRequest(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      ULONG MaximumBytes)
{
  UNIMPLEMENTED;
}


/* INTERNAL FUNCTIONS *******************************************************/

static NTSTATUS STDCALL
ScsiClassCreateClose(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
//  PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

//  if (deviceExtension->ClassCreateClose)
//    return(deviceExtension->ClassCreateClose(DeviceObject, Irp));

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassReadWrite(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassScsiDispatch(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassDeviceDispatch(IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}

/* EOF */
