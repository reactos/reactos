/* $Id: disk.c,v 1.2 2002/01/14 01:44:03 ekohl Exp $
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


//static NTSTATUS
//DiskCreateDevices(VOID);


BOOLEAN STDCALL
DiskClassFindDevices(PDRIVER_OBJECT DriverObject,
		     PUNICODE_STRING RegistryPath,
		     PCLASS_INIT_DATA InitializationData,
		     PDEVICE_OBJECT PortDeviceObject,
		     ULONG PortNumber);


NTSTATUS STDCALL
DiskClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp);

NTSTATUS STDCALL
DiskClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
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

  DbgPrint("Disk Class Driver %s\n", VERSION);

  InitData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
  InitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);	// + sizeof(DISK_DATA)
  InitData.DeviceType = FILE_DEVICE_DISK;
  InitData.DeviceCharacteristics = 0;

  InitData.ClassError = NULL;				// DiskClassProcessError;
  InitData.ClassReadWriteVerification = NULL;		// DiskClassReadWriteVerification;
  InitData.ClassFindDeviceCallBack = NULL;		// DiskClassDeviceVerification;
  InitData.ClassFindDevices = DiskClassFindDevices;
  InitData.ClassDeviceControl = DiskClassDeviceControl;
  InitData.ClassShutdownFlush = DiskClassShutdownFlush;
  InitData.ClassCreateClose = NULL;
  InitData.ClassStartIo = NULL;

  return(ScsiClassInitialize(DriverObject,
			     RegistryPath,
			     &InitData));
}


//    DiskClassFindDevices
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
DiskClassFindDevices(PDRIVER_OBJECT DriverObject,
		     PUNICODE_STRING RegistryPath,
		     PCLASS_INIT_DATA InitializationData,
		     PDEVICE_OBJECT PortDeviceObject,
		     ULONG PortNumber)
{
  PIO_SCSI_CAPABILITIES PortCapabilities = NULL;
  PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
  PCHAR Buffer;
  ULONG DeviceCount;
  NTSTATUS Status;
//  PCONFIGURATION_INFORMATION ConfigInfo;

  DPRINT1("DiskClassFindDevices() called.\n");

  /* Get port capabilities */
  Status = ScsiClassGetCapabilities(PortDeviceObject,
				    &PortCapabilities);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetCapabilities() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

//  DPRINT1("MaximumTransferLength: %lu\n", Capabilities.MaximumTransferLength);

  /* Get inquiry data */
  Status = ScsiClassGetInquiryData(PortDeviceObject,
				   (PSCSI_ADAPTER_BUS_INFO *)&Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ScsiClassGetInquiryData() failed! (Status 0x%lX)\n", Status);
      return(FALSE);
    }

  /* Get number of unclaimed devices */
  AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;
  DeviceCount = ScsiClassFindUnclaimedDevices(InitializationData,
					      AdapterBusInfo);
  if (DeviceCount == 0)
    {
      DPRINT("ScsiClassFindUnclaimedDevices() returned 0!");
      return(FALSE);
    }

//  ConfigInfo = IoGetConfigurationInformation();
//  DPRINT1("Number of SCSI ports: %lu\n", ConfigInfo->ScsiPortCount);




  ExFreePool(Buffer);

  return(TRUE);
}


//    IDECreateDevice
//
//  DESCRIPTION:
//    Creates a device by calling IoCreateDevice and a symbolic link for Win32
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   PDRIVER_OBJECT      DriverObject      The system supplied driver object
//    OUT  PDEVICE_OBJECT     *DeviceObject      The created device object
//    IN   PCONTROLLER_OBJECT  ControllerObject  The Controller for the device
//    IN   BOOLEAN             LBASupported      Does the drive support LBA addressing?
//    IN   BOOLEAN             DMASupported      Does the drive support DMA?
//    IN   int                 SectorsPerLogCyl  Sectors per cylinder
//    IN   int                 SectorsPerLogTrk  Sectors per track
//    IN   DWORD               Offset            First valid sector for this device
//    IN   DWORD               Size              Count of valid sectors for this device
//
//  RETURNS:
//    NTSTATUS
//
#if 0
NTSTATUS
IDECreateDevice(IN PDRIVER_OBJECT DriverObject,
                OUT PDEVICE_OBJECT *DeviceObject,
                IN PCONTROLLER_OBJECT ControllerObject,
                IN int UnitNumber,
                IN ULONG DiskNumber,
                IN ULONG PartitionNumber,
                IN PIDE_DRIVE_IDENTIFY DrvParms,
                IN DWORD Offset,
                IN DWORD Size)
{
  WCHAR                  NameBuffer[IDE_MAX_NAME_LENGTH];
  WCHAR                  ArcNameBuffer[IDE_MAX_NAME_LENGTH + 15];
  UNICODE_STRING         DeviceName;
  UNICODE_STRING         ArcName;
  NTSTATUS               RC;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

    // Create a unicode device name
  swprintf(NameBuffer,
           L"\\Device\\Harddisk%d\\Partition%d",
           DiskNumber,
           PartitionNumber);
  RtlInitUnicodeString(&DeviceName,
                       NameBuffer);

    // Create the device
  RC = IoCreateDevice(DriverObject, sizeof(IDE_DEVICE_EXTENSION),
      &DeviceName, FILE_DEVICE_DISK, 0, TRUE, DeviceObject);
  if (!NT_SUCCESS(RC))
    {
      DPRINT("IoCreateDevice call failed\n",0);
      return RC;
    }

    //  Set the buffering strategy here...
  (*DeviceObject)->Flags |= DO_DIRECT_IO;
  (*DeviceObject)->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    //  Fill out Device extension data
  DeviceExtension = (PIDE_DEVICE_EXTENSION) (*DeviceObject)->DeviceExtension;
  DeviceExtension->DeviceObject = (*DeviceObject);
  DeviceExtension->ControllerObject = ControllerObject;
  DeviceExtension->UnitNumber = UnitNumber;
  DeviceExtension->LBASupported = 
    (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0;
  DeviceExtension->DMASupported = 
    (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0;
    // FIXME: deal with bizarre sector sizes
  DeviceExtension->BytesPerSector = 512 /* DrvParms->BytesPerSector */;
  DeviceExtension->SectorsPerLogCyl = DrvParms->LogicalHeads *
      DrvParms->SectorsPerTrack;
  DeviceExtension->SectorsPerLogTrk = DrvParms->SectorsPerTrack;
  DeviceExtension->LogicalHeads = DrvParms->LogicalHeads;
  DeviceExtension->Offset = Offset;
  DeviceExtension->Size = Size;
  DPRINT("%wZ: offset %d size %d \n",
         &DeviceName,
         DeviceExtension->Offset,
         DeviceExtension->Size);

    //  Initialize the DPC object here
  IoInitializeDpcRequest(*DeviceObject, IDEDpcForIsr);

  if (PartitionNumber != 0)
    {
      DbgPrint("%wZ %dMB\n", &DeviceName, Size / 2048);
    }

  /* assign arc name */
  if (PartitionNumber == 0)
    {
      swprintf(ArcNameBuffer,
               L"\\ArcName\\multi(0)disk(0)rdisk(%d)",
               DiskNumber);
    }
  else
    {
      swprintf(ArcNameBuffer,
               L"\\ArcName\\multi(0)disk(0)rdisk(%d)partition(%d)",
               DiskNumber,
               PartitionNumber);
    }
  RtlInitUnicodeString (&ArcName,
                        ArcNameBuffer);
  DPRINT("%wZ ==> %wZ\n", &ArcName, &DeviceName);
  RC = IoAssignArcName (&ArcName,
                        &DeviceName);
  if (!NT_SUCCESS(RC))
    {
      DPRINT("IoAssignArcName (%wZ) failed (Status %x)\n",
             &ArcName, RC);
    }


  return  RC;
}
#endif




//    DiskClassDeviceControl
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
DiskClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  DPRINT("DiskClassDeviceControl() called!\n");

#if 0
  NTSTATUS  RC;
  ULONG     ControlCode, InputLength, OutputLength;
  PIO_STACK_LOCATION     IrpStack;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

  RC = STATUS_SUCCESS;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
  InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //  A huge switch statement in a Windows program?! who would have thought?
  switch (ControlCode) 
    {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
      if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)) 
        {
          Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        } 
      else 
        {
          PDISK_GEOMETRY Geometry;

          Geometry = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;
          Geometry->MediaType = FixedMedia;
              // FIXME: should report for RawDevice even on partition
          Geometry->Cylinders.QuadPart = DeviceExtension->Size / 
              DeviceExtension->SectorsPerLogCyl;
          Geometry->TracksPerCylinder = DeviceExtension->SectorsPerLogTrk /
              DeviceExtension->SectorsPerLogCyl;
          Geometry->SectorsPerTrack = DeviceExtension->SectorsPerLogTrk;
          Geometry->BytesPerSector = DeviceExtension->BytesPerSector;

          Irp->IoStatus.Status = STATUS_SUCCESS;
          Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }
      break;

    case IOCTL_DISK_GET_PARTITION_INFO:
    case IOCTL_DISK_SET_PARTITION_INFO:
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
    case IOCTL_DISK_SET_DRIVE_LAYOUT:
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

      //  If we get here, something went wrong.  inform the requestor
    default:
      RC = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Status = RC;
      Irp->IoStatus.Information = 0;
      break;
    }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return  RC;
#endif

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


//    DiskClassShutdownFlush
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
DiskClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
		       IN PIRP Irp)
{
  DPRINT("DiskClassShutdownFlush() called!\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/* EOF */
