/* $Id: cdrom.c,v 1.1 2002/01/31 15:00:00 ekohl Exp $
 *
 */

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>

#include "../include/scsi.h"
#include "../include/class2.h"
#include "../include/ntddscsi.h"

//#define NDEBUG
#include <debug.h>

#define VERSION "V0.0.1"



BOOLEAN STDCALL
CdromFindDevices(PDRIVER_OBJECT DriverObject,
		 PUNICODE_STRING RegistryPath,
		 PCLASS_INIT_DATA InitializationData,
		 PDEVICE_OBJECT PortDeviceObject,
		 ULONG PortNumber);


NTSTATUS STDCALL
CdromDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

NTSTATUS STDCALL
CdromShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);




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
  InitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);	// + sizeof(DISK_DATA)
  InitData.DeviceType = FILE_DEVICE_CD_ROM;
  InitData.DeviceCharacteristics = 0;

  InitData.ClassError = NULL;				// CdromProcessError;
  InitData.ClassReadWriteVerification = NULL;		// CdromReadWriteVerification;
  InitData.ClassFindDeviceCallBack = NULL;		// CdromDeviceVerification;
  InitData.ClassFindDevices = CdromFindDevices;
  InitData.ClassDeviceControl = CdromDeviceControl;
  InitData.ClassShutdownFlush = CdromShutdownFlush;
  InitData.ClassCreateClose = NULL;
  InitData.ClassStartIo = NULL;

  return(ScsiClassInitialize(DriverObject,
			     RegistryPath,
			     &InitData));
}


//    CdromFindDevices
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
CdromFindDevices(PDRIVER_OBJECT DriverObject,
		 PUNICODE_STRING RegistryPath,
		 PCLASS_INIT_DATA InitializationData,
		 PDEVICE_OBJECT PortDeviceObject,
		 ULONG PortNumber)
{
  PIO_SCSI_CAPABILITIES PortCapabilities;
  PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
  PCHAR Buffer;
#if 0
  ULONG DeviceCount;
#endif
  ULONG ScsiBus;
  NTSTATUS Status;
//  PCONFIGURATION_INFORMATION ConfigInfo;

  DPRINT1("CdromFindDevices() called.\n");

  /* Get port capabilities */
  Status = ScsiClassGetCapabilities(PortDeviceObject,
				    &PortCapabilities);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetCapabilities() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  DPRINT1("MaximumTransferLength: %lu\n", PortCapabilities->MaximumTransferLength);

  /* Get inquiry data */
  Status = ScsiClassGetInquiryData(PortDeviceObject,
				   (PSCSI_ADAPTER_BUS_INFO *)&Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetInquiryData() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  /* Check whether there are unclaimed devices */
  AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;
#if 0
  DeviceCount = ScsiClassFindUnclaimedDevices(InitializationData,
					      AdapterBusInfo);
  if (DeviceCount == 0)
    {
      DPRINT("ScsiClassFindUnclaimedDevices() returned 0!");
      return(FALSE);
    }
#endif

//  ConfigInfo = IoGetConfigurationInformation();
//  DPRINT1("Number of SCSI ports: %lu\n", ConfigInfo->ScsiPortCount);

  /* Search each bus of this adapter */
  for (ScsiBus = 0; ScsiBus < (ULONG)AdapterBusInfo->NumberOfBuses; ScsiBus++)
    {
      DPRINT("Searching bus %lu\n", ScsiBus);
#if 0
      lunInfo = (PVOID)(Buffer + adapterInfo->BusData[scsiBus].InquiryDataOffset);


      while (AdapterBusInfo->BusData[ScsiBus].InquiryDataOffset)
	{

	}
#endif
    }

  ExFreePool(Buffer);
  ExFreePool(PortCapabilities);

  return(TRUE);
}


//    CdromDeviceControl
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
CdromDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  DPRINT("CdromDeviceControl() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


//    CdromShutdownFlush
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
CdromShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  DPRINT("CdromShutdownFlush() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/* EOF */
