/*
 * video.h
 *
 * Video port and miniport driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __VIDEO_H
#define __VIDEO_H


#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __WINDDI_H
#error winddi.h cannot be included with video.h
#else

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#if defined(_VIDEOPORT_)
  #define VPAPI DECLSPEC_EXPORT
#else
  #define VPAPI DECLSPEC_IMPORT
#endif

#include "videoagp.h"
#include "ntddvdeo.h"


typedef LONG VP_STATUS;
typedef VP_STATUS *PVP_STATUS;
typedef struct __DMA_PARAMETERS * PDMA;
typedef struct _VIDEO_PORT_EVENT *PEVENT;
typedef struct _VIDEO_PORT_SPIN_LOCK *PSPIN_LOCK;
typedef struct __VP_DMA_ADAPTER *PVP_DMA_ADAPTER;

#define DISPLAY_ADAPTER_HW_ID             0xFFFFFFFF

#define EVENT_TYPE_MASK                   1
#define SYNCHRONIZATION_EVENT             0
#define NOTIFICATION_EVENT                1

#define INITIAL_EVENT_STATE_MASK          2
#define INITIAL_EVENT_NOT_SIGNALED        0
#define INITIAL_EVENT_SIGNALED            2

typedef enum VIDEO_DEBUG_LEVEL {
  Error = 0,
  Warn,
  Trace,
  Info
} VIDEO_DEBUG_LEVEL, *PVIDEO_DEBUG_LEVEL;

typedef enum {
  VideoPortUnlockAfterDma = 1,
  VideoPortKeepPagesLocked,
  VideoPortDmaInitOnly
} DMA_FLAGS;

typedef enum _HW_DMA_RETURN {
  DmaAsyncReturn,
  DmaSyncReturn
} HW_DMA_RETURN, *PHW_DMA_RETURN;

typedef HW_DMA_RETURN
(*PVIDEO_HW_START_DMA)(
    PVOID  HwDeviceExtension,
    PDMA  pDma);


#ifdef DBG

#define PAGED_CODE() \
  if (VideoPortGetCurrentIrql() > 1 /* APC_LEVEL */) \
  { \
    VideoPortDebugPrint(Error, "Video: Pageable code called at IRQL %d\n", VideoPortGetCurrentIrql() ); \
    ASSERT(FALSE); \
  }

#else

#define PAGED_CODE()

#endif

typedef struct _VIDEO_HARDWARE_CONFIGURATION_DATA {
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  USHORT Version;
  USHORT Revision;
  USHORT Irql;
  USHORT Vector;
  ULONG ControlBase;
  ULONG ControlSize;
  ULONG CursorBase;
  ULONG CursorSize;
  ULONG FrameBase;
  ULONG FrameSize;
} VIDEO_HARDWARE_CONFIGURATION_DATA, *PVIDEO_HARDWARE_CONFIGURATION_DATA;

#define SIZE_OF_NT4_VIDEO_PORT_CONFIG_INFO       0x42
#define SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA 0x28
#define SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA 0x50

typedef enum _VIDEO_DEVICE_DATA_TYPE {
  VpMachineData = 0,
  VpCmosData,
  VpBusData,
  VpControllerData,
  VpMonitorData
} VIDEO_DEVICE_DATA_TYPE, *PVIDEO_DEVICE_DATA_TYPE;



/* Video miniport driver functions */

typedef struct _VP_SCATTER_GATHER_ELEMENT {
  PHYSICAL_ADDRESS  Address;
  ULONG  Length;
  ULONG_PTR  Reserved;
} VP_SCATTER_GATHER_ELEMENT, *PVP_SCATTER_GATHER_ELEMENT;

typedef struct _VP_SCATTER_GATHER_LIST {
  ULONG  NumberOfElements;
  ULONG_PTR  Reserved;
  VP_SCATTER_GATHER_ELEMENT  Elements[0];
} VP_SCATTER_GATHER_LIST, *PVP_SCATTER_GATHER_LIST;

typedef VOID DDKAPI
(*PEXECUTE_DMA)(
	IN PVOID  HwDeviceExtension,
	IN PVP_DMA_ADAPTER  VpDmaAdapter,
	IN PVP_SCATTER_GATHER_LIST  SGList,
	IN PVOID  Context);

typedef PVOID DDKAPI
(*PVIDEO_PORT_GET_PROC_ADDRESS)(
  IN PVOID  HwDeviceExtension,
  IN PUCHAR  FunctionName);

typedef struct _VIDEO_PORT_CONFIG_INFO {
  ULONG  Length;
  ULONG  SystemIoBusNumber;
  INTERFACE_TYPE  AdapterInterfaceType;
  ULONG  BusInterruptLevel;
  ULONG  BusInterruptVector;
  KINTERRUPT_MODE  InterruptMode;
  ULONG  NumEmulatorAccessEntries;
  PEMULATOR_ACCESS_ENTRY  EmulatorAccessEntries;
  ULONG_PTR  EmulatorAccessEntriesContext;
  PHYSICAL_ADDRESS  VdmPhysicalVideoMemoryAddress;
  ULONG  VdmPhysicalVideoMemoryLength;
  ULONG  HardwareStateSize;
  ULONG  DmaChannel;
  ULONG  DmaPort;
  UCHAR  DmaShareable;
  UCHAR  InterruptShareable;
  BOOLEAN  Master;
  DMA_WIDTH  DmaWidth;
  DMA_SPEED  DmaSpeed;
  BOOLEAN  bMapBuffers;
  BOOLEAN  NeedPhysicalAddresses;
  BOOLEAN  DemandMode;
  ULONG  MaximumTransferLength;
  ULONG  NumberOfPhysicalBreaks;
  BOOLEAN  ScatterGather;
  ULONG  MaximumScatterGatherChunkSize;
  PVIDEO_PORT_GET_PROC_ADDRESS VideoPortGetProcAddress;
  PWSTR  DriverRegistryPath;
  ULONGLONG  SystemMemorySize;
} VIDEO_PORT_CONFIG_INFO, *PVIDEO_PORT_CONFIG_INFO;

typedef VP_STATUS DDKAPI
(*PVIDEO_HW_FIND_ADAPTER)(
	IN PVOID  HwDeviceExtension,
	IN PVOID  HwContext,
	IN PWSTR  ArgumentString,
	IN OUT  PVIDEO_PORT_CONFIG_INFO  ConfigInfo,
	OUT PUCHAR  Again);

typedef VP_STATUS DDKAPI
(*PVIDEO_HW_POWER_GET)(
  IN PVOID  HwDeviceExtension,
  IN ULONG  HwId,
  IN OUT  PVIDEO_POWER_MANAGEMENT  VideoPowerControl);

/* PVIDEO_HW_GET_CHILD_DESCRIPTOR return values */
#define VIDEO_ENUM_MORE_DEVICES           ERROR_CONTINUE
#define VIDEO_ENUM_NO_MORE_DEVICES        ERROR_NO_MORE_DEVICES
#define VIDEO_ENUM_INVALID_DEVICE         ERROR_INVALID_NAME

/* PVIDEO_HW_GET_CHILD_DESCRIPTOR.ChildEnumInfo constants */
typedef struct _VIDEO_CHILD_ENUM_INFO {
  ULONG  Size;
  ULONG  ChildDescriptorSize;
  ULONG  ChildIndex;
  ULONG  ACPIHwId;
  PVOID  ChildHwDeviceExtension;
} VIDEO_CHILD_ENUM_INFO, *PVIDEO_CHILD_ENUM_INFO;

/* PVIDEO_HW_GET_CHILD_DESCRIPTOR.VideoChildType constants */
typedef enum _VIDEO_CHILD_TYPE {
  Monitor = 1,
  NonPrimaryChip,
  VideoChip,
  Other
} VIDEO_CHILD_TYPE, *PVIDEO_CHILD_TYPE;

typedef VP_STATUS DDKAPI
(*PVIDEO_HW_GET_CHILD_DESCRIPTOR)(
  IN PVOID  HwDeviceExtension,
  IN PVIDEO_CHILD_ENUM_INFO  ChildEnumInfo,
  OUT  PVIDEO_CHILD_TYPE  VideoChildType,
  OUT  PUCHAR  pChildDescriptor,
  OUT  PULONG  UId,
  OUT  PULONG  pUnused);

typedef BOOLEAN DDKAPI
(*PVIDEO_HW_INITIALIZE)(
  IN PVOID  HwDeviceExtension);

typedef BOOLEAN DDKAPI
(*PVIDEO_HW_INTERRUPT)(
  IN PVOID  HwDeviceExtension);

/* VIDEO_ACCESS_RANGE.RangePassive */
#define VIDEO_RANGE_PASSIVE_DECODE        1
#define VIDEO_RANGE_10_BIT_DECODE         2

#ifndef VIDEO_ACCESS_RANGE_DEFINED /* also in miniport.h */
#define VIDEO_ACCESS_RANGE_DEFINED
typedef struct _VIDEO_ACCESS_RANGE {
  PHYSICAL_ADDRESS  RangeStart;
  ULONG  RangeLength;
  UCHAR  RangeInIoSpace;
  UCHAR  RangeVisible;
  UCHAR  RangeShareable;
  UCHAR  RangePassive;
} VIDEO_ACCESS_RANGE, *PVIDEO_ACCESS_RANGE;
#endif

typedef VOID DDKAPI
(*PVIDEO_HW_LEGACYRESOURCES)(
  IN ULONG  VendorId,
  IN ULONG  DeviceId,
  IN OUT  PVIDEO_ACCESS_RANGE  *LegacyResourceList,
  IN OUT  PULONG  LegacyResourceCount);

typedef VP_STATUS DDKAPI
(*PMINIPORT_QUERY_DEVICE_ROUTINE)(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Context,
  IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
  IN PVOID  Identifier,
  IN ULONG  IdentifierLength,
  IN PVOID  ConfigurationData,
  IN ULONG  ConfigurationDataLength,
  IN OUT  PVOID  ComponentInformation,
  IN ULONG  ComponentInformationLength);

typedef struct _QUERY_INTERFACE {
  CONST GUID  *InterfaceType;
  USHORT  Size;
  USHORT  Version;
  PINTERFACE  Interface;
  PVOID  InterfaceSpecificData;
} QUERY_INTERFACE, *PQUERY_INTERFACE;

typedef VP_STATUS DDKAPI
(*PVIDEO_HW_QUERY_INTERFACE)(
  IN PVOID  HwDeviceExtension,
  IN OUT  PQUERY_INTERFACE  QueryInterface);

typedef VP_STATUS DDKAPI
(*PMINIPORT_GET_REGISTRY_ROUTINE)(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Context,
  IN OUT  PWSTR  ValueName,
  IN OUT  PVOID  ValueData,
  IN ULONG  ValueLength);

typedef BOOLEAN DDKAPI
(*PVIDEO_HW_RESET_HW)(
  IN PVOID  HwDeviceExtension,
  IN ULONG  Columns,
  IN ULONG  Rows);

typedef VP_STATUS DDKAPI
(*PVIDEO_HW_POWER_SET)(
  IN PVOID  HwDeviceExtension,
  IN ULONG  HwId,
  IN PVIDEO_POWER_MANAGEMENT  VideoPowerControl);

typedef struct _STATUS_BLOCK {
   _ANONYMOUS_UNION union {
    VP_STATUS  Status;
    PVOID  Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR Information;
} STATUS_BLOCK, *PSTATUS_BLOCK;

typedef struct _VIDEO_REQUEST_PACKET {
  ULONG  IoControlCode;
  PSTATUS_BLOCK  StatusBlock;
  PVOID  InputBuffer;
  ULONG  InputBufferLength;
  PVOID  OutputBuffer;
  ULONG  OutputBufferLength;
} VIDEO_REQUEST_PACKET, *PVIDEO_REQUEST_PACKET;

typedef BOOLEAN DDKAPI
(*PVIDEO_HW_START_IO)(
  IN PVOID  HwDeviceExtension,
  IN PVIDEO_REQUEST_PACKET  RequestPacket);

typedef BOOLEAN DDKAPI
(*PMINIPORT_SYNCHRONIZE_ROUTINE)(
  IN PVOID  Context);

typedef VOID DDKAPI
(*PVIDEO_HW_TIMER)(
  IN PVOID  HwDeviceExtension);

typedef VOID DDKAPI
(*PMINIPORT_DPC_ROUTINE)(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Context);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_UCHAR)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PUCHAR  Data);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_UCHAR_STRING)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PUCHAR  Data,
  IN ULONG  DataLength);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_ULONG)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PULONG  Data);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_ULONG_STRING)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PULONG  Data,
  IN ULONG  DataLength);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_USHORT)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PUSHORT  Data);

typedef VP_STATUS DDKAPI
(*PDRIVER_IO_PORT_USHORT_STRING)(
  IN ULONG_PTR  Context,
  IN ULONG  Port,
  IN UCHAR  AccessMode,
  IN PUSHORT  Data,
  IN ULONG  DataLength);



typedef struct _INT10_BIOS_ARGUMENTS {
  ULONG  Eax;
  ULONG  Ebx;
  ULONG  Ecx;
  ULONG  Edx;
  ULONG  Esi;
  ULONG  Edi;
  ULONG  Ebp;
  USHORT  SegDs;
  USHORT  SegEs;
} INT10_BIOS_ARGUMENTS, *PINT10_BIOS_ARGUMENTS;

typedef struct _VIDEO_CHILD_STATE {
  ULONG  Id;
  ULONG  State;
} VIDEO_CHILD_STATE, *PVIDEO_CHILD_STATE;

typedef struct _VIDEO_CHILD_STATE_CONFIGURATION {
  ULONG  Count;
  VIDEO_CHILD_STATE  ChildStateArray[ANYSIZE_ARRAY];
} VIDEO_CHILD_STATE_CONFIGURATION, *PVIDEO_CHILD_STATE_CONFIGURATION;

typedef struct _VIDEO_HW_INITIALIZATION_DATA {
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
  PVIDEO_HW_START_DMA  HwStartDma;
  PVIDEO_HW_POWER_SET  HwSetPowerState;
  PVIDEO_HW_POWER_GET  HwGetPowerState;
  PVIDEO_HW_GET_CHILD_DESCRIPTOR  HwGetVideoChildDescriptor;
  PVIDEO_HW_QUERY_INTERFACE  HwQueryInterface;
  ULONG  HwChildDeviceExtensionSize;
  PVIDEO_ACCESS_RANGE  HwLegacyResourceList;
  ULONG  HwLegacyResourceCount;
  PVIDEO_HW_LEGACYRESOURCES  HwGetLegacyResources;
  BOOLEAN  AllowEarlyEnumeration;
  ULONG  Reserved;
} VIDEO_HW_INITIALIZATION_DATA, *PVIDEO_HW_INITIALIZATION_DATA;

/* VIDEO_PORT_AGP_INTERFACE.Version contants */
#define VIDEO_PORT_AGP_INTERFACE_VERSION_1 1

typedef struct _VIDEO_PORT_AGP_INTERFACE {
  SHORT  Size;
  SHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PAGP_RESERVE_PHYSICAL  AgpReservePhysical;
  PAGP_RELEASE_PHYSICAL  AgpReleasePhysical;
  PAGP_COMMIT_PHYSICAL  AgpCommitPhysical;
  PAGP_FREE_PHYSICAL  AgpFreePhysical;
  PAGP_RESERVE_VIRTUAL  AgpReserveVirtual;
  PAGP_RELEASE_VIRTUAL  AgpReleaseVirtual;
  PAGP_COMMIT_VIRTUAL  AgpCommitVirtual;
  PAGP_FREE_VIRTUAL  AgpFreeVirtual;
  ULONGLONG  AgpAllocationLimit;
} VIDEO_PORT_AGP_INTERFACE, *PVIDEO_PORT_AGP_INTERFACE;

/* VIDEO_PORT_AGP_INTERFACE_2.Version constants */
#define VIDEO_PORT_AGP_INTERFACE_VERSION_2 2

typedef struct _VIDEO_PORT_AGP_INTERFACE_2 {
  IN USHORT  Size;
  IN USHORT  Version;
  OUT PVOID  Context;
  OUT PINTERFACE_REFERENCE  InterfaceReference;
  OUT PINTERFACE_DEREFERENCE  InterfaceDereference;
  OUT PAGP_RESERVE_PHYSICAL  AgpReservePhysical;
  OUT PAGP_RELEASE_PHYSICAL  AgpReleasePhysical;
  OUT PAGP_COMMIT_PHYSICAL  AgpCommitPhysical;
  OUT PAGP_FREE_PHYSICAL  AgpFreePhysical;
  OUT PAGP_RESERVE_VIRTUAL  AgpReserveVirtual;
  OUT PAGP_RELEASE_VIRTUAL  AgpReleaseVirtual;
  OUT PAGP_COMMIT_VIRTUAL  AgpCommitVirtual;
  OUT PAGP_FREE_VIRTUAL  AgpFreeVirtual;
  OUT ULONGLONG  AgpAllocationLimit;
  OUT PAGP_SET_RATE  AgpSetRate;
} VIDEO_PORT_AGP_INTERFACE_2, *PVIDEO_PORT_AGP_INTERFACE_2;

#define VIDEO_PORT_I2C_INTERFACE_VERSION_1  1

typedef VOID DDKAPI
(*PVIDEO_WRITE_CLOCK_LINE)(
  PVOID HwDeviceExtension,
  UCHAR Data);

typedef VOID DDKAPI
(*PVIDEO_WRITE_DATA_LINE)(
  PVOID HwDeviceExtension,
  UCHAR Data);

typedef BOOLEAN DDKAPI
(*PVIDEO_READ_CLOCK_LINE)(
  PVOID HwDeviceExtension);

typedef BOOLEAN DDKAPI
(*PVIDEO_READ_DATA_LINE)(
  PVOID HwDeviceExtension);

typedef struct _I2C_CALLBACKS
{
  IN PVIDEO_WRITE_CLOCK_LINE  WriteClockLine;
  IN PVIDEO_WRITE_DATA_LINE  WriteDataLine;
  IN PVIDEO_READ_CLOCK_LINE  ReadClockLine;
  IN PVIDEO_READ_DATA_LINE  ReadDataLine;
} I2C_CALLBACKS, *PI2C_CALLBACKS;

typedef BOOLEAN DDKAPI
(*PI2C_START)(
  IN PVOID  HwDeviceExtension,
  IN PI2C_CALLBACKS  I2CCallbacks);

typedef BOOLEAN DDKAPI
(*PI2C_STOP)(
  IN PVOID  HwDeviceExtension,
  IN PI2C_CALLBACKS  I2CCallbacks);

typedef BOOLEAN DDKAPI
(*PI2C_WRITE)(
  IN PVOID  HwDeviceExtension,
  IN PI2C_CALLBACKS  I2CCallbacks,
  IN PUCHAR  Buffer,
  IN ULONG  Length);

typedef BOOLEAN DDKAPI
(*PI2C_READ)(
  IN PVOID  HwDeviceExtension,
  IN PI2C_CALLBACKS  I2CCallbacks,
  OUT PUCHAR  Buffer,
  IN ULONG  Length);

typedef struct _VIDEO_PORT_I2C_INTERFACE {
  USHORT  Size;
  USHORT  Version;
  PVOID  Context;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;
  PI2C_START  I2CStart;
  PI2C_STOP  I2CStop;
  PI2C_WRITE  I2CWrite;
  PI2C_READ  I2CRead;
} VIDEO_PORT_I2C_INTERFACE, *PVIDEO_PORT_I2C_INTERFACE;

/* VIDEO_PORT_INT10_INTERFACE.Version constants */
#define VIDEO_PORT_INT10_INTERFACE_VERSION_1 1

typedef VP_STATUS DDKAPI
(*PINT10_ALLOCATE_BUFFER)(
  IN PVOID  Context,
  OUT PUSHORT  Seg,
  OUT PUSHORT  Off,
  IN OUT PULONG  Length);

typedef VP_STATUS DDKAPI
(*PINT10_CALL_BIOS)(
  IN PVOID  Context,
  IN OUT PINT10_BIOS_ARGUMENTS  BiosArguments);

typedef VP_STATUS DDKAPI
(*PINT10_FREE_BUFFER)(
  IN PVOID  Context,
  IN USHORT  Seg,
  IN USHORT  Off);

typedef VP_STATUS DDKAPI
(*PINT10_READ_MEMORY)(
  IN PVOID  Context,
  IN USHORT  Seg,
  IN USHORT  Off,
  OUT PVOID  Buffer,
  IN ULONG  Length);

typedef VP_STATUS DDKAPI
(*PINT10_WRITE_MEMORY)(
  IN PVOID  Context,
  IN USHORT  Seg,
  IN USHORT  Off,
  IN PVOID  Buffer,
  IN ULONG  Length);

typedef struct _VIDEO_PORT_INT10_INTERFACE {
  IN USHORT  Size;
  IN USHORT  Version;
  OUT PVOID  Context;
  OUT PINTERFACE_REFERENCE  InterfaceReference;
  OUT PINTERFACE_DEREFERENCE  InterfaceDereference;
  OUT PINT10_ALLOCATE_BUFFER  Int10AllocateBuffer;
  OUT PINT10_FREE_BUFFER  Int10FreeBuffer;
  OUT PINT10_READ_MEMORY  Int10ReadMemory;
  OUT PINT10_WRITE_MEMORY  Int10WriteMemory;
  OUT PINT10_CALL_BIOS  Int10CallBios;
} VIDEO_PORT_INT10_INTERFACE, *PVIDEO_PORT_INT10_INTERFACE;

/* Flags for VideoPortGetDeviceBase and VideoPortMapMemory */
#define VIDEO_MEMORY_SPACE_MEMORY         0x00
#define VIDEO_MEMORY_SPACE_IO             0x01
#define VIDEO_MEMORY_SPACE_USER_MODE      0x02
#define VIDEO_MEMORY_SPACE_DENSE          0x04
#define VIDEO_MEMORY_SPACE_P6CACHE        0x08

typedef struct _VIDEO_X86_BIOS_ARGUMENTS {
  ULONG  Eax;
  ULONG  Ebx;
  ULONG  Ecx;
  ULONG  Edx;
  ULONG  Esi;
  ULONG  Edi;
  ULONG  Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;

typedef struct _VP_DEVICE_DESCRIPTION {
  BOOLEAN  ScatterGather;
  BOOLEAN  Dma32BitAddresses;
  BOOLEAN  Dma64BitAddresses;
  ULONG  MaximumLength;
} VP_DEVICE_DESCRIPTION, *PVP_DEVICE_DESCRIPTION;

typedef struct _VPOSVERSIONINFO {
  IN ULONG  Size;
  OUT ULONG  MajorVersion;
  OUT ULONG  MinorVersion;
  OUT ULONG  BuildNumber;
  OUT USHORT  ServicePackMajor;
  OUT USHORT  ServicePackMinor;
} VPOSVERSIONINFO, *PVPOSVERSIONINFO;



/* Video port functions for miniports */

VPAPI
VOID
DDKAPI
VideoDebugPrint(
  IN ULONG  DebugPrintLevel,
  IN PCHAR  DebugMessage,
  IN ...);

VPAPI
VOID
DDKAPI
VideoPortAcquireDeviceLock(
  IN PVOID  HwDeviceExtension);

VPAPI
VOID
DDKAPI
VideoPortAcquireSpinLock(
  IN PVOID  HwDeviceExtension,
  IN PSPIN_LOCK  SpinLock,
  OUT PUCHAR  OldIrql);

VPAPI
VOID
DDKAPI
VideoPortAcquireSpinLockAtDpcLevel(
  IN PVOID  HwDeviceExtension,
  IN PSPIN_LOCK  SpinLock);

VPAPI
VP_STATUS
DDKAPI
VideoPortAllocateBuffer(
  IN PVOID  HwDeviceExtension,
  IN ULONG  Size,
  OUT PVOID  *Buffer);

VPAPI
PVOID
DDKAPI
VideoPortAllocateCommonBuffer(
  IN PVOID  HwDeviceExtension,
  IN PVP_DMA_ADAPTER  VpDmaAdapter,
  IN ULONG  DesiredLength,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled,
  PVOID  Reserved);

VPAPI
PVOID
DDKAPI
VideoPortAllocateContiguousMemory(
  IN PVOID  HwDeviceExtension,
  IN ULONG  NumberOfBytes,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress);

/* VideoPortAllocatePool.PoolType constants */
typedef enum _VP_POOL_TYPE {
  VpNonPagedPool = 0,
  VpPagedPool,
  VpNonPagedPoolCacheAligned = 4,
  VpPagedPoolCacheAligned
} VP_POOL_TYPE, *PVP_POOL_TYPE;

VPAPI
PVOID
DDKAPI
VideoPortAllocatePool(
  IN PVOID  HwDeviceExtension,
  IN VP_POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

VPAPI
PDMA
DDKAPI
VideoPortAssociateEventsWithDmaHandle(
  IN PVOID  HwDeviceExtension,
  IN OUT PVIDEO_REQUEST_PACKET  pVrp,
  IN PVOID  MappedUserEvent,
  IN PVOID  DisplayDriverEvent);

/* VideoPortCheckForDeviceExistence.Flags constants */
#define CDE_USE_SUBSYSTEM_IDS             0x00000001
#define CDE_USE_REVISION                  0x00000002

VPAPI
BOOLEAN
DDKAPI
VideoPortCheckForDeviceExistence(
  IN PVOID  HwDeviceExtension,
  IN USHORT  VendorId,
  IN USHORT  DeviceId,
  IN UCHAR  RevisionId,
  IN USHORT  SubVendorId,
  IN USHORT  SubSystemId,
  IN ULONG  Flags);

VPAPI
VOID
DDKAPI
VideoPortClearEvent(
  IN PVOID  HwDeviceExtension,
  IN PEVENT  pEvent);

VPAPI
ULONG
DDKAPI
VideoPortCompareMemory(
  IN PVOID  Source1,
  IN PVOID  Source2,
  IN ULONG  Length);

VPAPI
VP_STATUS
DDKAPI
VideoPortCompleteDma(
  IN PVOID  HwDeviceExtension,
  IN PVP_DMA_ADAPTER  VpDmaAdapter,
  IN PVP_SCATTER_GATHER_LIST  VpScatterGather,
  IN BOOLEAN  WriteToDevice);

VPAPI
VP_STATUS
DDKAPI
VideoPortCreateEvent(
  IN PVOID  HwDeviceExtension,
  IN ULONG  EventFlag,
  IN PVOID  Unused,
  OUT PEVENT  *ppEvent);

VPAPI
VP_STATUS
DDKAPI
VideoPortCreateSecondaryDisplay(
  IN PVOID  HwDeviceExtension,
  IN OUT PVOID  *SecondaryDeviceExtension,
  IN ULONG  ulFlag);

VPAPI
VP_STATUS
DDKAPI
VideoPortCreateSpinLock(
  IN PVOID  HwDeviceExtension,
  OUT PSPIN_LOCK  *SpinLock);

typedef struct _DDC_CONTROL {
  IN ULONG  Size;
  IN I2C_CALLBACKS  I2CCallbacks;
  IN UCHAR  EdidSegment;
} DDC_CONTROL, *PDDC_CONTROL;

VPAPI
BOOLEAN
DDKAPI
VideoPortDDCMonitorHelper(
  IN PVOID  HwDeviceExtension,
  IN PVOID  DDCControl,
  IN OUT PUCHAR  EdidBuffer,
  IN ULONG  EdidBufferSize);

VPAPI
VOID
DDKCDECLAPI
VideoPortDebugPrint(
  IN VIDEO_DEBUG_LEVEL  DebugPrintLevel,
  IN PCHAR  DebugMessage,
  IN ...);

VPAPI
VP_STATUS
DDKAPI
VideoPortDeleteEvent(
  IN PVOID  HwDeviceExtension,
  IN PEVENT  pEvent);

VPAPI
VP_STATUS
DDKAPI
VideoPortDeleteSpinLock(
  IN PVOID  HwDeviceExtension,
  IN PSPIN_LOCK  SpinLock);

VPAPI
VP_STATUS
DDKAPI
VideoPortDisableInterrupt(
  IN PVOID  HwDeviceExtension);

VPAPI
PDMA
DDKAPI
VideoPortDoDma(
  IN PVOID  HwDeviceExtension,
  IN PDMA  pDma,
  IN DMA_FLAGS  DmaFlags);

VPAPI
VP_STATUS
DDKAPI
VideoPortEnableInterrupt(
  IN PVOID  HwDeviceExtension);

VPAPI
VP_STATUS
DDKAPI
VideoPortEnumerateChildren(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Reserved);

VPAPI
VOID
DDKAPI
VideoPortFreeCommonBuffer(
  IN PVOID  HwDeviceExtension,
  IN ULONG  Length,
  IN PVOID  VirtualAddress,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

VPAPI
VOID
DDKAPI
VideoPortFreeDeviceBase(
  IN PVOID  HwDeviceExtension,
  IN PVOID  MappedAddress);

VPAPI
VOID
DDKAPI
VideoPortFreePool(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Ptr);

VPAPI
VP_STATUS
DDKAPI
VideoPortGetAccessRanges(
  IN PVOID  HwDeviceExtension,
  IN ULONG  NumRequestedResources,
  IN PIO_RESOURCE_DESCRIPTOR  RequestedResources  OPTIONAL,
  IN ULONG  NumAccessRanges,
  OUT PVIDEO_ACCESS_RANGE  AccessRanges,
  IN PVOID  VendorId,
  IN PVOID  DeviceId,
  OUT PULONG  Slot);

VPAPI
PVOID
DDKAPI
VideoPortGetAssociatedDeviceExtension(
  IN PVOID  DeviceObject);

VPAPI
ULONG
DDKAPI
VideoPortGetAssociatedDeviceID(
  IN PVOID DeviceObject);

VPAPI
ULONG
DDKAPI
VideoPortGetBusData(
  IN PVOID  HwDeviceExtension,
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  SlotNumber,
  IN OUT PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

VPAPI
ULONG
DDKAPI
VideoPortGetBytesUsed(
  IN PVOID  HwDeviceExtension,
  IN PDMA  pDma);

VPAPI
PVOID
DDKAPI
VideoPortGetCommonBuffer(
  IN PVOID  HwDeviceExtension,
  IN ULONG  DesiredLength,
  IN ULONG  Alignment,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  OUT PULONG  pActualLength,
  IN BOOLEAN  CacheEnabled);

VPAPI
UCHAR
DDKAPI
VideoPortGetCurrentIrql(
  VOID);

VPAPI
PVOID
DDKAPI
VideoPortGetDeviceBase(
  IN PVOID  HwDeviceExtension,
  IN PHYSICAL_ADDRESS  IoAddress,
  IN ULONG  NumberOfUchars,
  IN UCHAR  InIoSpace);

VPAPI
VP_STATUS
DDKAPI
VideoPortGetDeviceData(
  IN PVOID  HwDeviceExtension,
  IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
  IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
  IN PVOID  Context);

VPAPI
PVP_DMA_ADAPTER
DDKAPI
VideoPortGetDmaAdapter(
  IN PVOID  HwDeviceExtension,
  IN PVP_DEVICE_DESCRIPTION  VpDeviceDescription);

VPAPI
PVOID
DDKAPI
VideoPortGetDmaContext(
  IN PVOID  HwDeviceExtension,
  IN PDMA  pDma);

VPAPI
PVOID
DDKAPI
VideoPortGetMdl(
  IN PVOID  HwDeviceExtension,
  IN PDMA  pDma);

VPAPI
VP_STATUS
DDKAPI
VideoPortGetRegistryParameters(
  IN PVOID  HwDeviceExtension,
  IN PWSTR  ParameterName,
  IN UCHAR  IsParameterFileName,
  IN PMINIPORT_GET_REGISTRY_ROUTINE  CallbackRoutine,
  IN PVOID  Context);

VPAPI
PVOID
DDKAPI
VideoPortGetRomImage(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Unused1,
  IN ULONG  Unused2,
  IN ULONG  Length);

VPAPI
VP_STATUS
DDKAPI
VideoPortGetVersion(
  IN PVOID  HwDeviceExtension,
  IN OUT PVPOSVERSIONINFO  pVpOsVersionInfo);

VPAPI
VP_STATUS
DDKAPI
VideoPortGetVgaStatus(
  IN PVOID  HwDeviceExtension,
  OUT PULONG  VgaStatus);

VPAPI
ULONG
DDKAPI
VideoPortInitialize(
  IN PVOID  Argument1,
  IN PVOID  Argument2,
  IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
  IN PVOID  HwContext);

VPAPI
VP_STATUS
DDKAPI
VideoPortInt10(
  IN PVOID  HwDeviceExtension,
  IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments);

VPAPI
LONG
DDKFASTAPI
VideoPortInterlockedDecrement(
  IN PLONG  Addend);

VPAPI
LONG
DDKFASTAPI
VideoPortInterlockedExchange(
  IN OUT PLONG  Target,
  IN LONG  Value);

VPAPI
LONG
DDKFASTAPI
VideoPortInterlockedIncrement(
  IN PLONG  Addend);

typedef enum _VP_LOCK_OPERATION {
  VpReadAccess = 0,
  VpWriteAccess,
  VpModifyAccess
} VP_LOCK_OPERATION;

VPAPI
PVOID
DDKAPI
VideoPortLockBuffer(
  IN PVOID  HwDeviceExtension,
  IN PVOID  BaseAddress,
  IN ULONG  Length,
  IN VP_LOCK_OPERATION  Operation);

VPAPI
BOOLEAN
DDKAPI
VideoPortLockPages(
  IN PVOID  HwDeviceExtension,
  IN OUT PVIDEO_REQUEST_PACKET  pVrp,
  IN OUT PEVENT  pUEvent,
  IN PEVENT  pDisplayEvent,
  IN DMA_FLAGS  DmaFlags);

VPAPI
VOID
DDKAPI
VideoPortLogError(
  IN PVOID  HwDeviceExtension,
  IN PVIDEO_REQUEST_PACKET  Vrp  OPTIONAL,
  IN VP_STATUS  ErrorCode,
  IN ULONG  UniqueId);

VPAPI
VP_STATUS
DDKAPI
VideoPortMapBankedMemory(
  IN PVOID  HwDeviceExtension,
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN OUT PULONG  Length,
  PULONG  InIoSpace,
  PVOID  *VirtualAddress,
  ULONG  BankLength,
  UCHAR  ReadWriteBank,
  PBANKED_SECTION_ROUTINE  BankRoutine,
  PVOID  Context);

VPAPI
PDMA
DDKAPI
VideoPortMapDmaMemory(
  IN PVOID  HwDeviceExtension,
  IN PVIDEO_REQUEST_PACKET  pVrp,
  IN PHYSICAL_ADDRESS  BoardAddress,
  IN PULONG  Length,
  IN PULONG  InIoSpace,
  IN PVOID  MappedUserEvent,
  IN PVOID  DisplayDriverEvent,
  IN OUT PVOID  *VirtualAddress);

VPAPI
VP_STATUS
DDKAPI
VideoPortMapMemory(
  IN PVOID  HwDeviceExtension,
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN OUT PULONG  Length,
  IN PULONG  InIoSpace,
  IN OUT PVOID  *VirtualAddress);

VPAPI
VOID
DDKAPI
VideoPortMoveMemory(
  IN PVOID  Destination,
  IN PVOID  Source,
  IN ULONG  Length);

VPAPI
VOID
DDKAPI
VideoPortPutDmaAdapter(
  IN PVOID  HwDeviceExtension,
  IN PVP_DMA_ADAPTER  VpDmaAdapter);

VPAPI
LONGLONG
DDKAPI
VideoPortQueryPerformanceCounter(
  IN PVOID  HwDeviceExtension,
  OUT PLONGLONG  PerformanceFrequency  OPTIONAL);

/* VideoPortQueryServices.ServicesType constants */
typedef enum _VIDEO_PORT_SERVICES {
  VideoPortServicesAGP = 1,
  VideoPortServicesI2C,
  VideoPortServicesHeadless,
  VideoPortServicesInt10
} VIDEO_PORT_SERVICES;

VPAPI
VP_STATUS
DDKAPI
VideoPortQueryServices(
  IN PVOID HwDeviceExtension,
  IN VIDEO_PORT_SERVICES ServicesType,
  IN OUT PINTERFACE Interface);

VPAPI
VOID
DDKAPI
VideoPortQuerySystemTime(
  OUT PLARGE_INTEGER  CurrentTime);

VPAPI
BOOLEAN
DDKAPI
VideoPortQueueDpc(
  IN PVOID  HwDeviceExtension,
  IN PMINIPORT_DPC_ROUTINE  CallbackRoutine,
  IN PVOID  Context);

VPAPI
VOID
DDKAPI
VideoPortReadPortBufferUchar(
  IN PUCHAR  Port,
  OUT PUCHAR  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortReadPortBufferUlong(
  IN PULONG  Port,
  OUT PULONG  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortReadPortBufferUshort(
  IN PUSHORT  Port,
  OUT PUSHORT  Buffer,
  IN ULONG  Count);

VPAPI
UCHAR
DDKAPI
VideoPortReadPortUchar(
  IN PUCHAR  Port);

VPAPI
ULONG
DDKAPI
VideoPortReadPortUlong(
  IN PULONG  Port);

VPAPI
USHORT
DDKAPI
VideoPortReadPortUshort(
  IN PUSHORT  Port);

VPAPI
VOID
DDKAPI
VideoPortReadRegisterBufferUchar(
  IN PUCHAR  Register,
  OUT PUCHAR  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortReadRegisterBufferUlong(
  IN PULONG  Register,
  OUT PULONG  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortReadRegisterBufferUshort(
  IN PUSHORT  Register,
  OUT PUSHORT  Buffer,
  IN ULONG  Count);

VPAPI
UCHAR
DDKAPI
VideoPortReadRegisterUchar(
  IN PUCHAR  Register);

VPAPI
ULONG
DDKAPI
VideoPortReadRegisterUlong(
  IN PULONG  Register);

VPAPI
USHORT
DDKAPI
VideoPortReadRegisterUshort(
  IN PUSHORT  Register);

VPAPI
LONG
DDKAPI
VideoPortReadStateEvent(
  IN PVOID  HwDeviceExtension,
  IN PEVENT  pEvent);

VPAPI
VOID
DDKAPI
VideoPortReleaseBuffer(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Buffer);

VPAPI
VOID
DDKAPI
VideoPortReleaseCommonBuffer(
  IN PVOID  HwDeviceExtension,
  IN PVP_DMA_ADAPTER  VpDmaAdapter,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

VPAPI
VOID
DDKAPI
VideoPortReleaseDeviceLock(
  IN PVOID  HwDeviceExtension);

VPAPI
VOID
DDKAPI
VideoPortReleaseSpinLock(
  IN PVOID  HwDeviceExtension,
  IN PSPIN_LOCK  SpinLock,
  IN UCHAR  NewIrql);

VPAPI
VOID
DDKAPI
VideoPortReleaseSpinLockFromDpcLevel(
  IN PVOID  HwDeviceExtension,
  IN PSPIN_LOCK  SpinLock);

VPAPI
BOOLEAN
DDKAPI
VideoPortScanRom(
  PVOID  HwDeviceExtension,
  PUCHAR  RomBase,
  ULONG  RomLength,
  PUCHAR  String);

VPAPI
ULONG
DDKAPI
VideoPortSetBusData(
  IN PVOID  HwDeviceExtension,
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

VPAPI
VOID
DDKAPI
VideoPortSetBytesUsed(
  IN PVOID  HwDeviceExtension,
  IN OUT PDMA  pDma,
  IN ULONG  BytesUsed);

VPAPI
VOID
DDKAPI
VideoPortSetDmaContext(
  IN PVOID  HwDeviceExtension,
  OUT PDMA  pDma,
  IN PVOID  InstanceContext);

VPAPI
LONG
DDKAPI
VideoPortSetEvent(
  IN PVOID  HwDeviceExtension,
  IN PEVENT  pEvent);

VPAPI
VP_STATUS
DDKAPI
VideoPortSetRegistryParameters(
  IN PVOID  HwDeviceExtension,
  IN PWSTR  ValueName,
  IN PVOID  ValueData,
  IN ULONG  ValueLength);

VPAPI
VP_STATUS
DDKAPI
VideoPortSetTrappedEmulatorPorts(
  IN PVOID  HwDeviceExtension,
  IN ULONG  NumAccessRanges,
  IN PVIDEO_ACCESS_RANGE  AccessRange);

VPAPI
BOOLEAN
DDKAPI
VideoPortSignalDmaComplete(
  IN PVOID  HwDeviceExtension,
  IN PVOID  pDmaHandle);

VPAPI
VOID
DDKAPI
VideoPortStallExecution(
  IN ULONG  Microseconds);

VPAPI
VP_STATUS
DDKAPI
VideoPortStartDma(
  IN PVOID  HwDeviceExtension,
  IN PVP_DMA_ADAPTER  VpDmaAdapter,
  IN PVOID  Mdl,
  IN ULONG  Offset,
  IN OUT PULONG  pLength,
  IN PEXECUTE_DMA  ExecuteDmaRoutine,
  IN PVOID  Context,
  IN BOOLEAN  WriteToDevice);

VPAPI
VOID
DDKAPI
VideoPortStartTimer(
  IN PVOID  HwDeviceExtension);

VPAPI
VOID
DDKAPI
VideoPortStopTimer(
  IN PVOID  HwDeviceExtension);

/* VideoPortSynchronizeExecution.Priority constants */
typedef enum VIDEO_SYNCHRONIZE_PRIORITY {
  VpLowPriority = 0,
  VpMediumPriority,
  VpHighPriority
} VIDEO_SYNCHRONIZE_PRIORITY, *PVIDEO_SYNCHRONIZE_PRIORITY;

VPAPI
BOOLEAN
DDKAPI
VideoPortSynchronizeExecution(
  IN PVOID  HwDeviceExtension,
  IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
  IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
  IN PVOID  Context);

VPAPI
VOID
DDKAPI
VideoPortUnLockBuffer(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Mdl);

VPAPI
BOOLEAN
DDKAPI
VideoPortUnlockPages(
  IN PVOID  hwDeviceExtension,
  IN OUT PDMA  pDma);

VPAPI
BOOLEAN
DDKAPI
VideoPortUnmapDmaMemory(
  IN PVOID  HwDeviceExtension,
  IN PVOID  VirtualAddress,
  IN HANDLE  ProcessHandle,
  IN PDMA  BoardMemoryHandle);

VPAPI
VP_STATUS
DDKAPI
VideoPortUnmapMemory(
  IN PVOID  HwDeviceExtension,
  IN OUT PVOID  VirtualAddress,
  IN HANDLE  ProcessHandle);

VPAPI
VP_STATUS
DDKAPI
VideoPortVerifyAccessRanges(
  IN PVOID  HwDeviceExtension,
  IN ULONG  NumAccessRanges,
  IN PVIDEO_ACCESS_RANGE  AccessRanges);

VPAPI
VP_STATUS
DDKAPI
VideoPortWaitForSingleObject(
  IN PVOID  HwDeviceExtension,
  IN PVOID  Object,
  IN PLARGE_INTEGER  Timeout  OPTIONAL);

VPAPI
VOID
DDKAPI
VideoPortWritePortBufferUchar(
  IN PUCHAR  Port,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWritePortBufferUlong(
  IN PULONG  Port,
  IN PULONG  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWritePortBufferUshort(
  IN PUSHORT  Port,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWritePortUchar(
  IN PUCHAR  Port,
  IN UCHAR  Value);

VPAPI
VOID
DDKAPI
VideoPortWritePortUlong(
  IN PULONG  Port,
  IN ULONG  Value);

VPAPI
VOID
DDKAPI
VideoPortWritePortUshort(
  IN PUSHORT  Port,
  IN USHORT  Value);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterBufferUchar(
  IN PUCHAR  Register,
  IN PUCHAR  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterBufferUlong(
  IN PULONG  Register,
  IN PULONG  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterBufferUshort(
  IN PUSHORT  Register,
  IN PUSHORT  Buffer,
  IN ULONG  Count);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterUchar(
  IN PUCHAR  Register,
  IN UCHAR  Value);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterUlong(
  IN PULONG  Register,
  IN ULONG  Value);

VPAPI
VOID
DDKAPI
VideoPortWriteRegisterUshort(
  IN PUSHORT  Register,
  IN USHORT  Value);

VPAPI
VOID
DDKAPI
VideoPortZeroDeviceMemory(
  IN PVOID  Destination,
  IN ULONG  Length);

VPAPI
VOID
DDKAPI
VideoPortZeroMemory(
  IN PVOID  Destination,
  IN ULONG  Length);

#ifdef DBG
#define VideoDebugPrint(x) VideoPortDebugPrint x
#else
#define VideoDebugPrint(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* defined __WINDDI_H */

#endif /* __VIDEO_H */
