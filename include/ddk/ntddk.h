/*
 * ntddk.h
 *
 * Windows Device Driver Kit
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
 * DEFINES:
 *    DBG          - Debugging enabled/disabled (0/1)
 *    POOL_TAGGING - Enable pool tagging
 *    _X86_        - X86 environment
 */

#ifndef _NTDDK_
#define _NTDDK_

#if !defined(_NTHAL_) && !defined(_NTIFS_)
#define _NTDDK_INCLUDED_
#define _DDK_DRIVER_
#endif

/* Dependencies */

#define NT_INCLUDED
#define _CTYPE_DISABLE_MACROS

#include <wdm.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

/* FIXME
#include <bugcodes.h>
#include <ntiologc.h>
*/

#include <stdarg.h> // FIXME
#include <basetyps.h> // FIXME


#ifdef __cplusplus
extern "C" {
#endif

//
// GUID and UUID
//
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif
typedef GUID UUID;

typedef struct _BUS_HANDLER *PBUS_HANDLER;

#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT           12288

#ifdef _X86_

#define SIZE_OF_80387_REGISTERS   80

typedef struct _FLOATING_SAVE_AREA {
  ULONG ControlWord;
  ULONG StatusWord;
  ULONG TagWord;
  ULONG ErrorOffset;
  ULONG ErrorSelector;
  ULONG DataOffset;
  ULONG DataSelector;
  UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
  ULONG Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

#include "pshpack4.h"
typedef struct _CONTEXT {
  ULONG ContextFlags;
  ULONG Dr0;
  ULONG Dr1;
  ULONG Dr2;
  ULONG Dr3;
  ULONG Dr6;
  ULONG Dr7;
  FLOATING_SAVE_AREA FloatSave;
  ULONG SegGs;
  ULONG SegFs;
  ULONG SegEs;
  ULONG SegDs;
  ULONG Edi;
  ULONG Esi;
  ULONG Ebx;
  ULONG Edx;
  ULONG Ecx;
  ULONG Eax;
  ULONG Ebp;
  ULONG Eip;
  ULONG SegCs;
  ULONG EFlags;
  ULONG Esp;
  ULONG SegSs;
  UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;
#include "poppack.h"

#endif /* _X86_ */

#ifdef _AMD64_

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64 0x100000

#define CONTEXT_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT (CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_XSTATE (CONTEXT_AMD64 | 0x20L)

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000
#define CONTEXT_SERVICE_ACTIVE 0x10000000
#define CONTEXT_EXCEPTION_REQUEST 0x40000000
#define CONTEXT_EXCEPTION_REPORTING 0x80000000

#endif /* RC_INVOKED */

#endif /* _AMD64_ */

typedef enum _INTERLOCKED_RESULT {
  ResultNegative = RESULT_NEGATIVE,
  ResultZero = RESULT_ZERO,
  ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

typedef struct _OSVERSIONINFOA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OSVERSIONINFOW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

typedef struct _OSVERSIONINFOEXA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;

typedef struct _OSVERSIONINFOEXW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

#ifdef UNICODE
typedef OSVERSIONINFOEXW OSVERSIONINFOEX;
typedef POSVERSIONINFOEXW POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXW LPOSVERSIONINFOEX;
typedef OSVERSIONINFOW OSVERSIONINFO;
typedef POSVERSIONINFOW POSVERSIONINFO;
typedef LPOSVERSIONINFOW LPOSVERSIONINFO;
#else
typedef OSVERSIONINFOEXA OSVERSIONINFOEX;
typedef POSVERSIONINFOEXA POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXA LPOSVERSIONINFOEX;
typedef OSVERSIONINFOA OSVERSIONINFO;
typedef POSVERSIONINFOA POSVERSIONINFO;
typedef LPOSVERSIONINFOA LPOSVERSIONINFO;
#endif /* UNICODE */

/* Executive Types */

#define PROTECTED_POOL                    0x80000000

typedef struct _ZONE_SEGMENT_HEADER {
  SINGLE_LIST_ENTRY SegmentList;
  PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
  SINGLE_LIST_ENTRY FreeList;
  SINGLE_LIST_ENTRY SegmentList;
  ULONG BlockSize;
  ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

/* Executive Functions */

static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER Zone)
{
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return (PVOID) Zone->FreeList.Next;
}

static __inline PVOID
ExFreeToZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Block)
{
  ((PSINGLE_LIST_ENTRY) Block)->Next = Zone->FreeList.Next;
  Zone->FreeList.Next = ((PSINGLE_LIST_ENTRY) Block);
  return ((PSINGLE_LIST_ENTRY) Block)->Next;
}

/*
 * PVOID
 * ExInterlockedAllocateFromZone(
 *   IN PZONE_HEADER  Zone,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedAllocateFromZone(Zone, Lock) \
    ((PVOID) ExInterlockedPopEntryList(&Zone->FreeList, Lock))

/* PVOID
 * ExInterlockedFreeToZone(
 *  IN PZONE_HEADER  Zone,
 *  IN PVOID  Block,
 *  IN PKSPIN_LOCK  Lock);
 */
#define ExInterlockedFreeToZone(Zone, Block, Lock) \
    ExInterlockedPushEntryList(&(Zone)->FreeList, (PSINGLE_LIST_ENTRY)(Block), Lock)

/*
 * BOOLEAN
 * ExIsFullZone(
 *  IN PZONE_HEADER  Zone)
 */
#define ExIsFullZone(Zone) \
  ((Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY) NULL)

/* BOOLEAN
 * ExIsObjectInFirstZoneSegment(
 *     IN PZONE_HEADER Zone,
 *     IN PVOID Object);
 */
#define ExIsObjectInFirstZoneSegment(Zone,Object) \
    ((BOOLEAN)( ((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) && \
                ((PUCHAR)(Object) <  (PUCHAR)(Zone)->SegmentList.Next + \
                         (Zone)->TotalSegmentSize)) )

#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExInitializeResource ExInitializeResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  OUT PZONE_HEADER Zone,
  IN ULONG BlockSize,
  IN OUT PVOID InitialSegment,
  IN ULONG InitialSegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  OUT UUID *Uuid);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseAccessViolation(
  VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseDatatypeMisalignment(
  VOID);

#endif

#ifndef _ARC_DDK_
#define _ARC_DDK_
typedef enum _CONFIGURATION_TYPE {
  ArcSystem,
  CentralProcessor,
  FloatingPointProcessor,
  PrimaryIcache,
  PrimaryDcache,
  SecondaryIcache,
  SecondaryDcache,
  SecondaryCache,
  EisaAdapter,
  TcAdapter,
  ScsiAdapter,
  DtiAdapter,
  MultiFunctionAdapter,
  DiskController,
  TapeController,
  CdromController,
  WormController,
  SerialController,
  NetworkController,
  DisplayController,
  ParallelController,
  PointerController,
  KeyboardController,
  AudioController,
  OtherController,
  DiskPeripheral,
  FloppyDiskPeripheral,
  TapePeripheral,
  ModemPeripheral,
  MonitorPeripheral,
  PrinterPeripheral,
  PointerPeripheral,
  KeyboardPeripheral,
  TerminalPeripheral,
  OtherPeripheral,
  LinePeripheral,
  NetworkPeripheral,
  SystemMemory,
  DockingInformation,
  RealModeIrqRoutingTable,
  RealModePCIEnumeration,
  MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;
#endif /* !_ARC_DDK_ */

typedef struct _CONTROLLER_OBJECT {
  CSHORT Type;
  CSHORT Size;
  PVOID ControllerExtension;
  KDEVICE_QUEUE DeviceWaitQueue;
  ULONG Spare1;
  LARGE_INTEGER Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

typedef struct _CONFIGURATION_INFORMATION {
  ULONG DiskCount;
  ULONG FloppyCount;
  ULONG CdRomCount;
  ULONG TapeCount;
  ULONG ScsiPortCount;
  ULONG SerialCount;
  ULONG ParallelCount;
  BOOLEAN AtDiskPrimaryAddressClaimed;
  BOOLEAN AtDiskSecondaryAddressClaimed;
  ULONG Version;
  ULONG MediumChangerCount;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef
NTSTATUS
(DDKAPI *PIO_QUERY_DEVICE_ROUTINE)(
  IN PVOID Context,
  IN PUNICODE_STRING PathName,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
  IN CONFIGURATION_TYPE ControllerType,
  IN ULONG ControllerNumber,
  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
  IN CONFIGURATION_TYPE PeripheralType,
  IN ULONG PeripheralNumber,
  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation);

typedef
VOID
(DDKAPI DRIVER_REINITIALIZE)(
  IN struct _DRIVER_OBJECT *DriverObject,
  IN PVOID Context,
  IN ULONG Count);

typedef DRIVER_REINITIALIZE *PDRIVER_REINITIALIZE;

/** Filesystem runtime library routines **/

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  IN NTSTATUS Status);
#endif

/* Hardware abstraction layer routines */

#if !defined(NO_LEGACY_DRIVERS)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN ULONG BusInterruptLevel,
  IN ULONG BusInterruptVector,
  OUT PKIRQL Irql,
  OUT PKAFFINITY Affinity);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Length);

#endif

#endif /* !defined(NO_LEGACY_DRIVERS) */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION DeviceDescription,
  IN OUT PULONG NumberOfMapRegisters);

NTHALAPI
BOOLEAN
NTAPI
HalMakeBeep(
  IN ULONG Frequency);

VOID
NTAPI
HalPutDmaAdapter(
  IN PADAPTER_OBJECT DmaAdapter);

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN PHYSICAL_ADDRESS BusAddress,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG MBRTypeIdentifier,
  OUT PVOID *Buffer);
#endif

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_) 
// nothing here
#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT AdapterObject,
  IN PVOID MapRegisterBase,
  IN ULONG NumberOfMapRegisters);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT AdapterObject);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PWAIT_CONTEXT_BLOCK  Wcb,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)  */

/* I/O Manager Functions */

/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#if !(defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_))
NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG NumberOfMapRegisters,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context);
#endif

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice);

NTKERNELAPI
VOID
NTAPI
IoAllocateController(
  IN PCONTROLLER_OBJECT ControllerObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
PCONTROLLER_OBJECT
NTAPI
IoCreateController(
  IN ULONG Size);

NTKERNELAPI
VOID
NTAPI
IoDeleteController(
  IN PCONTROLLER_OBJECT ControllerObject);

NTKERNELAPI
VOID
NTAPI
IoFreeController(
  IN PCONTROLLER_OBJECT ControllerObject);

NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(
  VOID);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(
  IN PETHREAD Thread);

NTKERNELAPI
VOID
NTAPI
IoCancelFileOpen(
  IN PDEVICE_OBJECT DeviceObject,
  IN PFILE_OBJECT FileObject);

NTKERNELAPI
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(
  VOID);

NTKERNELAPI
PIRP
NTAPI
IoMakeAssociatedIrp(
  IN PIRP Irp,
  IN CCHAR StackSize);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryDeviceDescription(
  IN PINTERFACE_TYPE BusType OPTIONAL,
  IN PULONG BusNumber OPTIONAL,
  IN PCONFIGURATION_TYPE ControllerType OPTIONAL,
  IN PULONG ControllerNumber OPTIONAL,
  IN PCONFIGURATION_TYPE PeripheralType OPTIONAL,
  IN PULONG PeripheralNumber OPTIONAL,
  IN PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
  IN OUT PVOID Context OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRaiseHardError(
  IN PIRP Irp,
  IN PVPB Vpb OPTIONAL,
  IN PDEVICE_OBJECT RealDeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoRaiseInformationalHardError(
  IN NTSTATUS ErrorStatus,
  IN PUNICODE_STRING String OPTIONAL,
  IN PKTHREAD Thread OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT DriverObject,
  IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRegisterDriverReinitialization(
  IN PDRIVER_OBJECT DriverObject,
  IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceByPointer(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportDetectedDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN INTERFACE_TYPE LegacyBusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PCM_RESOURCE_LIST ResourceList OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
  IN BOOLEAN ResourceAssigned,
  IN OUT PDEVICE_OBJECT *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceForDetection(
  IN PDRIVER_OBJECT DriverObject,
  IN PCM_RESOURCE_LIST DriverList OPTIONAL,
  IN ULONG DriverListSize OPTIONAL,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
  IN ULONG DeviceListSize OPTIONAL,
  OUT PBOOLEAN ConflictDetected);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceUsage(
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PCM_RESOURCE_LIST DriverList OPTIONAL,
  IN ULONG DriverListSize OPTIONAL,
  IN PDEVICE_OBJECT DeviceObject,
  IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
  IN ULONG DeviceListSize OPTIONAL,
  IN BOOLEAN OverrideConflict,
  OUT PBOOLEAN ConflictDetected);

NTKERNELAPI
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(
  IN PIRP Irp,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoAssignResources(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources OPTIONAL,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDisk(
  IN PDEVICE_OBJECT DeviceObject,
  IN PCREATE_DISK Disk OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadDiskSignature(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG BytesPerSector,
  OUT PDISK_SIGNATURE Signature);

NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN BOOLEAN ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadPartitionTableEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX **PartitionBuffer);

NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG PartitionNumber,
  IN ULONG PartitionType);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetPartitionInformationEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG PartitionNumber,
  IN struct _SET_PARTITION_INFORMATION_EX *PartitionInfo);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetSystemPartition(
  IN PUNICODE_STRING VolumeNameString);

NTKERNELAPI
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(
  IN BOOLEAN EnableHardErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyPartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN BOOLEAN FixErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(
  IN PVOID VolumeDeviceObject,
  OUT PUNICODE_STRING DosName);

NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG SectorsPerTrack,
  IN ULONG NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWritePartitionTableEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX *DriveLayout);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/** Kernel debugger routines **/

NTSYSAPI
ULONG
NTAPI
DbgPrompt(
  IN PCCH Prompt,
  OUT PCH Response,
  IN ULONG MaximumResponseLength);

/* Kernel Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  IN ULONG BugCheckCode);

NTKERNELAPI
LONG
NTAPI
KePulseEvent(
  IN OUT PRKEVENT Event,
  IN KPRIORITY Increment,
  IN BOOLEAN Wait);

NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  IN OUT PRKTHREAD Thread,
  IN LONG Increment);

#endif

/* Memory Manager Types */

typedef struct _PHYSICAL_MEMORY_RANGE {
  PHYSICAL_ADDRESS BaseAddress;
  LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

/* Memory Manager Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(
  VOID);

NTKERNELAPI
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(
  IN PVOID BaseAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmGetVirtualForPhysical(
  IN PHYSICAL_ADDRESS PhysicalAddress);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapUserAddressesToPage(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes,
  IN PVOID PageAddress);

NTKERNELAPI
PVOID
NTAPI
MmMapVideoDisplay(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSessionSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSystemSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(
  VOID);

NTKERNELAPI
VOID
NTAPI
MmLockPagableSectionByHandle(
  IN PVOID ImageSectionHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(
  IN PVOID MappedBase);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(
  IN PVOID MappedBase);

NTKERNELAPI
VOID
NTAPI
MmUnsecureVirtualMemory(
  IN HANDLE SecureHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS StartAddress,
  IN OUT PLARGE_INTEGER NumberOfBytes);

NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  IN PVOID Address,
  IN SIZE_T Size,
  IN ULONG ProbeMode);

NTKERNELAPI
VOID
NTAPI
MmUnmapVideoDisplay(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

/* NtXxx Functions */

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
  OUT PHANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN PCLIENT_ID ClientId OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
  IN HANDLE ProcessHandle,
  IN PROCESSINFOCLASS ProcessInformationClass,
  OUT PVOID ProcessInformation OPTIONAL,
  IN ULONG ProcessInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

/** Process manager routines **/

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
  IN BOOLEAN Remove);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(
  VOID);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentThreadId(
  VOID);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetVersion(
  OUT PULONG MajorVersion OPTIONAL,
  OUT PULONG MinorVersion OPTIONAL,
  OUT PULONG BuildNumber OPTIONAL,
  OUT PUNICODE_STRING CSDVersion OPTIONAL);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
HANDLE
NTAPI
PsGetProcessId(
  IN PEPROCESS Process);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

extern NTKERNELAPI PEPROCESS PsInitialSystemProcess;

/* RTL Types */

typedef struct _RTL_SPLAY_LINKS {
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

/* RTL Functions */

#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)

#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;
#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
  OUT PVOID *CallersAddress,
  OUT PVOID *CallersCaller);
#endif

#endif

#if !defined(MIDL_PASS)

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(
  IN LONG Val)
{
  LUID Luid;
  LARGE_INTEGER Temp;

  Temp.QuadPart = Val;
  Luid.LowPart = Temp.u.LowPart;
  Luid.HighPart = Temp.u.HighPart;
  return Luid;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
  IN ULONG Val)
{
  LUID Luid;

  Luid.LowPart = Val;
  Luid.HighPart = 0;
  return Luid;
}

#endif

#if defined(_AMD64_) || defined(_IA64_)
//DECLSPEC_DEPRECATED_DDK_WINXP
__inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
  if (Remainder)
    Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
  return ret;
}

#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL);
#endif

#endif /* defined(_AMD64_) || defined(_IA64_) */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
  IN PCUNICODE_STRING  String1,
  IN PCUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlUpperString(
  IN OUT PSTRING  DestinationString,
  IN const PSTRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK AccessMask,
  IN PGENERIC_MAPPING GenericMapping);

NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
  IN PVOID VolumeDeviceObject,
  OUT PUNICODE_STRING DosName);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
  IN OUT PRTL_OSVERSIONINFOW lpVersionInformation);

NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
  IN PRTL_OSVERSIONINFOEXW VersionInfo,
  IN ULONG TypeMask,
  IN ULONGLONG ConditionMask);

NTSYSAPI
LONG
NTAPI
RtlCompareString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
  OUT PSTRING DestinationString,
  IN const PSTRING SourceString OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  IN PCSZ String,
  IN ULONG Base OPTIONAL,
  OUT PULONG Value);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
  IN CHAR Character);

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
  OUT PVOID *Callers,
  IN ULONG Count,
  IN ULONG Flags);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

/* Security reference monitor routines */

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  IN LUID PrivilegeValue,
  IN KPROCESSOR_MODE PreviousMode);
#endif

/* ZwXxx Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTSTATUS
NTAPI
ZwCancelTimer(
  IN HANDLE TimerHandle,
  OUT PBOOLEAN CurrentState OPTIONAL);

NTSTATUS
NTAPI
ZwCreateTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN TIMER_TYPE TimerType);

NTSTATUS
NTAPI
ZwOpenTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
  IN HANDLE ThreadHandle,
  IN THREADINFOCLASS ThreadInformationClass,
  IN PVOID ThreadInformation,
  IN ULONG ThreadInformationLength);

NTSTATUS
NTAPI
ZwSetTimer(
  IN HANDLE TimerHandle,
  IN PLARGE_INTEGER DueTime,
  IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
  IN PVOID TimerContext OPTIONAL,
  IN BOOLEAN ResumeTimer,
  IN LONG Period OPTIONAL,
  OUT PBOOLEAN PreviousState OPTIONAL);

#endif

/* Windows Device Driver Kit */
#include "winddk.h"

#ifdef __cplusplus
}
#endif


#endif /* _NTDDK_ */
