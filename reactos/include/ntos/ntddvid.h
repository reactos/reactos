#ifndef NTOS_NTDDVID_H
#define NTOS_NTDDVID_H

#include <ddk/ntddk.h>

#define  EMULATOR_READ_ACCESS   0x00000001
#define  EMULATOR_WRITE_ACCESS  0x00000002

typedef enum _EMULATOR_PORT_ACCESS_TYPE 
{
  Uchar,
  Ushort,
  Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;

typedef struct _EMULATOR_ACCESS_ENTRY 
{
  ULONG  BasePort;
  ULONG  NumConsecutivePorts;
  EMULATOR_PORT_ACCESS_TYPE  AccessType;
  UCHAR  AccessMode;
  UCHAR  StringSupport;
  PVOID  Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;


typedef LONG VP_STATUS, *PVP_STATUS;

typedef struct _STATUS_BLOCK
{
  VP_STATUS  Status;
  ULONG  Information;
} STATUS_BLOCK, *PSTATUS_BLOCK;

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

typedef struct _VIDEO_REQUEST_PACKET
{
  ULONG  IoControlCode;
  PSTATUS_BLOCK  StatusBlock;
  PVOID  InputBuffer;
  ULONG  InputBufferLength;
  PVOID  OutputBuffer;
  ULONG  OutputBufferLength;
} VIDEO_REQUEST_PACKET, *PVIDEO_REQUEST_PACKET;

typedef VP_STATUS STDCALL
(*PVIDEO_HW_FIND_ADAPTER)(PVOID  HwDeviceExtension,
			  PVOID  HwContext,
			  PWSTR  ArgumentString,
			  PVIDEO_PORT_CONFIG_INFO  ConfigInfo,
			  PUCHAR  Again);

typedef BOOLEAN STDCALL
(*PVIDEO_HW_INITIALIZE)(PVOID  HwDeviceExtension);

typedef BOOLEAN STDCALL
(*PVIDEO_HW_INTERRUPT)(PVOID  HwDeviceExtension);

typedef BOOLEAN STDCALL
(*PVIDEO_HW_START_IO)(PVOID  HwDeviceExtension,
		      PVIDEO_REQUEST_PACKET  RequestPacket);

typedef BOOLEAN STDCALL
(*PVIDEO_HW_RESET_HW)(PVOID  HwDeviceExtension,
		      ULONG  Columns,
		      ULONG  Rows);

typedef VOID STDCALL
(*PVIDEO_HW_TIMER)(PVOID  HwDeviceExtension);

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

typedef enum _VIDEO_DEVICE_DATA_TYPE
{
  VpMachineData,
  VpCmosData,
  VpBusData,
  VpControllerData,
  VpMonitorData
} VIDEO_DEVICE_DATA_TYPE, *PVIDEO_DEVICE_DATA_TYPE;


typedef VP_STATUS STDCALL
(*PMINIPORT_QUERY_DEVICE_ROUTINE)(PVOID  HwDeviceExtension,
				  PVOID  Context,
				  VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
				  PVOID  Identifier,
				  ULONG  IdentiferLength,
				  PVOID  ConfigurationData,
				  ULONG  ConfigurationDataLength,
				  PVOID  ComponentInformation,
				  ULONG  ComponentInformationLength);

typedef struct _VIDEO_ACCESS_RANGE
{
  PHYSICAL_ADDRESS  RangeStart;
  ULONG  RangeLength;
  UCHAR  RangeInIoSpace;
  UCHAR  RangeVisible;
  UCHAR  RangeShareable;
} VIDEO_ACCESS_RANGE, *PVIDEO_ACCESS_RANGE;

typedef enum _VIDEO_SYNCHRONIZE_PRIORITY
{
  VpLowPriority,
  VpMediumPriority,
  VpHighPriority
} VIDEO_SYNCHRONIZE_PRIORITY, *PVIDEO_SYNCHRONIZE_PRIORITY;


typedef VP_STATUS STDCALL
(*PMINIPORT_GET_REGISTRY_ROUTINE)(PVOID  HwDeviceExtension,
				  PVOID  Context,
				  PWSTR  ValueName,
				  PVOID  ValueData,
				  ULONG  ValueLength);

typedef VOID STDCALL
(*PBANKED_SECTION_ROUTINE)(IN ULONG  ReadBank,
			   IN ULONG  WriteBank,
			   IN PVOID  Context);

typedef BOOLEAN STDCALL
(*PMINIPORT_SYNCHRONIZE_ROUTINE)(PVOID Context);


NTSTATUS STDCALL VideoPortInt10(IN PVOID  HwDeviceExtension,
  IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments);

ULONG STDCALL VideoPortCompareMemory(IN PVOID  Source1, IN PVOID  Source2, IN ULONG  Length);
VOID VideoPortDebugPrint(IN ULONG DebugPrintLevel, IN PCHAR DebugMessage, ...);
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

#if 0
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

// Bit definitions for Attribute Flags
#define VIDEO_MODE_COLOR          0x0001
#define VIDEO_MODE_GRAPHICS       0x0002
#define VIDEO_MODE_PALETTE_DRIVEN 0x0004

#define VIDEO_MEMORY_SPACE_MEMORY    0x00
#define VIDEO_MEMORY_SPACE_IO        0x01
#define VIDEO_MEMORY_SPACE_USER_MODE 0x02
#define VIDEO_MEMORY_SPACE_DENSE     0x04
#define VIDEO_MEMORY_SPACE_P6CACHE   0x08

typedef struct _VIDEO_POINTER_CAPABILITIES
{
  ULONG  Flags;
  ULONG  MaxWidth;
  ULONG  MaxHeight;
  ULONG  HWPtrBitmapStart;
  ULONG  HWPtrBitmapEnd;
} VIDEO_POINTER_CAPABILITIES, *PVIDEO_POINTER_CAPABILITIES;

typedef struct _VIDEO_POINTER_ATTRIBUTES
{
  ULONG  Flags;
  ULONG  Width;
  ULONG  Height;
  ULONG  WidthInBytes;
  ULONG  Enable;
  SHORT  Column;
  SHORT  Row;
  UCHAR  Pixels[1];
} VIDEO_POINTER_ATTRIBUTES, *PVIDEO_POINTER_ATTRIBUTES;

typedef enum _VIDEO_BANK_TYPE
{
  VideoNotBanked = 0,
  VideoBanked1RW,
  VideoBanked1R1W,
  VideoBanked2RW,
  NumVideoBankTypes
} VIDEO_BANK_TYPE, *PVIDEO_BANK_TYPE;

typedef struct _VIDEO_BANK_SELECT
{
  ULONG  Length;
  ULONG  Size;
  ULONG  BankingFlags;
  ULONG  BankingType;
  ULONG  PlanarHCBankingType;
  ULONG  BitmapWidthInBytes;
  ULONG  BitmapSize;
  ULONG  Granularity;
  ULONG  PlanarHCGranularity;
  ULONG  CodeOffset;
  ULONG  PlanarHCBankCodeOffset;
  ULONG  PlanarHCEnableCodeOffset;
  ULONG  PlanarHCDisableCodeOffset;
} VIDEO_BANK_SELECT, *PVIDEO_BANK_SELECT;

typedef struct _VIDEO_CLUTDATA
{
  UCHAR  Red;
  UCHAR  Green;
  UCHAR  Blue;
  UCHAR  Unused;
} VIDEO_CLUTDATA, *PVIDEO_CLUTDATA;

typedef struct _VIDEO_NUM_MODES
{
  ULONG  NumModes;
  ULONG  ModeInformationLength;
} VIDEO_NUM_MODES, *PVIDEO_NUM_MODES;

typedef struct _VIDEO_MODE_INFORMATION
{
  ULONG  Length;
  ULONG  ModeIndex;
  ULONG  VisScreenWidth;
  ULONG  VisScreenHeight;
  ULONG  ScreenStride;
  ULONG  NumberOfPlanes;
  ULONG  BitsPerPlane;
  ULONG  Frequency;
  ULONG  XMillimeter;
  ULONG  YMillimeter;
  ULONG  NumberRedBits;
  ULONG  NumberGreenBits;
  ULONG  NumberBlueBits;
  ULONG  RedMask;
  ULONG  GreenMask;
  ULONG  BlueMask;
  ULONG  AttributeFlags;
  ULONG  VideoMemoryBitmapWidth;
  ULONG  VideoMemoryBitmapHeight;
  ULONG  DriverSpecificAttributeFlags;
} VIDEO_MODE_INFORMATION, *PVIDEO_MODE_INFORMATION;

#define IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES       CTL_CODE(FILE_DEVICE_VIDEO,  0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_AVAIL_MODES           CTL_CODE(FILE_DEVICE_VIDEO,  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_CURRENT_MODE          CTL_CODE(FILE_DEVICE_VIDEO,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_CURRENT_MODE            CTL_CODE(FILE_DEVICE_VIDEO,  3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_RESET_DEVICE                CTL_CODE(FILE_DEVICE_VIDEO,  4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_MAP_VIDEO_MEMORY            CTL_CODE(FILE_DEVICE_VIDEO,  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_UNMAP_VIDEO_MEMORY          CTL_CODE(FILE_DEVICE_VIDEO,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SHARE_VIDEO_MEMORY          CTL_CODE(FILE_DEVICE_VIDEO,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY        CTL_CODE(FILE_DEVICE_VIDEO,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_GET_PUBLIC_ACCESS_RANGES    CTL_CODE(FILE_DEVICE_VIDEO,  9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES   CTL_CODE(FILE_DEVICE_VIDEO,  10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_GET_POWER_MANAGEMENT        CTL_CODE(FILE_DEVICE_VIDEO,  11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_POWER_MANAGEMENT        CTL_CODE(FILE_DEVICE_VIDEO,  12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_COLOR_CAPABILITIES          CTL_CODE(FILE_DEVICE_VIDEO,  13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_COLOR_REGISTERS         CTL_CODE(FILE_DEVICE_VIDEO,  14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_DISABLE_POINTER             CTL_CODE(FILE_DEVICE_VIDEO,  15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_ENABLE_POINTER              CTL_CODE(FILE_DEVICE_VIDEO,  16, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES  CTL_CODE(FILE_DEVICE_VIDEO,  17, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_POINTER_ATTR          CTL_CODE(FILE_DEVICE_VIDEO,  18, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_POINTER_ATTR            CTL_CODE(FILE_DEVICE_VIDEO,  19, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_POINTER_POSITION      CTL_CODE(FILE_DEVICE_VIDEO,  20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_POINTER_POSITION        CTL_CODE(FILE_DEVICE_VIDEO,  21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SAVE_HARDWARE_STATE         CTL_CODE(FILE_DEVICE_VIDEO,  22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_RESTORE_HARDWARE_STATE      CTL_CODE(FILE_DEVICE_VIDEO,  23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_DISABLE_CURSOR              CTL_CODE(FILE_DEVICE_VIDEO,  24, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_ENABLE_CURSOR               CTL_CODE(FILE_DEVICE_VIDEO,  25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_CURSOR_ATTR           CTL_CODE(FILE_DEVICE_VIDEO,  26, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_CURSOR_ATTR             CTL_CODE(FILE_DEVICE_VIDEO,  27, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_QUERY_CURSOR_POSITION       CTL_CODE(FILE_DEVICE_VIDEO,  28, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_CURSOR_POSITION         CTL_CODE(FILE_DEVICE_VIDEO,  29, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_GET_BANK_SELECT_CODE        CTL_CODE(FILE_DEVICE_VIDEO,  30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_SET_PALETTE_REGISTERS       CTL_CODE(FILE_DEVICE_VIDEO,  31, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_LOAD_AND_SET_FONT           CTL_CODE(FILE_DEVICE_VIDEO,  32, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct
{
  USHORT  NumEntries;
  USHORT  FirstEntry;
  union
    {
      VIDEO_CLUTDATA  RgbArray;
      ULONG  RgbLong;
    } LookupTable[1];
} VIDEO_CLUT, *PVIDEO_CLUT;

typedef struct _VIDEO_MEMORY
{
  PVOID RequestedVirtualAddress;
} VIDEO_MEMORY, *PVIDEO_MEMORY;

typedef struct _VIDEO_MEMORY_INFORMATION
{
  PVOID  VideoRamBase;
  ULONG  VideoRamLength;
  PVOID  FrameBufferBase;
  ULONG  FrameBufferLength;
} VIDEO_MEMORY_INFORMATION, *PVIDEO_MEMORY_INFORMATION;

typedef struct _VIDEO_MODE
{
  ULONG  RequestedMode;
} VIDEO_MODE, *PVIDEO_MODE;

typedef struct _VIDEO_SHARE_MEMORY
{
  HANDLE  ProcessHandle;
  ULONG  ViewOffset;
  ULONG  ViewSize;
  PVOID  RequestedVirtualAddress;
} VIDEO_SHARE_MEMORY, *PVIDEO_SHARE_MEMORY;

#endif

#endif /* NTOS_NTDDVID_H */
