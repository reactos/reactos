
#include <ntddk.h>
#include <ntddvid.h>

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

    //  Export other driver entry points...
  DriverObject->DriverStartIo = VidtartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatchDeviceControl;

// FIXME: should the miniport driver be loaded here?

  return  STATUS_SUCCESS;
}

ULONG 
VideoPortCompareMemory(IN PVOID  Source1, 
                       IN PVOID  Source2, 
                       IN ULONG  Length)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortDebugPrint(IN ULONG DebugPrintLevel, 
                    IN PCHAR DebugMessage, ...)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortDisableInterrupt(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}
VP_STATUS 
VideoPortEnableInterrupt(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortFreeDeviceBase(IN PVOID  HwDeviceExtension, 
                        IN PVOID  MappedAddress)
{
  UNIMPLEMENTED;
}

ULONG 
VideoPortGetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    OUT PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  UNIMPLEMENTED;
}

UCHAR 
VideoPortGetCurrentIrql(VOID)
{
  UNIMPLEMENTED;
}

PVOID 
VideoPortGetDeviceBase(IN PVOID  HwDeviceExtension,
                       IN PHYSICAL_ADDRESS  IoAddress,
                       IN ULONG  NumberOfUchars,
                       IN UCHAR  InIoSpace)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortGetDeviceData(IN PVOID  HwDeviceExtension,
                       IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                       IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
                       IN PVOID Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
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
VideoPortGetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ParameterName,
                               IN UCHAR  IsParameterFileName,
                               IN PMINIPORT_GET_REGISTRY_ROUTINE  GetRegistryRoutine,
                               IN PVOID  Context)
{
  UNIMPLEMENTED;
}

ULONG 
VideoPortInitialize(IN PVOID  Context1,
                    IN PVOID  Context2,
                    IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
                    IN PVOID  HwContext)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortInt10(IN PVOID  HwDeviceExtension,
               IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortLogError(IN PVOID  HwDeviceExtension,
                  IN PVIDEO_REQUEST_PACKET  Vrp OPTIONAL,
                  IN VP_STATUS  ErrorCode,
                  IN ULONG  UniqueId)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortMapBankedMemory(IN PVOID  HwDeviceExtension,
                         IN PHYSICAL_ADDRESS  PhysicalAddress,
                         IN PULONG  Length,
                         IN PULONG  InIoSpace,
                         INOUT PVOID  *VirtualAddress,
                         IN ULONG  BankLength,
                         IN UCHAR  ReadWriteBank,
                         IN PBANKED_SECTION_ROUTINE  BankRoutine,
                         IN PVOID  Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortMapMemory(IN PVOID  HwDeviceExtension,
                   IN PHYSICAL_ADDRESS  PhysicalAddress,
                   IN PULONG  Length,
                   IN PULONG  InIoSpace,
                   INOUT PVOID  *VirtualAddress)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortMoveMemory(OUT PVOID  Destination,
                    IN PVOID  Source,
                    IN ULONG  Length)
{
  UNIMPLEMENTED;
}

UCHAR 
VideoPortReadPortUchar(IN PUCHAR  Port)
{
  UNIMPLEMENTED;
}

USHORT 
VideoPortReadPortUshort(IN PUSHORT Port)
{
  UNIMPLEMENTED;
}

ULONG 
VideoPortReadPortUlong(IN PULONG Port)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadPortBufferUchar(IN PUCHAR  Port, 
                             OUT PUCHAR  Buffer, 
                             IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadPortBufferUshort(IN PUSHORT Port, 
                              OUT PUSHORT Buffer, 
                              IN ULONG Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadPortBufferUlong(IN PULONG Port, 
                             OUT PULONG Buffer, 
                             IN ULONG Count)
{
  UNIMPLEMENTED;
}

UCHAR 
VideoPortReadRegisterUchar(IN PUCHAR Register)
{
  UNIMPLEMENTED;
}

USHORT 
VideoPortReadRegisterUshort(IN PUSHORT Register)
{
  UNIMPLEMENTED;
}

ULONG 
VideoPortReadRegisterUlong(IN PULONG Register)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadRegisterBufferUchar(IN PUCHAR  Register, 
                                 OUT PUCHAR  Buffer, 
                                 IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadRegisterBufferUshort(IN PUSHORT  Register, 
                                  OUT PUSHORT  Buffer, 
                                  IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortReadRegisterBufferUlong(IN PULONG  Register, 
                                 OUT PULONG  Buffer, 
                                 IN ULONG  Count)
{
  UNIMPLEMENTED;
}

BOOLEAN 
VideoPortScanRom(IN PVOID  HwDeviceExtension, 
                 IN PUCHAR  RomBase,
                 IN ULONG  RomLength,
                 IN PUCHAR  String)
{
  UNIMPLEMENTED;
}

ULONG 
VideoPortSetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    IN PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortSetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ValueName,
                               IN PVOID  ValueData,
                               IN ULONG  ValueLength)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortSetTrappedEmulatorPorts(IN PVOID  HwDeviceExtension,
                                 IN ULONG  NumAccessRanges,
                                 IN PVIDEO_ACCESS_RANGE  AccessRange)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortStallExecution(IN ULONG  Microseconds)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortStartTimer(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortStopTimer(IN PVOID  HwDeviceExtension)
{
  UNIMPLEMENTED;
}

BOOLEAN 
VideoPortSynchronizeExecution(IN PVOID  HwDeviceExtension,
                              IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
                              IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
                              INOUT PVOID  Context)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortUnmapMemory(IN PVOID  HwDeviceExtension,
                     IN PVOID  VirtualAddress,
                     IN HANDLE  ProcessHandle)
{
  UNIMPLEMENTED;
}

VP_STATUS 
VideoPortVerifyAccessRanges(IN PVOID  HwDeviceExtension,
                            IN ULONG  NumAccessRanges,
                            IN PVIDEO_ACCESS_RANGE  AccessRanges)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortUchar(IN PUCHAR  Port, 
                        IN UCHAR  Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortUshort(IN PUSHORT  Port, 
                         IN USHORT  Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortUlong(IN PULONG Port, 
                        IN ULONG Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortBufferUchar(IN PUCHAR  Port, 
                              IN PUCHAR  Buffer, 
                              IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortBufferUshort(IN PUSHORT  Port, 
                               IN PUSHORT  Buffer, 
                               IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWritePortBufferUlong(IN PULONG  Port, 
                              IN PULONG  Buffer, 
                              IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterUchar(IN PUCHAR  Register, 
                            IN UCHAR  Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterUshort(IN PUSHORT  Register, 
                             IN USHORT  Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterUlong(IN PULONG  Register, 
                            IN ULONG  Value)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterBufferUchar(IN PUCHAR  Register, 
                                  IN PUCHAR  Buffer, 
                                  IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterBufferUshort(IN PUSHORT  Register, 
                                   IN PUSHORT  Buffer, 
                                   IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID 
VideoPortWriteRegisterBufferUlong(IN PULONG  Register, 
                                  IN PULONG  Buffer, 
                                  IN ULONG  Count)
{
  UNIMPLEMENTED;
}

VOID VideoPortZeroMemory(OUT PVOID  Destination, 
                         IN ULONG  Length)
{
  UNIMPLEMENTED;
}

VOID VideoPortZeroDeviceMemory(OUT PVOID  Destination, 
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


