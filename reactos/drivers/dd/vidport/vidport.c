/* $Id: vidport.c,v 1.11 2000/03/01 03:25:11 ekohl Exp $
 *
 * VideoPort driver
 *   Written by Rex Jolliff
 */

#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#include "vidport.h"

#define UNIMPLEMENTED do {DbgPrint("%s:%d: Function not implemented", __FILE__, __LINE__); for(;;);} while (0)

#define VERSION "0.0.0"

static VOID VidStartIo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
static NTSTATUS VidDispatchOpenClose(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

//  -------------------------------------------------------  Public Interface

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

STDCALL NTSTATUS 
DriverEntry(IN PDRIVER_OBJECT DriverObject, 
            IN PUNICODE_STRING RegistryPath) 
{
  DbgPrint("VideoPort Driver %s\n", VERSION);
  return  STATUS_SUCCESS;
}

ULONG
STDCALL
VideoPortCompareMemory(IN PVOID  Source1, 
                       IN PVOID  Source2, 
                       IN ULONG  Length)
{
  return RtlCompareMemory(Source1, Source2, Length);
}

VOID 
STDCALL
VideoPortDebugPrint(IN ULONG DebugPrintLevel, 
                    IN PCHAR DebugMessage, ...)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortDisableInterrupt(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortEnableInterrupt(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortFreeDeviceBase(IN PVOID  HwDeviceExtension, 
                        IN PVOID  MappedAddress)
{
  UNIMPLEMENTED;
}

ULONG 
STDCALL
VideoPortGetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    OUT PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  return HalGetBusDataByOffset(BusDataType, 
                               0, 
                               SlotNumber, 
                               Buffer, 
                               Offset, 
                               Length);
}

UCHAR 
STDCALL
VideoPortGetCurrentIrql(VOID)
{
  return KeGetCurrentIrql();
}

PVOID 
STDCALL
VideoPortGetDeviceBase(IN PVOID  HwDeviceExtension,
                       IN PHYSICAL_ADDRESS  IoAddress,
                       IN ULONG  NumberOfUchars,
                       IN UCHAR  InIoSpace)
{
  if (InIoSpace)
    {
      return  MmMapIoSpace(IoAddress, NumberOfUchars, FALSE);
    }
  else
    {
      UNIMPLEMENTED;
      return  NULL;
    }
}

VP_STATUS 
STDCALL
VideoPortGetDeviceData(IN PVOID  HwDeviceExtension,
                       IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                       IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
                       IN PVOID Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortGetAccessRanges(IN PVOID  HwDeviceExtension,
                         IN ULONG  NumRequestedResources,
                         IN PIO_RESOURCE_DESCRIPTOR  RequestedResources OPTIONAL,
                         IN ULONG  NumAccessRanges,
                         IN PVIDEO_ACCESS_RANGE  AccessRanges,
                         IN PVOID  VendorId,
                         IN PVOID  DeviceId,
                         IN PULONG  Slot)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortGetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ParameterName,
                               IN UCHAR  IsParameterFileName,
                               IN PMINIPORT_GET_REGISTRY_ROUTINE  GetRegistryRoutine,
                               IN PVOID  Context)
{
  UNIMPLEMENTED;
}

ULONG 
STDCALL
VideoPortInitialize(IN PVOID  Context1,
                    IN PVOID  Context2,
                    IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
                    IN PVOID  HwContext)
{
  UCHAR  Again;
  WCHAR  DeviceBuffer[20];
  WCHAR  SymlinkBuffer[20];
  NTSTATUS  Status;
  PDRIVER_OBJECT  MPDriverObject = (PDRIVER_OBJECT) Context1;
  PDEVICE_OBJECT  MPDeviceObject;
  VIDEO_PORT_CONFIG_INFO  ConfigInfo;
  PVIDEOPORT_EXTENSION_DATA  ExtensionData;
  ULONG DeviceNumber = 0;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;

  /*  Build Dispatch table from passed data  */
  MPDriverObject->DriverStartIo = (PDRIVER_STARTIO) HwInitializationData->HwStartIO;

  /*  Create a unicode device name  */
  Again = FALSE;
  do
    {
      swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
      RtlInitUnicodeString(&DeviceName, DeviceBuffer);

      /*  Create the device  */
      Status = IoCreateDevice(MPDriverObject, 
                              HwInitializationData->HwDeviceExtensionSize +
                                sizeof(VIDEOPORT_EXTENSION_DATA),
                              &DeviceName, 
                              FILE_DEVICE_VIDEO, 
                              0, 
                              TRUE, 
                              &MPDeviceObject);
      if (!NT_SUCCESS(Status)) 
        {
          DbgPrint("IoCreateDevice call failed\n",0);
          return Status;
        }

      /* initialize the miniport drivers dispatch table */
      MPDriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatchOpenClose;
      MPDriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatchOpenClose;
      MPDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatchDeviceControl;

      /* create symbolic link "\??\DISPLAYx" */
      swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber+1);
      RtlInitUnicodeString (&SymlinkName,
                            SymlinkBuffer);
      IoCreateSymbolicLink (&SymlinkName,
                            &DeviceName);

      ExtensionData = 
        (PVIDEOPORT_EXTENSION_DATA) MPDeviceObject->DeviceExtension;
      ExtensionData->DeviceObject = MPDeviceObject;
      
      /*  Set the buffering strategy here...  */
      MPDeviceObject->Flags |= DO_BUFFERED_IO;

      /*  Call HwFindAdapter entry point  */
      /* FIXME: Need to figure out what string to pass as param 3  */
      Status = HwInitializationData->HwFindAdapter(VPExtensionToMPExtension(ExtensionData),
                                                   Context2,
                                                   L"",
                                                   &ConfigInfo,
                                                   &Again);
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("HwFindAdapter call failed");
          IoDeleteDevice(MPDeviceObject);

          return  Status;
        }

      /* FIXME: Allocate hardware resources for device  */

      /*  Allocate interrupt for device  */
      if (HwInitializationData->HwInterrupt != NULL &&
          !(ConfigInfo.BusInterruptLevel == 0 &&
            ConfigInfo.BusInterruptVector == 0))
        {
          ExtensionData->IRQL = ConfigInfo.BusInterruptLevel;
          ExtensionData->InterruptLevel = 
            HalGetInterruptVector(ConfigInfo.AdapterInterfaceType,
                                  ConfigInfo.SystemIoBusNumber,
                                  ConfigInfo.BusInterruptLevel,
                                  ConfigInfo.BusInterruptVector,
                                  &ExtensionData->IRQL,
                                  &ExtensionData->Affinity);
          KeInitializeSpinLock(&ExtensionData->InterruptSpinLock);
          Status = IoConnectInterrupt(&ExtensionData->InterruptObject,
                                      (PKSERVICE_ROUTINE)
                                        HwInitializationData->HwInterrupt,
                                      VPExtensionToMPExtension(ExtensionData),
                                      &ExtensionData->InterruptSpinLock,
                                      ExtensionData->InterruptLevel,
                                      ExtensionData->IRQL,
                                      ExtensionData->IRQL,
                                      ConfigInfo.InterruptMode,
                                      FALSE,
                                      ExtensionData->Affinity,
                                      FALSE);
          if (!NT_SUCCESS(Status))
            {
              DbgPrint("IoConnectInterrupt failed\n");
              IoDeleteDevice(MPDeviceObject);
              
              return Status;
            }
          
        }
      DeviceNumber++;
    }
  while (Again);


  /* FIXME: initialize timer routine for MP Driver  */
  if (HwInitializationData->HwTimer != NULL)
    {
      Status = IoInitializeTimer(MPDeviceObject,
                                 (PIO_TIMER_ROUTINE)
                                   HwInitializationData->HwTimer,
                                 VPExtensionToMPExtension(ExtensionData));
      if (!NT_SUCCESS(Status))
        {
          DbgPrint("IoInitializeTimer failed\n");
          
          if (HwInitializationData->HwInterrupt != NULL)
            {
              IoDisconnectInterrupt(ExtensionData->InterruptObject);
            }
          IoDeleteDevice(MPDeviceObject);
          
          return Status;
        }
    }
  
  return  STATUS_SUCCESS;
}

VP_STATUS 
STDCALL
VideoPortInt10(IN PVOID  HwDeviceExtension,
               IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments)
{
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortLogError(IN PVOID  HwDeviceExtension,
                  IN PVIDEO_REQUEST_PACKET  Vrp OPTIONAL,
                  IN VP_STATUS  ErrorCode,
                  IN ULONG  UniqueId)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortMapBankedMemory(IN PVOID  HwDeviceExtension,
                         IN PHYSICAL_ADDRESS  PhysicalAddress,
                         IN PULONG  Length,
                         IN PULONG  InIoSpace,
                         OUT PVOID  *VirtualAddress,
                         IN ULONG  BankLength,
                         IN UCHAR  ReadWriteBank,
                         IN PBANKED_SECTION_ROUTINE  BankRoutine,
                         IN PVOID  Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortMapMemory(IN PVOID  HwDeviceExtension,
                   IN PHYSICAL_ADDRESS  PhysicalAddress,
                   IN PULONG  Length,
                   IN PULONG  InIoSpace,
                   OUT PVOID  *VirtualAddress)
{
  if (*InIoSpace)
    {
      *VirtualAddress = MmMapIoSpace(PhysicalAddress, *Length, FALSE);
      
      return *VirtualAddress != NULL ? STATUS_SUCCESS : 
                                  STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      UNIMPLEMENTED;
    }
  
  return  STATUS_SUCCESS;
}

VOID 
STDCALL
VideoPortMoveMemory(OUT PVOID  Destination,
                    IN PVOID  Source,
                    IN ULONG  Length)
{
  RtlMoveMemory(Destination, Source, Length);
}

UCHAR 
STDCALL
VideoPortReadPortUchar(IN PUCHAR  Port)
{
  return  READ_PORT_UCHAR(Port);
}

USHORT 
STDCALL
VideoPortReadPortUshort(IN PUSHORT Port)
{
  return  READ_PORT_USHORT(Port);
}

ULONG 
STDCALL
VideoPortReadPortUlong(IN PULONG Port)
{
  return  READ_PORT_ULONG(Port);
}

VOID 
STDCALL
VideoPortReadPortBufferUchar(IN PUCHAR  Port, 
                             OUT PUCHAR  Buffer, 
                             IN ULONG  Count)
{
  READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadPortBufferUshort(IN PUSHORT Port, 
                              OUT PUSHORT Buffer, 
                              IN ULONG Count)
{
  READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadPortBufferUlong(IN PULONG Port, 
                             OUT PULONG Buffer, 
                             IN ULONG Count)
{
  READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

UCHAR 
STDCALL
VideoPortReadRegisterUchar(IN PUCHAR Register)
{
  return  READ_REGISTER_UCHAR(Register);
}

USHORT 
STDCALL
VideoPortReadRegisterUshort(IN PUSHORT Register)
{
  return  READ_REGISTER_USHORT(Register);
}

ULONG 
STDCALL
VideoPortReadRegisterUlong(IN PULONG Register)
{
  return  READ_REGISTER_ULONG(Register);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUchar(IN PUCHAR  Register, 
                                 OUT PUCHAR  Buffer, 
                                 IN ULONG  Count)
{
  READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUshort(IN PUSHORT  Register, 
                                  OUT PUSHORT  Buffer, 
                                  IN ULONG  Count)
{
  READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUlong(IN PULONG  Register, 
                                 OUT PULONG  Buffer, 
                                 IN ULONG  Count)
{
  READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

BOOLEAN 
STDCALL
VideoPortScanRom(IN PVOID  HwDeviceExtension, 
                 IN PUCHAR  RomBase,
                 IN ULONG  RomLength,
                 IN PUCHAR  String)
{
  UNIMPLEMENTED;
}

ULONG 
STDCALL
VideoPortSetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    IN PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  return  HalSetBusDataByOffset(BusDataType,
                                0,
                                SlotNumber,
                                Buffer,
                                Offset,
                                Length);
}

VP_STATUS 
STDCALL
VideoPortSetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ValueName,
                               IN PVOID  ValueData,
                               IN ULONG  ValueLength)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortSetTrappedEmulatorPorts(IN PVOID  HwDeviceExtension,
                                 IN ULONG  NumAccessRanges,
                                 IN PVIDEO_ACCESS_RANGE  AccessRange)
{
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortStallExecution(IN ULONG  Microseconds)
{
  KeStallExecutionProcessor(Microseconds);
}

VOID 
STDCALL
VideoPortStartTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  IoStartTimer(ExtensionData->DeviceObject);
}

VOID 
STDCALL
VideoPortStopTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  IoStopTimer(ExtensionData->DeviceObject);
}

BOOLEAN 
STDCALL
VideoPortSynchronizeExecution(IN PVOID  HwDeviceExtension,
                              IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
                              IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
                              OUT PVOID  Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortUnmapMemory(IN PVOID  HwDeviceExtension,
                     IN PVOID  VirtualAddress,
                     IN HANDLE  ProcessHandle)
{
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortVerifyAccessRanges(IN PVOID  HwDeviceExtension,
                            IN ULONG  NumAccessRanges,
                            IN PVIDEO_ACCESS_RANGE  AccessRanges)
{
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortWritePortUchar(IN PUCHAR  Port, 
                        IN UCHAR  Value)
{
  WRITE_PORT_UCHAR(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortUshort(IN PUSHORT  Port, 
                         IN USHORT  Value)
{
  WRITE_PORT_USHORT(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortUlong(IN PULONG Port, 
                        IN ULONG Value)
{
  WRITE_PORT_ULONG(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortBufferUchar(IN PUCHAR  Port, 
                              IN PUCHAR  Buffer, 
                              IN ULONG  Count)
{
  WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWritePortBufferUshort(IN PUSHORT  Port, 
                               IN PUSHORT  Buffer, 
                               IN ULONG  Count)
{
  WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWritePortBufferUlong(IN PULONG  Port, 
                              IN PULONG  Buffer, 
                              IN ULONG  Count)
{
  WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWriteRegisterUchar(IN PUCHAR  Register, 
                            IN UCHAR  Value)
{
  WRITE_REGISTER_UCHAR(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterUshort(IN PUSHORT  Register, 
                             IN USHORT  Value)
{
  WRITE_REGISTER_USHORT(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterUlong(IN PULONG  Register, 
                            IN ULONG  Value)
{
  WRITE_REGISTER_ULONG(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterBufferUchar(IN PUCHAR  Register, 
                                  IN PUCHAR  Buffer, 
                                  IN ULONG  Count)
{
  WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortWriteRegisterBufferUshort(IN PUSHORT  Register, 
                                   IN PUSHORT  Buffer, 
                                   IN ULONG  Count)
{
  WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortWriteRegisterBufferUlong(IN PULONG  Register, 
                                  IN PULONG  Buffer, 
                                  IN ULONG  Count)
{
  WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID
STDCALL
VideoPortZeroMemory(OUT PVOID  Destination, 
                         IN ULONG  Length)
{
  RtlZeroMemory (Destination, Length);
}

VOID
STDCALL
VideoPortZeroDeviceMemory(OUT PVOID  Destination, 
                               IN ULONG  Length)
{
  UNIMPLEMENTED;
}


//  -------------------------------------------  Nondiscardable statics

//    VidDispatchOpenClose
//
//  DESCRIPTION:
//    Answer requests for Open/Close calls: a null operation
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

static  NTSTATUS  
VidDispatchOpenClose(IN PDEVICE_OBJECT pDO, 
                     IN PIRP Irp) 
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

//    VidStartIo
//
//  DESCRIPTION:
//    Get the next requested I/O packet started
//
//  RUN LEVEL:
//    DISPATCH_LEVEL
//
//  ARGUMENTS:
//    Dispatch routine standard arguments
//
//  RETURNS:
//    NTSTATUS
//

static  VOID  
VidStartIo(IN PDEVICE_OBJECT DeviceObject, 
           IN PIRP Irp) 
{
  UNIMPLEMENTED;
}

//    VidDispatchDeviceControl
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

static  NTSTATUS  
VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, 
                         IN PIRP Irp) 
{
  UNIMPLEMENTED;
}


