/*
 * NTDDVID.H - Video Port and MiniPort driver interface
 */

#include <ddk/miniport.h>

/*
 * the rough idea:
 *  init:
 *    miniport driver defines entrypoint thusly:
 *      ULONG  DriverEntry(PVOID Context1, PVOID Context2);
 *    miniport allocates and initializes a VIDEO_HW_INIT_DATA struct
 *    miniport calls VideoPortInitialize
 *    video port driver handles init of DriverObject (Context1)
 *    video port driver calls back into HwVidFindAdapter entry point
 *    video port driver finishes up and returns the status code that
 *      the miniport driver should return.
 *  io requests:
 *    video port driver reformats IRP into VRP
 *    video port driver calls back into HwVidStartIO entry point
 *    minimum IoControlCodes that must be handles by the miniport:
 *      IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES
 *      IOCTL_VIDEO_QUERY_AVAIL_MODES
 *      IOCTL_VIDEO_SET_CURRENT_MODE
 *      IOCTL_VIDEO_MAP_MEMORY
 *      IOCTL_VIDEO_RESET_DEVICE
 *  interrupts are handled the same as KM drivers.
 */

typedef LONG VP_STATUS, *PVP_STATUS;

#define VIDEO_MEMORY_SPACE_MEMORY    0x00
#define VIDEO_MEMORY_SPACE_IO        0x01
#define VIDEO_MEMORY_SPACE_USER_MODE 0x02
#define VIDEO_MEMORY_SPACE_DENSE     0x04
#define VIDEO_MEMORY_SPACE_P6CACHE   0x08

typedef enum _VIDEO_DEVICE_DATA_TYPE 
{
  VpMachineData,
  VpCmosData,
  VpBusData,
  VpControllerData,
  VpMonitorData
} VIDEO_DEVICE_DATA_TYPE, *PVIDEO_DEVICE_DATA_TYPE;

typedef enum _VIDEO_SYNCHRONIZE_PRIORITY 
{
  VpLowPriority,
  VpMediumPriority,
  VpHighPriority
} VIDEO_SYNCHRONIZE_PRIORITY, *PVIDEO_SYNCHRONIZE_PRIORITY;

typedef struct _STATUS_BLOCK 
{
  VP_STATUS  Status;
  ULONG  Information;
} STATUS_BLOCK, *PSTATUS_BLOCK;

typedef struct _VIDEO_REQUEST_PACKET 
{
  ULONG  IoControlCode;
  PSTATUS_BLOCK  StatusBlock;
  PVOID  InputBuffer;
  ULONG  InputBufferLength;
  PVOID  OutputBuffer;
  ULONG  OutputBufferLength;
} VIDEO_REQUEST_PACKET, *PVIDEO_REQUEST_PACKET;

typedef struct _VIDEO_PORT_CONFIG_INFO 
{
  ULONG  Length;
  ULONG  SystemIoBusNumber;
  INTERFACE_TYPE  AdapterInterfaceType;
  ULONG  BusInterruptLevel;
  ULONG  BusInterruptVector;
  KINTERRUPT_MODE  InterruptMode;
  ULONG  NumEmulatorAccessEntries;
  PEMULATOR_ACCESS_ENTRY  EmulatorAccessEntries;
  ULONG  EmulatorAccessEntriesContext;
  PHYSICAL_ADDRESS  VdmPhysicalVideoMemoryAddress;
  ULONG  VdmPhysicalVideoMemoryLength;
  ULONG  HardwareStateSize;
  ULONG  DmaChannel;
  ULONG  DmaPort;
  UCHAR  DmaShareable;
  UCHAR  InterruptShareable;
} VIDEO_PORT_CONFIG_INFO, *PVIDEO_PORT_CONFIG_INFO;

typedef VP_STATUS  (*PVIDEO_HW_FIND_ADAPTER) (PVOID  HwDeviceExtension,
                                              PVOID  HwContext,
                                              PWSTR  ArgumentString,
                                              PVIDEO_PORT_CONFIG_INFO  ConfigInfo,
                                              PUCHAR  Again);
typedef BOOLEAN  (*PVIDEO_HW_INITIALIZE)(PVOID  HwDeviceExtension);
typedef BOOLEAN  (*PVIDEO_HW_INTERRUPT)(PVOID  HwDeviceExtension);
typedef BOOLEAN  (*PVIDEO_HW_START_IO)(PVOID  HwDeviceExtension,
                                       PVIDEO_REQUEST_PACKET  RequestPacket);
typedef BOOLEAN  (*PVIDEO_HW_RESET_HW)(PVOID  HwDeviceExtension,
                                       ULONG  Columns,
                                       ULONG  Rows);
typedef VOID  (*PVIDEO_HW_TIMER)(PVOID  HwDeviceExtension);

typedef struct _VIDEO_HW_INITIALIZATION_DATA 
{
  ULONG  HwInitDataSize;
  INTERFACE_TYPE  AdapterInterfaceType;
  PVIDEO_HW_FIND_ADAPTER  HwFindAdapter;
  PVIDEO_HW_INITIALIZE  HwInitialize;
  PVIDEO_HW_INTERRUPT  HwInterrupt;
  PVIDEO_HW_START_IO  HwStartIO;
  ULONG  HwDeviceExtensionSize;
  ULONG  StartingDeviceNumber;
  PVIDEO_HW_RESET_HW  HwResetHw;
  PVIDEO_HW_TIMER  HwTimer;
} VIDEO_HW_INITIALIZATION_DATA, *PVIDEO_HW_INITIALIZATION_DATA;

typedef VP_STATUS (*PMINIPORT_GET_REGISTRY_ROUTINE)(PVOID  HwDeviceExtension,
                                                    PVOID  Context,
                                                    PWSTR  ValueName,
                                                    PVOID  ValueData,
                                                    ULONG  ValueLength);
typedef VP_STATUS (*PMINIPORT_QUERY_DEVICE_ROUTINE)(PVOID  HwDeviceExtension,
                                                    PVOID  Context,
                                                    VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                                                    PVOID  Identifier,
                                                    ULONG  IdentiferLength,
                                                    PVOID  ConfigurationData,
                                                    ULONG  ConfigurationDataLength,
                                                    PVOID  ComponentInformation,
                                                    ULONG  ComponentInformationLength);
typedef BOOLEAN (*PMINIPORT_SYNCHRONIZE_ROUTINE)(PVOID  Context);

typedef struct _VIDEO_ACCESS_RANGE 
{
  PHYSICAL_ADDRESS  RangeStart;
  ULONG  RangeLength;
  UCHAR  RangeInIoSpace;
  UCHAR  RangeVisible;
  UCHAR  RangeShareable;
} VIDEO_ACCESS_RANGE, *PVIDEO_ACCESS_RANGE;

typedef struct _VIDEO_X86_BIOS_ARGUMENTS 
{
  ULONG  Eax;
  ULONG  Ebx;
  ULONG  Ecx;
  ULONG  Edx;
  ULONG  Esi;
  ULONG  Edi;
  ULONG  Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;

typedef VOID (*PBANKED_SECTION_ROUTINE)(IN ULONG  ReadBank, IN ULONG  WriteBank, IN PVOID  Context);

ULONG STDCALL VideoPortCompareMemory(IN PVOID  Source1, IN PVOID  Source2, IN ULONG  Length);
VOID STDCALL VideoPortDebugPrint(IN ULONG DebugPrintLevel, IN PCHAR DebugMessage, ...);
VP_STATUS STDCALL VideoPortDisableInterrupt(IN PVOID  HwDeviceExtension);
VP_STATUS STDCALL VideoPortEnableInterrupt(IN PVOID  HwDeviceExtension);
VOID STDCALL VideoPortFreeDeviceBase(IN PVOID  HwDeviceExtension, IN PVOID  MappedAddress);
ULONG STDCALL VideoPortGetBusData(IN PVOID  HwDeviceExtension,
                          IN BUS_DATA_TYPE  BusDataType,
                          IN ULONG  SlotNumber,
                          OUT PVOID  Buffer,
                          IN ULONG  Offset,
                          IN ULONG  Length);
UCHAR STDCALL VideoPortGetCurrentIrql(VOID);
PVOID STDCALL VideoPortGetDeviceBase(IN PVOID  HwDeviceExtension,
                             IN PHYSICAL_ADDRESS  IoAddress,
                             IN ULONG  NumberOfUchars,
                             IN UCHAR  InIoSpace);
VP_STATUS STDCALL VideoPortGetDeviceData(IN PVOID  HwDeviceExtension,
                                 IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                                 IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
                                 IN PVOID Context);
VP_STATUS STDCALL VideoPortGetAccessRanges(IN PVOID  HwDeviceExtension,
                                   IN ULONG  NumRequestedResources,
                                   IN PIO_RESOURCE_DESCRIPTOR  RequestedResources OPTIONAL,
                                   IN ULONG  NumAccessRanges,
                                   IN PVIDEO_ACCESS_RANGE  AccessRanges,
                                   IN PVOID  VendorId,
                                   IN PVOID  DeviceId,
                                   IN PULONG  Slot);
VP_STATUS STDCALL VideoPortGetRegistryParameters(IN PVOID  HwDeviceExtension,
                                         IN PWSTR  ParameterName,
                                         IN UCHAR  IsParameterFileName,
                                         IN PMINIPORT_GET_REGISTRY_ROUTINE  GetRegistryRoutine,
                                         IN PVOID  Context);
ULONG STDCALL VideoPortInitialize(IN PVOID  Context1,
                          IN PVOID  Context2,
                          IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
                          IN PVOID  HwContext);
VP_STATUS STDCALL VideoPortInt10(IN PVOID  HwDeviceExtension,
                         IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments);
VOID STDCALL VideoPortLogError(IN PVOID  HwDeviceExtension,
                       IN PVIDEO_REQUEST_PACKET  Vrp OPTIONAL,
                       IN VP_STATUS  ErrorCode,
                       IN ULONG  UniqueId);
VP_STATUS STDCALL VideoPortMapBankedMemory(IN PVOID  HwDeviceExtension,
                                   IN PHYSICAL_ADDRESS  PhysicalAddress,
                                   IN PULONG  Length,
                                   IN PULONG  InIoSpace,
                                   OUT PVOID  *VirtualAddress,
                                   IN ULONG  BankLength,
                                   IN UCHAR  ReadWriteBank,
                                   IN PBANKED_SECTION_ROUTINE  BankRoutine,
                                   IN PVOID  Context);
VP_STATUS STDCALL VideoPortMapMemory(IN PVOID  HwDeviceExtension,
                             IN PHYSICAL_ADDRESS  PhysicalAddress,
                             IN PULONG  Length,
                             IN PULONG  InIoSpace,
                             OUT PVOID  *VirtualAddress);
VOID STDCALL VideoPortMoveMemory(OUT PVOID  Destination,
                         IN PVOID  Source,
                         IN ULONG  Length);
UCHAR STDCALL VideoPortReadPortUchar(IN PUCHAR  Port);
USHORT STDCALL VideoPortReadPortUshort(IN PUSHORT Port);
ULONG STDCALL VideoPortReadPortUlong(IN PULONG Port);
VOID STDCALL VideoPortReadPortBufferUchar(IN PUCHAR  Port, OUT PUCHAR  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortReadPortBufferUshort(IN PUSHORT Port, OUT PUSHORT Buffer, IN ULONG Count);
VOID STDCALL VideoPortReadPortBufferUlong(IN PULONG Port, OUT PULONG Buffer, IN ULONG Count);
UCHAR STDCALL VideoPortReadRegisterUchar(IN PUCHAR Register);
USHORT STDCALL VideoPortReadRegisterUshort(IN PUSHORT Register);
ULONG STDCALL VideoPortReadRegisterUlong(IN PULONG Register);
VOID STDCALL VideoPortReadRegisterBufferUchar(IN PUCHAR  Register, OUT PUCHAR  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortReadRegisterBufferUshort(IN PUSHORT  Register, OUT PUSHORT  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortReadRegisterBufferUlong(IN PULONG  Register, OUT PULONG  Buffer, IN ULONG  Count);
BOOLEAN STDCALL VideoPortScanRom(IN PVOID  HwDeviceExtension, 
                         IN PUCHAR  RomBase,
                         IN ULONG  RomLength,
                         IN PUCHAR  String);
ULONG STDCALL VideoPortSetBusData(IN PVOID  HwDeviceExtension,
                          IN BUS_DATA_TYPE  BusDataType,
                          IN ULONG  SlotNumber,
                          IN PVOID  Buffer,
                          IN ULONG  Offset,
                          IN ULONG  Length);
VP_STATUS STDCALL VideoPortSetRegistryParameters(IN PVOID  HwDeviceExtension,
                                         IN PWSTR  ValueName,
                                         IN PVOID  ValueData,
                                         IN ULONG  ValueLength);
VP_STATUS STDCALL VideoPortSetTrappedEmulatorPorts(IN PVOID  HwDeviceExtension,
                                           IN ULONG  NumAccessRanges,
                                           IN PVIDEO_ACCESS_RANGE  AccessRange);
VOID STDCALL VideoPortStallExecution(IN ULONG  Microseconds);
VOID STDCALL VideoPortStartTimer(IN PVOID  HwDeviceExtension);
VOID STDCALL VideoPortStopTimer(IN PVOID  HwDeviceExtension);
BOOLEAN STDCALL VideoPortSynchronizeExecution(IN PVOID  HwDeviceExtension,
                                      IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
                                      IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
                                      OUT PVOID  Context);
VP_STATUS STDCALL VideoPortUnmapMemory(IN PVOID  HwDeviceExtension,
                               IN PVOID  VirtualAddress,
                               IN HANDLE  ProcessHandle);
VP_STATUS STDCALL VideoPortVerifyAccessRanges(IN PVOID  HwDeviceExtension,
                                      IN ULONG  NumAccessRanges,
                                      IN PVIDEO_ACCESS_RANGE  AccessRanges);
VOID STDCALL VideoPortWritePortUchar(IN PUCHAR  Port, IN UCHAR  Value);
VOID STDCALL VideoPortWritePortUshort(IN PUSHORT  Port, IN USHORT  Value);
VOID STDCALL VideoPortWritePortUlong(IN PULONG Port, IN ULONG Value);
VOID STDCALL VideoPortWritePortBufferUchar(IN PUCHAR  Port, IN PUCHAR  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortWritePortBufferUshort(IN PUSHORT  Port, IN PUSHORT  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortWritePortBufferUlong(IN PULONG  Port, IN PULONG  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortWriteRegisterUchar(IN PUCHAR  Register, IN UCHAR  Value);
VOID STDCALL VideoPortWriteRegisterUshort(IN PUSHORT  Register, IN USHORT  Value);
VOID STDCALL VideoPortWriteRegisterUlong(IN PULONG  Register, IN ULONG  Value);
VOID STDCALL VideoPortWriteRegisterBufferUchar(IN PUCHAR  Register, IN PUCHAR  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortWriteRegisterBufferUshort(IN PUSHORT  Register, IN PUSHORT  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortWriteRegisterBufferUlong(IN PULONG  Register, IN PULONG  Buffer, IN ULONG  Count);
VOID STDCALL VideoPortZeroMemory(OUT PVOID  Destination, IN ULONG  Length);
VOID STDCALL VideoPortZeroDeviceMemory(OUT PVOID  Destination, IN ULONG  Length);


