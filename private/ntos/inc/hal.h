/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hal.h

Abstract:

    This header file defines the Hardware Architecture Layer (HAL) interfaces
    that are exported by a system vendor to the NT system.

Author:

    David N. Cutler (davec) 25-Apr-1991


Revision History:

--*/

#ifndef _HAL_
#define _HAL_

//
// Define OEM bitmapped font check values.
//

#define OEM_FONT_VERSION 0x200
#define OEM_FONT_TYPE 0
#define OEM_FONT_ITALIC 0
#define OEM_FONT_UNDERLINE 0
#define OEM_FONT_STRIKEOUT 0
#define OEM_FONT_CHARACTER_SET 255
#define OEM_FONT_FAMILY (3 << 4)

//
// Define OEM bitmapped font file header structure.
//
// N.B. this is a packed structure.
//

#include "pshpack1.h"
typedef struct _OEM_FONT_FILE_HEADER {
    USHORT Version;
    ULONG FileSize;
    UCHAR Copyright[60];
    USHORT Type;
    USHORT Points;
    USHORT VerticleResolution;
    USHORT HorizontalResolution;
    USHORT Ascent;
    USHORT InternalLeading;
    USHORT ExternalLeading;
    UCHAR Italic;
    UCHAR Underline;
    UCHAR StrikeOut;
    USHORT Weight;
    UCHAR CharacterSet;
    USHORT PixelWidth;
    USHORT PixelHeight;
    UCHAR Family;
    USHORT AverageWidth;
    USHORT MaximumWidth;
    UCHAR FirstCharacter;
    UCHAR LastCharacter;
    UCHAR DefaultCharacter;
    UCHAR BreakCharacter;
    USHORT WidthInBytes;
    ULONG Device;
    ULONG Face;
    ULONG BitsPointer;
    ULONG BitsOffset;
    UCHAR Filler;
    struct {
        USHORT Width;
        USHORT Offset;
    } Map[1];
} OEM_FONT_FILE_HEADER, *POEM_FONT_FILE_HEADER;
#include "poppack.h"


// begin_ntddk begin_wdm
//
// Define the device description structure.
//

typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Dma64BitAddresses;
    ULONG BusNumber; // unused for WDM
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

//
// Define the supported version numbers for the device description structure.
//

#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1

// end_ntddk end_wdm

//
// Boot record disk partition table entry structure format.
//

typedef struct _PARTITION_DESCRIPTOR {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    UCHAR StartingCylinderLsb;      // Not used
    UCHAR StartingCylinderMsb;      // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    UCHAR EndingCylinderLsb;        // Not used
    UCHAR EndingCylinderMsb;        // Not used
    UCHAR StartingSectorLsb0;       // Hidden sectors
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;      // Sectors in this partition
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Number of partition table entries
//

#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Partition table record and boot signature offsets in 16-bit words.
//

#define PARTITION_TABLE_OFFSET         (0x1be / 2)
#define BOOT_SIGNATURE_OFFSET          ((0x200 / 2) - 1)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

//
// Initial size of the Partition list structure.
//

#define PARTITION_BUFFER_SIZE          2048

//
// Partition active flag - i.e., boot indicator
//

#define PARTITION_ACTIVE_FLAG          0x80


// begin_ntddk
//
// The following function prototypes are for HAL routines with a prefix of Hal.
//
// General functions.
//

typedef
BOOLEAN
(*PHAL_RESET_DISPLAY_PARAMETERS) (
    IN ULONG Columns,
    IN ULONG Rows
    );

NTHALAPI
VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    );

// end_ntddk

NTHALAPI
VOID
HalDisplayString (
    PUCHAR String
    );

NTHALAPI
VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    );

NTHALAPI
VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    );

NTHALAPI
BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
VOID
HalProcessorIdle(
    VOID
    );

NTHALAPI
VOID
HalReportResourceUsage (
    VOID
    );

NTHALAPI
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    );

//
// Get and set environment variable values.
//

NTHALAPI
ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    );

NTHALAPI
ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    );

//
// Cache and write buffer flush functions.
//
//

#if defined(_ALPHA_) || defined(_IA64_)    // ntddk ntifs ntndis
                                                                                // ntddk ntifs ntndis

NTHALAPI
VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTHALAPI
VOID
HalFlushDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

NTHALAPI                                        // ntddk ntifs ntndis
ULONG                                           // ntddk ntifs ntndis
HalGetDmaAlignmentRequirement (                 // ntddk ntifs ntndis
    VOID                                        // ntddk ntifs ntndis
    );                                          // ntddk ntifs ntndis
                                                // ntddk ntifs ntndis
NTHALAPI
VOID
HalPurgeDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalPurgeIcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepDcache (
    VOID
    );

NTHALAPI
VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepIcache (
    VOID
    );

NTHALAPI
VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    );


NTHALAPI
VOID
HalZeroPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PFN_NUMBER PageFrame
    );

#endif                                          // ntddk ntifs ntndis
                                                // ntddk ntifs ntndis
#if defined(_M_IX86)                            // ntddk ntifs ntndis
                                                // ntddk ntifs ntndis
#define HalGetDmaAlignmentRequirement() 1L      // ntddk ntifs ntndis

NTHALAPI
VOID
HalHandleNMI (
    IN OUT PVOID NmiInformation
    );

#endif                                          // ntddk ntifs ntndis
                                                // ntddk ntifs wdm ntndis

#if defined(_M_IA64)

NTHALAPI
VOID
HalSweepCacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    );


NTHALAPI
LONGLONG
HalCallPal (
    IN  ULONGLONG  FunctionIndex,
    IN  ULONGLONG  Arguement1,
    IN  ULONGLONG  Arguement2,
    IN  ULONGLONG  Arguement3,
    OUT PULONGLONG ReturnValue0,
    OUT PULONGLONG ReturnValue1,
    OUT PULONGLONG ReturnValue2,
    OUT PULONGLONG ReturnValue3  
    );

#endif

NTHALAPI                                        // ntddk ntifs wdm ntndis
VOID                                            // ntddk ntifs wdm ntndis
KeFlushWriteBuffer (                            // ntddk ntifs wdm ntndis
    VOID                                        // ntddk ntifs wdm ntndis
    );                                          // ntddk ntifs wdm ntndis
                                                // ntddk ntifs wdm ntndis

#if defined(_ALPHA_)

NTHALAPI
PVOID
HalCreateQva(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN PVOID VirtualAddress
    );

NTHALAPI
PVOID
HalDereferenceQva(
    PVOID Qva,
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber
    );

#endif

#if !defined(_X86_)

NTHALAPI
BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    );

#endif

//
// Profiling functions.
//

NTHALAPI
VOID
HalCalibratePerformanceCounter (
    IN volatile PLONG Number,
    IN ULONGLONG NewCount
    );

NTHALAPI
ULONG_PTR
HalSetProfileInterval (
    IN ULONG_PTR Interval
    );


NTHALAPI
VOID
HalStartProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

NTHALAPI
VOID
HalStopProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

//
// Timer and interrupt functions.
//

NTHALAPI
BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    );

NTHALAPI
BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    );

#if defined(_M_IX86)

NTHALAPI
VOID
FASTCALL
HalRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

ULONG
FASTCALL
HalSystemVectorDispatchEntry (
   IN ULONG Vector,
   OUT PKINTERRUPT_ROUTINE **FlatDispatch,
   OUT PKINTERRUPT_ROUTINE *NoConnection
   );

#endif

//
// Firmware interface functions.
//

NTHALAPI
VOID
HalReturnToFirmware (
    IN FIRMWARE_REENTRY Routine
    );

//
// System interrupts functions.
//

NTHALAPI
VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    );

NTHALAPI
BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    );

// begin_ntddk
//
// I/O driver configuration functions.
//
#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

NTHALAPI
ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTHALAPI
ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

NTHALAPI
ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTHALAPI
BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

//
// Values for AddressSpace parameter of HalTranslateBusAddress
//
// BUGBUG--figure out which flags should be added to resource descriptor structures
//
//      0x0         - Memory space
//      0x1         - Port space
//      0x2 - 0x1F  - Address spaces specific for Alpha
//                      0x2 - UserMode view of memory space
//                      0x3 - UserMode view of port space
//                      0x4 - Dense memory space
//                      0x5 - reserved
//                      0x6 - UserMode view of dense memory space
//                      0x7 - 0x1F - reserved
//

NTHALAPI
PVOID
HalAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN OUT PULONG NumberOfMapRegisters
    );

#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

NTHALAPI
ULONG
HalGetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTHALAPI
PADAPTER_OBJECT
HalGetAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

// end_ntddk

#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );
#endif // NO_LEGACY_DRIVERS

// begin_ntddk
//
// System beep functions.
//
#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    );
#endif // NO_LEGACY_DRIVERS

//
// The following function prototypes are for HAL routines with a prefix of Io.
//
// DMA adapter object functions.
//

// end_ntddk

#if defined(NO_LEGACY_DRIVERS)
NTKERNELAPI
VOID
IoAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

#else
NTHALAPI
VOID
IoAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );
#endif // NO_LEGACY_DRIVERS

// begin_ntddk


#if defined(NO_LEGACY_DRIVERS)
NTKERNELAPI
NTSTATUS
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

NTKERNELAPI
NTSTATUS
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTKERNELAPI
NTSTATUS
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

#else
NTHALAPI
NTSTATUS
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

NTHALAPI
NTSTATUS
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTHALAPI
NTSTATUS
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );
#endif // NO_LEGACY_DRIVERS

// end_ntddk

//
// Multi-Processorfunctions.
//

NTHALAPI
BOOLEAN
HalAllProcessorsStarted (
    VOID
    );

NTHALAPI
VOID
HalInitializeProcessor (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    );

NTHALAPI
VOID
HalRequestIpi (
    IN ULONG Mask
    );

//
// The following function prototypes are for HAL routines with a prefix of Kd.
//
// Kernel debugger port functions.
//

NTHALAPI
BOOLEAN
KdPortInitialize (
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    BOOLEAN Initialize
    );

NTHALAPI
ULONG
KdPortGetByte (
    OUT PUCHAR Input
    );

NTHALAPI
ULONG
KdPortPollByte (
    OUT PUCHAR Input
    );

NTHALAPI
VOID
KdPortPutByte (
    IN UCHAR Output
    );

NTHALAPI
VOID
KdPortRestore (
    VOID
    );

NTHALAPI
VOID
KdPortSave (
    VOID
    );

//
// The following function prototypes are for HAL routines with a prefix of Ke.
//
// begin_ntddk begin_ntifs begin_wdm
//
// Performance counter function.
//

NTHALAPI
LARGE_INTEGER
KeQueryPerformanceCounter (
   IN PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

// begin_ntndis
//
// Stall processor execution function.
//

NTHALAPI
VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    );

// end_ntddk end_ntifs end_wdm end_ntndis


//*****************************************************************************
//
//  HAL BUS EXTENDERS

//
// Bus handlers
//

// begin_ntddk

typedef
VOID
(*PDEVICE_CONTROL_COMPLETION)(
    IN struct _DEVICE_CONTROL_CONTEXT     *ControlContext
    );

typedef struct _DEVICE_CONTROL_CONTEXT {
    NTSTATUS                Status;
    PDEVICE_HANDLER_OBJECT  DeviceHandler;
    PDEVICE_OBJECT          DeviceObject;
    ULONG                   ControlCode;
    PVOID                   Buffer;
    PULONG                  BufferLength;
    PVOID                   Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

// end_ntddk

typedef struct _HAL_DEVICE_CONTROL {
    //
    // Handler this DeviceControl is for
    //
    struct _BUS_HANDLER         *Handler;
    struct _BUS_HANDLER         *RootHandler;

    //
    // Bus specific storage for this Context
    //
    PVOID                       BusExtensionData;

    //
    // Reserved for HALs use
    //
    ULONG                       HalReserved[4];

    //
    // Reserved for BusExtneder use
    //
    ULONG                       BusExtenderReserved[4];

    //
    // DeviceControl Context and the CompletionRoutine
    //
    PDEVICE_CONTROL_COMPLETION  CompletionRoutine;
    DEVICE_CONTROL_CONTEXT      DeviceControl;

} HAL_DEVICE_CONTROL_CONTEXT, *PHAL_DEVICE_CONTROL_CONTEXT;


typedef
ULONG
(*PGETSETBUSDATA)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef
ULONG
(*PGETINTERRUPTVECTOR)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

typedef
BOOLEAN
(*PTRANSLATEBUSADDRESS)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef NTSTATUS
(*PADJUSTRESOURCELIST)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

typedef PDEVICE_HANDLER_OBJECT
(*PREFERENCE_DEVICE_HANDLER)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    SlotNumber
    );

//typedef VOID
//(*PDEREFERENCE_DEVICE_HANDLER)(
//    IN PDEVICE_HANDLER_OBJECT   DeviceHandler
//    );

typedef NTSTATUS
(*PASSIGNSLOTRESOURCES)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

typedef
NTSTATUS
(*PQUERY_BUS_SLOTS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

typedef ULONG
(*PGET_SET_DEVICE_INSTANCE_DATA)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );


typedef
NTSTATUS
(*PDEVICE_CONTROL)(
    IN PHAL_DEVICE_CONTROL_CONTEXT Context
    );


typedef
NTSTATUS
(*PHIBERNATEBRESUMEBUS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler
    );

//
// Supported range structures
//

#define BUS_SUPPORTED_RANGE_VERSION 1

typedef struct _SUPPORTED_RANGE {
    struct _SUPPORTED_RANGE     *Next;
    ULONG                       SystemAddressSpace;
    LONGLONG                    SystemBase;
    LONGLONG                    Base;
    LONGLONG                    Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES {
    USHORT              Version;
    BOOLEAN             Sorted;
    UCHAR               Reserved;

    ULONG               NoIO;
    SUPPORTED_RANGE     IO;

    ULONG               NoMemory;
    SUPPORTED_RANGE     Memory;

    ULONG               NoPrefetchMemory;
    SUPPORTED_RANGE     PrefetchMemory;

    ULONG               NoDma;
    SUPPORTED_RANGE     Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// Bus handler structure
//

#define BUS_HANDLER_VERSION 1

typedef struct _BUS_HANDLER {
    //
    // Version of structure
    //

    ULONG                           Version;

    //
    // This bus handler struction is for the following bus
    //

    INTERFACE_TYPE                  InterfaceType;
    BUS_DATA_TYPE                   ConfigurationType;
    ULONG                           BusNumber;

    //
    // Device object for this bus externder, or NULL if it is
    // a hal internal bus extender
    //

    PDEVICE_OBJECT                  DeviceObject;

    //
    // The parent handlers for this bus
    //

    struct _BUS_HANDLER             *ParentHandler;

    //
    // Bus specific strorage
    //

    PVOID                           BusData;

    //
    // Amount of bus specific strogare needed for DeviceControl function calls
    //

    ULONG                           DeviceControlExtensionSize;

    //
    // Supported address ranges this bus allows
    //

    PSUPPORTED_RANGES               BusAddresses;

    //
    // For future use
    //

    ULONG                           Reserved[4];

    //
    // Handlers for this bus
    //

    PGETSETBUSDATA                  GetBusData;
    PGETSETBUSDATA                  SetBusData;
    PADJUSTRESOURCELIST             AdjustResourceList;
    PASSIGNSLOTRESOURCES            AssignSlotResources;
    PGETINTERRUPTVECTOR             GetInterruptVector;
    PTRANSLATEBUSADDRESS            TranslateBusAddress;

    PVOID                           Spare1;
    PVOID                           Spare2;
    PVOID                           Spare3;
    PVOID                           Spare4;
    PVOID                           Spare5;
    PVOID                           Spare6;
    PVOID                           Spare7;
    PVOID                           Spare8;

} BUS_HANDLER, *PBUS_HANDLER;


VOID
HalpInitBusHandler (
    VOID
    );

typedef
NTSTATUS
(*PINSTALL_BUS_HANDLER)(
      IN PBUS_HANDLER   Bus
      );

typedef
NTSTATUS
(*pHalRegisterBusHandler)(
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

NTSTATUS
HaliRegisterBusHandler (
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

// begin_ntddk
typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForBus) (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );
// end_ntddk

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

typedef VOID
(FASTCALL *pHalRefernceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDerefernceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForConfigSpace) (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

// begin_ntddk
typedef
VOID
(FASTCALL *pHalReferenceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );
// end_ntddk

VOID
FASTCALL
HaliReferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDereferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );


NTSTATUS
HaliQueryBusSlots (
    IN PBUS_HANDLER             BusHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

NTSTATUS
HaliAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SupportedRanges,
    IN PSUPPORTED_RANGE                     InterruptRanges,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HaliLocateHiberRanges (
    IN PVOID MemoryMap
    );


typedef
VOID
(*pHalSetWakeEnable)(
    IN BOOLEAN          Enable
    );


typedef
VOID
(*pHalSetWakeAlarm)(
    IN ULONGLONG        WakeTime,
    IN PTIME_FIELDS     WakeTimeFields
    );

typedef
VOID
(*pHalLocateHiberRanges)(
    IN PVOID MemoryMap
    );


// begin_ntddk

//*****************************************************************************
//      HAL Function dispatch
//

typedef enum _HAL_QUERY_INFORMATION_CLASS {
    HalInstalledBusInformation,
    HalProfileSourceInformation,
    HalInformationClassUnused1,
    HalPowerInformation,
    HalProcessorSpeedInformation,
    HalCallbackInformation,
    HalMapRegisterInformation,
    HalMcaLogInformation,
    HalFrameBufferCachingInformation,
    HalDisplayBiosInformation,
    HalProcessorFeatureInformation
    // information levels >= 0x8000000 reserved for OEM use
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS {
    HalProfileSourceInterval,
    HalProfileSourceInterruptHandler,
    HalMcaRegisterDriver
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;


typedef
NTSTATUS
(*pHalQuerySystemInformation)(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );

NTSTATUS
HaliQuerySystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );

typedef
NTSTATUS
(*pHalSetSystemInformation)(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

NTSTATUS
HaliSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

typedef
VOID
(FASTCALL *pHalExamineMBR)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    );

typedef
VOID
(FASTCALL *pHalIoAssignDriveLetters)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

typedef
NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

typedef
NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

typedef
NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

typedef
NTSTATUS
(*pHalQueryBusSlots)(
    IN PBUS_HANDLER         BusHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

typedef
NTSTATUS
(*pHalInitPnpDriver)(
    VOID
    );

NTSTATUS
HaliInitPnpDriver(
    VOID
    );

typedef struct _PM_DISPATCH_TABLE {
    ULONG   Signature;
    ULONG   Version;
    PVOID   Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef
NTSTATUS
(*pHalInitPowerManagement)(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

NTSTATUS
HaliInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

typedef
struct _DMA_ADAPTER *
(*pHalGetDmaAdapter)(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

struct _DMA_ADAPTER *
HaliGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

typedef
NTSTATUS
(*pHalGetInterruptTranslator)(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

NTSTATUS
HaliGetInterruptTranslator(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

typedef
BOOLEAN
(*pHalTranslateBusAddress)(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef
NTSTATUS
(*pHalAssignSlotResources) (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

typedef
VOID
(*pHalHaltSystem) (
    VOID
    );

typedef
VOID
(*pHalResetDisplay) (
    VOID
    );

typedef
BOOLEAN
(*pHalFindBusAddressTranslation) (
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

typedef struct {
    ULONG                           Version;
    pHalQuerySystemInformation      HalQuerySystemInformation;
    pHalSetSystemInformation        HalSetSystemInformation;
    pHalQueryBusSlots               HalQueryBusSlots;
    ULONG                           Spare1;
    pHalExamineMBR                  HalExamineMBR;
    pHalIoAssignDriveLetters        HalIoAssignDriveLetters;
    pHalIoReadPartitionTable        HalIoReadPartitionTable;
    pHalIoSetPartitionInformation   HalIoSetPartitionInformation;
    pHalIoWritePartitionTable       HalIoWritePartitionTable;

    pHalHandlerForBus               HalReferenceHandlerForBus;
    pHalReferenceBusHandler         HalReferenceBusHandler;
    pHalReferenceBusHandler         HalDereferenceBusHandler;

    pHalInitPnpDriver               HalInitPnpDriver;
    pHalInitPowerManagement         HalInitPowerManagement;

    pHalGetDmaAdapter               HalGetDmaAdapter;
    pHalGetInterruptTranslator      HalGetInterruptTranslator;
} HAL_DISPATCH, *PHAL_DISPATCH;

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_DISPATCH   HalDispatchTable;
#define HALDISPATCH     HalDispatchTable

#else

extern  HAL_DISPATCH    HalDispatchTable;
#define HALDISPATCH     (&HalDispatchTable)

#endif

#define HAL_DISPATCH_VERSION        2

#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots
#define HalExamineMBR                   HALDISPATCH->HalExamineMBR
#define HalIoAssignDriveLetters         HALDISPATCH->HalIoAssignDriveLetters
#define HalIoReadPartitionTable         HALDISPATCH->HalIoReadPartitionTable
#define HalIoSetPartitionInformation    HALDISPATCH->HalIoSetPartitionInformation
#define HalIoWritePartitionTable        HALDISPATCH->HalIoWritePartitionTable

#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler

#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement

#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator
// end_ntddk

typedef struct {
    ULONG                               Version;

    pHalHandlerForBus                   HalHandlerForBus;
    pHalHandlerForConfigSpace           HalHandlerForConfigSpace;
    pHalLocateHiberRanges               HalLocateHiberRanges;

    pHalRegisterBusHandler              HalRegisterBusHandler;

    pHalSetWakeEnable                   HalSetWakeEnable;
    pHalSetWakeAlarm                    HalSetWakeAlarm;

    pHalTranslateBusAddress             HalPciTranslateBusAddress;
    pHalAssignSlotResources             HalPciAssignSlotResources;

    pHalHaltSystem                      HalHaltSystem;

    pHalFindBusAddressTranslation       HalFindBusAddressTranslation;

    pHalResetDisplay                    HalResetDisplay;

} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_PRIVATE_DISPATCH           HalPrivateDispatchTable;
#define HALPDISPATCH                    HalPrivateDispatchTable

#else

extern  HAL_PRIVATE_DISPATCH            HalPrivateDispatchTable;
#define HALPDISPATCH                    (&HalPrivateDispatchTable)

#endif

#define HAL_PRIVATE_DISPATCH_VERSION        1

#define HalRegisterBusHandler           HALPDISPATCH->HalRegisterBusHandler
#define HalHandlerForBus                HALPDISPATCH->HalHandlerForBus
#define HalHandlerForConfigSpace        HALPDISPATCH->HalHandlerForConfigSpace
#define HalLocateHiberRanges            HALPDISPATCH->HalLocateHiberRanges
#define HalSetWakeEnable                HALPDISPATCH->HalSetWakeEnable
#define HalSetWakeAlarm                 HALPDISPATCH->HalSetWakeAlarm
#define HalHaltSystem                   HALPDISPATCH->HalHaltSystem
#define HalResetDisplay                 HALPDISPATCH->HalResetDisplay

// begin_ntddk

//
// HAL System Information Structures.
//

// for the information class "HalInstalledBusInformation"
typedef struct _HAL_BUS_INFORMATION{
    INTERFACE_TYPE  BusType;
    BUS_DATA_TYPE   ConfigurationType;
    ULONG           BusNumber;
    ULONG           Reserved;
} HAL_BUS_INFORMATION, *PHAL_BUS_INFORMATION;

// for the information class "HalProfileSourceInformation"
typedef struct _HAL_PROFILE_SOURCE_INFORMATION {
    KPROFILE_SOURCE Source;
    BOOLEAN Supported;
    ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

// for the information class "HalProfileSourceInterval"
typedef struct _HAL_PROFILE_SOURCE_INTERVAL {
    KPROFILE_SOURCE Source;
    ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

// for the information class "HalDispayBiosInformation"
typedef enum _HAL_DISPLAY_BIOS_INFORMATION {
    HalDisplayInt10Bios,
    HalDisplayEmulatedBios,
    HalDisplayNoBios
} HAL_DISPLAY_BIOS_INFORMATION, *PHAL_DISPLAY_BIOS_INFORMATION;

// for the information class "HalPowerInformation"
typedef struct _HAL_POWER_INFORMATION {
    ULONG   TBD;
} HAL_POWER_INFORMATION, *PHAL_POWER_INFORMATION;

// for the information class "HalProcessorSpeedInformation"
typedef struct _HAL_PROCESSOR_SPEED_INFO {
    ULONG   TBD;
} HAL_PROCESSOR_SPEED_INFORMATION, *PHAL_PROCESSOR_SPEED_INFORMATION;

// for the information class "HalCallbackInformation"
typedef struct _HAL_CALLBACKS {
    PCALLBACK_OBJECT  SetSystemInformation;
    PCALLBACK_OBJECT  BusCheck;
} HAL_CALLBACKS, *PHAL_CALLBACKS;

// for the information class "HalProcessorFeatureInformation"
typedef struct _HAL_PROCESSOR_FEATURE {
    ULONG UsableFeatureBits;
} HAL_PROCESSOR_FEATURE;

#if defined(_X86_) || defined(_IA64_)

// for the information class "HalMcaLogInformation"

//
// ADDR register for each MCA bank
//

typedef union _MCI_ADDR{
    struct {
        ULONG Address;
        ULONG Reserved;
    };

    ULONGLONG   QuadPart;
} MCI_ADDR, *PMCI_ADDR;


typedef enum {
    HAL_MCE_RECORD,
    HAL_MCA_RECORD
} MCA_EXCEPTION_TYPE;

//
// MCA exception log entry
// Defined as a union to contain MCA specific log or Pentium style MCE info.
//

typedef struct _MCA_EXCEPTION {

    ULONG               VersionNumber;      // Version number of this record type
    MCA_EXCEPTION_TYPE  ExceptionType;      // MCA or MCE
    LARGE_INTEGER       TimeStamp;          // exception recording timestamp
    ULONG               ProcessorNumber;

    union {
        struct {
            UCHAR           BankNumber;
            MCI_STATS       Status;
            MCI_ADDR        Address;
            ULONGLONG       Misc;
        } Mca;

        struct {
            ULONGLONG       Address;        // physical addr of cycle causing the error
            ULONGLONG       Type;           // cycle specification causing the error
        } Mce;
    } u;

} MCA_EXCEPTION, *PMCA_EXCEPTION;


// for the information class "HalMcaRegisterDriver"

typedef
VOID
(*PDRIVER_EXCPTN_CALLBACK) (
    IN PVOID            Context,
    IN PMCA_EXCEPTION   BankLog
);

//
// Structure to record the callbacks from driver
//
typedef struct _MCA_DRIVER_INFO {
    PDRIVER_EXCPTN_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE      DpcCallback;
    PVOID                   DeviceContext;
} MCA_DRIVER_INFO, *PMCA_DRIVER_INFO;

#endif

//  begin_wdm begin_ntndis

typedef struct _SCATTER_GATHER_ELEMENT {
    PHYSICAL_ADDRESS Address;
    ULONG Length;
    ULONG_PTR Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

#pragma warning(disable:4200)
typedef struct _SCATTER_GATHER_LIST {
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    SCATTER_GATHER_ELEMENT Elements[];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;
#pragma warning(default:4200)

// end_ntndis

typedef struct _DMA_OPERATIONS *PDMA_OPERATIONS;

typedef struct _DMA_ADAPTER {
    USHORT Version;
    USHORT Size;
    PDMA_OPERATIONS DmaOperations;
    // Private Bus Device Driver data follows,
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID (*PPUT_DMA_ADAPTER)(
    PDMA_ADAPTER DmaAdapter
    );

typedef PVOID (*PALLOCATE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

typedef VOID (*PFREE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

typedef NTSTATUS (*PALLOCATE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

typedef BOOLEAN (*PFLUSH_ADAPTER_BUFFERS)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef VOID (*PFREE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID (*PFREE_MAP_REGISTERS)(
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

typedef PHYSICAL_ADDRESS (*PMAP_TRANSFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef ULONG (*PGET_DMA_ALIGNMENT)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef ULONG (*PREAD_DMA_COUNTER)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID
(*PDRIVER_LIST_CONTROL)(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

typedef NTSTATUS
(*PGET_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

typedef VOID
(*PPUT_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

typedef struct _DMA_OPERATIONS {
    ULONG Size;
    PPUT_DMA_ADAPTER PutDmaAdapter;
    PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;
    PFREE_COMMON_BUFFER FreeCommonBuffer;
    PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
    PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
    PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
    PFREE_MAP_REGISTERS FreeMapRegisters;
    PMAP_TRANSFER MapTransfer;
    PGET_DMA_ALIGNMENT GetDmaAlignment;
    PREAD_DMA_COUNTER ReadDmaCounter;
    PGET_SCATTER_GATHER_LIST GetScatterGatherList;
    PPUT_SCATTER_GATHER_LIST PutScatterGatherList;
} DMA_OPERATIONS;

// end_wdm


#if defined(_AXP64_)

//
// Use __inline DMA macros (hal.h)
//
#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

//
// Only PnP drivers!
//
#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif // _AXP64_


#if defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_))

// begin_wdm

__inline
PVOID
HalAllocateCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    ){

    PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
    PVOID commonBuffer;

    allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
    ASSERT( allocateCommonBuffer != NULL );

    commonBuffer = allocateCommonBuffer( DmaAdapter,
                                         Length,
                                         LogicalAddress,
                                         CacheEnabled );

    return commonBuffer;
}

__inline
VOID
HalFreeCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    ){

    PFREE_COMMON_BUFFER freeCommonBuffer;

    freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
    ASSERT( freeCommonBuffer != NULL );

    freeCommonBuffer( DmaAdapter,
                      Length,
                      LogicalAddress,
                      VirtualAddress,
                      CacheEnabled );
}

__inline
NTSTATUS
IoAllocateAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    ){

    PALLOCATE_ADAPTER_CHANNEL allocateAdapterChannel;
    NTSTATUS status;

    allocateAdapterChannel =
        *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;

    ASSERT( allocateAdapterChannel != NULL );

    status = allocateAdapterChannel( DmaAdapter,
                                     DeviceObject,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine,
                                     Context );

    return status;
}

__inline
BOOLEAN
IoFlushAdapterBuffers(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers;
    BOOLEAN result;

    flushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
    ASSERT( flushAdapterBuffers != NULL );

    result = flushAdapterBuffers( DmaAdapter,
                                  Mdl,
                                  MapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice );
    return result;
}

__inline
VOID
IoFreeAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter
    ){

    PFREE_ADAPTER_CHANNEL freeAdapterChannel;

    freeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
    ASSERT( freeAdapterChannel != NULL );

    freeAdapterChannel( DmaAdapter );
}

__inline
VOID
IoFreeMapRegisters(
    IN PDMA_ADAPTER DmaAdapter,
    IN PVOID MapRegisterBase,
    IN ULONG NumberOfMapRegisters
    ){

    PFREE_MAP_REGISTERS freeMapRegisters;

    freeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
    ASSERT( freeMapRegisters != NULL );

    freeMapRegisters( DmaAdapter,
                      MapRegisterBase,
                      NumberOfMapRegisters );
}


__inline
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PHYSICAL_ADDRESS physicalAddress;
    PMAP_TRANSFER mapTransfer;

    mapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
    ASSERT( mapTransfer != NULL );

    physicalAddress = mapTransfer( DmaAdapter,
                                   Mdl,
                                   MapRegisterBase,
                                   CurrentVa,
                                   Length,
                                   WriteToDevice );

    return physicalAddress;
}

__inline
ULONG
HalGetDmaAlignment(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PGET_DMA_ALIGNMENT getDmaAlignment;
    ULONG alignment;

    getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
    ASSERT( getDmaAlignment != NULL );

    alignment = getDmaAlignment( DmaAdapter );
    return alignment;
}

__inline
ULONG
HalReadDmaCounter(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PREAD_DMA_COUNTER readDmaCounter;
    ULONG counter;

    readDmaCounter = *(DmaAdapter)->DmaOperations->ReadDmaCounter;
    ASSERT( readDmaCounter != NULL );

    counter = readDmaCounter( DmaAdapter );
    return counter;
}

// end_wdm

#else

//
// DMA adapter object functions.
//
NTHALAPI
NTSTATUS
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );

NTHALAPI
PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

NTHALAPI
VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

NTHALAPI
ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    );

NTHALAPI
BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

NTHALAPI
VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    );

NTHALAPI
VOID
IoFreeMapRegisters(
   IN PADAPTER_OBJECT AdapterObject,
   IN PVOID MapRegisterBase,
   IN ULONG NumberOfMapRegisters
   );

NTHALAPI
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );
#endif // USE_DMA_MACROS && (_NTDDK_ || _NTDRIVER_)

NTSTATUS
HalGetScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutDmaAdapter(
    IN PADAPTER_OBJECT DmaAdapter
    );

// end_ntddk

#endif // _HAL_

