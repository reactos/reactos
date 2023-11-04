/*
	Module Name:

    srb.h

	Abstract:

    This file defines the interface between SCSI mini-port drivers and the
    SCSI port driver.  It is also used by SCSI class drivers to talk to the
    SCSI port driver.
    w2k-related definitions are added by Alter from w2k/xp DDK
 */

#ifndef _NTSRB_
#define _NTSRB_

#pragma pack(push, 8)

// Define SCSI maximum configuration parameters.

#define SCSI_MAXIMUM_LOGICAL_UNITS 8
#define SCSI_MAXIMUM_TARGETS_PER_BUS 128
#define SCSI_MAXIMUM_LUNS_PER_TARGET 255
#define SCSI_MAXIMUM_BUSES 8
#define SCSI_MINIMUM_PHYSICAL_BREAKS  16
#define SCSI_MAXIMUM_PHYSICAL_BREAKS 255

// This constant is for backward compatibility.
// This use to be the maximum number of targets supported.

#define SCSI_MAXIMUM_TARGETS 8
// begin_ntminitape
#define MAXIMUM_CDB_SIZE 12
// end_ntminitape

#ifndef USER_MODE

typedef PHYSICAL_ADDRESS SCSI_PHYSICAL_ADDRESS, *PSCSI_PHYSICAL_ADDRESS;

typedef struct _ACCESS_RANGE {
    SCSI_PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    BOOLEAN RangeInMemory;
}ACCESS_RANGE, *PACCESS_RANGE;

#ifdef __REACTOS__
#pragma pack(push, 4)
#endif

//
// Configuration information structure.  Contains the information necessary
// to initialize the adapter. NOTE: This structure's must be a multiple of
// quadwords.
//

typedef struct _PORT_CONFIGURATION_INFORMATION {
    ULONG Length;				// Length of port configuation information strucuture.
    ULONG SystemIoBusNumber;	// IO bus number (0 for machines that have only 1 IO bus
    INTERFACE_TYPE  AdapterInterfaceType;	// EISA, MCA or ISA
    ULONG BusInterruptLevel;	// Interrupt request level for device
    // Bus interrupt vector used with hardware buses which use as vector as
    // well as level, such as internal buses.
    ULONG BusInterruptVector;
    KINTERRUPT_MODE InterruptMode;	// Interrupt mode (level-sensitive or edge-triggered)

    ULONG MaximumTransferLength;	// Max bytes that can be transferred in a single SRB
    ULONG NumberOfPhysicalBreaks;	// Number of contiguous blocks of physical memory
    ULONG DmaChannel;				// DMA channel for devices using system DMA
    ULONG DmaPort;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG AlignmentMask;			// Alignment masked for the adapter for data transfers.
    ULONG NumberOfAccessRanges;		// Number of allocated access range elements.
    ACCESS_RANGE (*AccessRanges)[];	// Pointer to array of access range elements.
    PVOID Reserved;
    UCHAR NumberOfBuses;			// Number of SCSI buses attached to the adapter.
    CCHAR InitiatorBusId[8];		// SCSI bus ID for adapter
    BOOLEAN ScatterGather;			// Indicates that the adapter does scatter/gather
    BOOLEAN Master;					// Indicates that the adapter is a bus master
    BOOLEAN CachesData;				// Host caches data or state.
    BOOLEAN AdapterScansDown;		// Host adapter scans down for bios devices.
    BOOLEAN AtdiskPrimaryClaimed;	// Primary at disk address (0x1F0) claimed.
    BOOLEAN AtdiskSecondaryClaimed;	// Secondary at disk address (0x170) claimed.
    BOOLEAN Dma32BitAddresses;		// The master uses 32-bit DMA addresses.
    BOOLEAN DemandMode;				// Use Demand Mode DMA rather than Single Request.
    BOOLEAN MapBuffers;				// Data buffers must be mapped into virtual address space.
    BOOLEAN NeedPhysicalAddresses;	// We need to tranlate virtual to physical addresses.
    BOOLEAN TaggedQueuing;			// Supports tagged queuing
    BOOLEAN AutoRequestSense;		// Supports auto request sense.
    BOOLEAN MultipleRequestPerLu;	// Supports multiple requests per logical unit.
    BOOLEAN ReceiveEvent;			// Support receive event function.
    BOOLEAN RealModeInitialized;	// Indicates the real-mode driver has initialized the card.

    BOOLEAN BufferAccessScsiPortControlled; // Indicate that the miniport will not touch
									// the data buffers directly.
    UCHAR   MaximumNumberOfTargets;	// Indicator for wide scsi.
    UCHAR   ReservedUchars[2];		// Ensure quadword alignment.
    ULONG SlotNumber;				// Adapter slot number
    ULONG BusInterruptLevel2;		// Interrupt information for a second IRQ.
    ULONG BusInterruptVector2;
    KINTERRUPT_MODE InterruptMode2;
    ULONG DmaChannel2;				// DMA information for a second channel.
    ULONG DmaPort2;
    DMA_WIDTH DmaWidth2;
    DMA_SPEED DmaSpeed2;

} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

typedef struct _PORT_CONFIGURATION_INFORMATION_NT {
    // Fields added to allow for the miniport
    // to update these sizes based on requirements
    // for large transfers ( > 64K);
    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;
} PORT_CONFIGURATION_INFORMATION_NT, *PPORT_CONFIGURATION_INFORMATION_NT;

typedef struct _PORT_CONFIGURATION_INFORMATION_2K {
    // Used to determine whether the system and/or the miniport support
    // 64-bit physical addresses.  See SCSI_DMA64_* flags below.
    UCHAR  Dma64BitAddresses;
    // Indicates that the miniport can accept a SRB_FUNCTION_RESET_DEVICE
    // to clear all requests to a particular LUN.
    BOOLEAN ResetTargetSupported;
    // Indicates that the miniport can support more than 8 logical units per
    // target (maximum LUN number is one less than this field).
    UCHAR MaximumNumberOfLogicalUnits;
    // Supports WMI?
    BOOLEAN WmiDataProvider;
} PORT_CONFIGURATION_INFORMATION_2K, *PPORT_CONFIGURATION_INFORMATION_2K;

#ifdef __REACTOS__
#pragma pack(pop)
#endif

typedef struct _PORT_CONFIGURATION_INFORMATION_COMMON {
    PORT_CONFIGURATION_INFORMATION     comm;
    PORT_CONFIGURATION_INFORMATION_NT  nt4;
    PORT_CONFIGURATION_INFORMATION_2K  w2k;
} PORT_CONFIGURATION_INFORMATION_COMMON, *PPORT_CONFIGURATION_INFORMATION_COMMON;

//
// Version control for ConfigInfo structure.
//

#define CONFIG_INFO_VERSION_2 sizeof(PORT_CONFIGURATION_INFORMATION)

//
// Flags for controlling 64-bit DMA use (PORT_CONFIGURATION_INFORMATION field
// Dma64BitAddresses)
//

//
// Set by scsiport on entering HwFindAdapter if the system can support 64-bit
// physical addresses.  The miniport can use this information before calling
// ScsiPortGetUncachedExtension to modify the DeviceExtensionSize,
// SpecificLuExtensionSize & SrbExtensionSize fields to account for the extra
// size of the scatter gather list.
//

#define SCSI_DMA64_SYSTEM_SUPPORTED     0x80

//
// Set by the miniport before calling ScsiPortGetUncachedExtension to indicate
// that scsiport should provide it with 64-bit physical addresses.  If the
// system does not support 64-bit PA's then this bit will be ignored.
//

#define SCSI_DMA64_MINIPORT_SUPPORTED   0x01


//
// Command type (and parameter) definition(s) for AdapterControl requests.
//

typedef enum _SCSI_ADAPTER_CONTROL_TYPE {
    ScsiQuerySupportedControlTypes = 0,
    ScsiStopAdapter,
    ScsiRestartAdapter,
    ScsiSetBootConfig,
    ScsiSetRunningConfig,
    ScsiAdapterControlMax,
    MakeAdapterControlTypeSizeOfUlong = 0xffffffff
} SCSI_ADAPTER_CONTROL_TYPE, *PSCSI_ADAPTER_CONTROL_TYPE;

//
// Adapter control status values
//

typedef enum _SCSI_ADAPTER_CONTROL_STATUS {
    ScsiAdapterControlSuccess = 0,
    ScsiAdapterControlUnsuccessful
} SCSI_ADAPTER_CONTROL_STATUS, *PSCSI_ADAPTER_CONTROL_STATUS;

//
// Parameters for Adapter Control Functions:
//

//
// ScsiQuerySupportedControlTypes:
//

#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif
typedef struct _SCSI_SUPPORTED_CONTROL_TYPE_LIST {

    //
    // Specifies the number of entries in the adapter control type list.
    //

    IN ULONG MaxControlType;

    //
    // The miniport will set TRUE for each control type it supports.
    // The number of entries in this array is defined by MaxAdapterControlType
    // - the miniport must not attempt to set any AC types beyond the maximum
    // value specified.
    //

    OUT BOOLEAN SupportedTypeList[0];

} SCSI_SUPPORTED_CONTROL_TYPE_LIST, *PSCSI_SUPPORTED_CONTROL_TYPE_LIST;
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

//
// Uninitialized flag value.
//

#define SP_UNINITIALIZED_VALUE ((ULONG) ~0)
#define SP_UNTAGGED ((UCHAR) ~0)

//
// Set asynchronous events.
//

#define SRBEV_BUS_RESET               0x0001
#define SRBEV_SCSI_ASYNC_NOTIFICATION 0x0002

// begin_ntminitape

//
// SCSI I/O Request Block
//

typedef struct _SCSI_REQUEST_BLOCK {
    USHORT Length;                  // offset 0
    UCHAR Function;                 // offset 2
    UCHAR SrbStatus;                // offset 3
    UCHAR ScsiStatus;               // offset 4
    UCHAR PathId;                   // offset 5
    UCHAR TargetId;                 // offset 6
    UCHAR Lun;                      // offset 7
    UCHAR QueueTag;                 // offset 8
    UCHAR QueueAction;              // offset 9
    UCHAR CdbLength;                // offset a
    UCHAR SenseInfoBufferLength;    // offset b
    ULONG SrbFlags;                 // offset c
    ULONG DataTransferLength;       // offset 10
    ULONG TimeOutValue;             // offset 14
    PVOID DataBuffer;               // offset 18
    PVOID SenseInfoBuffer;          // offset 1c
    struct _SCSI_REQUEST_BLOCK *NextSrb; // offset 20
    PVOID OriginalRequest;          // offset 24
    PVOID SrbExtension;             // offset 28
    union {
        ULONG InternalStatus;       // offset 2c
        ULONG QueueSortKey;         // offset 2c
    };

#if defined(_WIN64)
    // Force PVOID alignment of Cdb
    ULONG Reserved;

#endif

    UCHAR Cdb[16];                  // offset 30
} SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

#define SCSI_REQUEST_BLOCK_SIZE sizeof(SCSI_REQUEST_BLOCK)

//
// SCSI I/O Request Block for WMI Requests
//

typedef struct _SCSI_WMI_REQUEST_BLOCK {
    USHORT Length;
    UCHAR Function;        // SRB_FUNCTION_WMI
    UCHAR SrbStatus;
    UCHAR WMISubFunction;
    UCHAR PathId;          // If SRB_WMI_FLAGS_ADAPTER_REQUEST is set in
    UCHAR TargetId;        // WMIFlags then PathId, TargetId and Lun are
    UCHAR Lun;             // reserved fields.
    UCHAR Reserved1;
    UCHAR WMIFlags;
    UCHAR Reserved2[2];
    ULONG SrbFlags;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    PVOID DataPath;
    PVOID Reserved3;
    PVOID OriginalRequest;
    PVOID SrbExtension;
    ULONG Reserved4;
    UCHAR Reserved5[16];
} SCSI_WMI_REQUEST_BLOCK, *PSCSI_WMI_REQUEST_BLOCK;

//
// SRB Functions
//

#define SRB_FUNCTION_EXECUTE_SCSI           0x00
#define SRB_FUNCTION_CLAIM_DEVICE           0x01
#define SRB_FUNCTION_IO_CONTROL             0x02
#define SRB_FUNCTION_RECEIVE_EVENT          0x03
#define SRB_FUNCTION_RELEASE_QUEUE          0x04
#define SRB_FUNCTION_ATTACH_DEVICE          0x05
#define SRB_FUNCTION_RELEASE_DEVICE         0x06
#define SRB_FUNCTION_SHUTDOWN               0x07
#define SRB_FUNCTION_FLUSH                  0x08
#define SRB_FUNCTION_ABORT_COMMAND          0x10
#define SRB_FUNCTION_RELEASE_RECOVERY       0x11
#define SRB_FUNCTION_RESET_BUS              0x12
#define SRB_FUNCTION_RESET_DEVICE           0x13
#define SRB_FUNCTION_TERMINATE_IO           0x14
#define SRB_FUNCTION_FLUSH_QUEUE            0x15
#define SRB_FUNCTION_REMOVE_DEVICE          0x16
#define SRB_FUNCTION_WMI                    0x17
#define SRB_FUNCTION_LOCK_QUEUE             0x18
#define SRB_FUNCTION_UNLOCK_QUEUE           0x19
#define SRB_FUNCTION_RESET_LOGICAL_UNIT     0x20

//
// SRB Status
//

#define SRB_STATUS_PENDING                  0x00
#define SRB_STATUS_SUCCESS                  0x01
#define SRB_STATUS_ABORTED                  0x02
#define SRB_STATUS_ABORT_FAILED             0x03
#define SRB_STATUS_ERROR                    0x04
#define SRB_STATUS_BUSY                     0x05
#define SRB_STATUS_INVALID_REQUEST          0x06
#define SRB_STATUS_INVALID_PATH_ID          0x07
#define SRB_STATUS_NO_DEVICE                0x08
#define SRB_STATUS_TIMEOUT                  0x09
#define SRB_STATUS_SELECTION_TIMEOUT        0x0A
#define SRB_STATUS_COMMAND_TIMEOUT          0x0B
#define SRB_STATUS_MESSAGE_REJECTED         0x0D
#define SRB_STATUS_BUS_RESET                0x0E
#define SRB_STATUS_PARITY_ERROR             0x0F
#define SRB_STATUS_REQUEST_SENSE_FAILED     0x10
#define SRB_STATUS_NO_HBA                   0x11
#define SRB_STATUS_DATA_OVERRUN             0x12
#define SRB_STATUS_UNEXPECTED_BUS_FREE      0x13
#define SRB_STATUS_PHASE_SEQUENCE_FAILURE   0x14
#define SRB_STATUS_BAD_SRB_BLOCK_LENGTH     0x15
#define SRB_STATUS_REQUEST_FLUSHED          0x16
#define SRB_STATUS_INVALID_LUN              0x20
#define SRB_STATUS_INVALID_TARGET_ID        0x21
#define SRB_STATUS_BAD_FUNCTION             0x22
#define SRB_STATUS_ERROR_RECOVERY           0x23
#define SRB_STATUS_NOT_POWERED              0x24

//
// This value is used by the port driver to indicate that a non-scsi-related
// error occured.  Miniports must never return this status.
//

#define SRB_STATUS_INTERNAL_ERROR           0x30

//
// Srb status values 0x38 through 0x3f are reserved for internal port driver
// use.
//



//
// SRB Status Masks
//

#define SRB_STATUS_QUEUE_FROZEN             0x40
#define SRB_STATUS_AUTOSENSE_VALID          0x80

#define SRB_STATUS(Status) (Status & ~(SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN))

//
// SRB Flag Bits
//

#define SRB_FLAGS_QUEUE_ACTION_ENABLE       0x00000002
#define SRB_FLAGS_DISABLE_DISCONNECT        0x00000004
#define SRB_FLAGS_DISABLE_SYNCH_TRANSFER    0x00000008
#define SRB_FLAGS_BYPASS_FROZEN_QUEUE       0x00000010
#define SRB_FLAGS_DISABLE_AUTOSENSE         0x00000020
#define SRB_FLAGS_DATA_IN                   0x00000040
#define SRB_FLAGS_DATA_OUT                  0x00000080
#define SRB_FLAGS_NO_DATA_TRANSFER          0x00000000
#define SRB_FLAGS_UNSPECIFIED_DIRECTION      (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)
#define SRB_FLAGS_NO_QUEUE_FREEZE           0x00000100
#define SRB_FLAGS_ADAPTER_CACHE_ENABLE      0x00000200
#define SRB_FLAGS_FREE_SENSE_BUFFER         0x00000400
#define SRB_FLAGS_IS_ACTIVE                 0x00010000
#define SRB_FLAGS_ALLOCATED_FROM_ZONE       0x00020000
#define SRB_FLAGS_SGLIST_FROM_POOL          0x00040000
#define SRB_FLAGS_BYPASS_LOCKED_QUEUE       0x00080000

#define SRB_FLAGS_NO_KEEP_AWAKE             0x00100000
#define SRB_FLAGS_PORT_DRIVER_ALLOCSENSE    0x00200000
#define SRB_FLAGS_PORT_DRIVER_SENSEHASPORT  0x00400000
#define SRB_FLAGS_DONT_START_NEXT_PACKET    0x00800000

#define SRB_FLAGS_PORT_DRIVER_RESERVED      0x0F000000
#define SRB_FLAGS_CLASS_DRIVER_RESERVED     0xF0000000
//
// Queue Action
//

#define SRB_SIMPLE_TAG_REQUEST              0x20
#define SRB_HEAD_OF_QUEUE_TAG_REQUEST       0x21
#define SRB_ORDERED_QUEUE_TAG_REQUEST       0x22

#define SRB_WMI_FLAGS_ADAPTER_REQUEST       0x01

// end_ntminitape

//
// SCSI Adapter Dependent Routines
//

typedef
BOOLEAN
(NTAPI *PHW_INITIALIZE) (
    IN PVOID DeviceExtension
    );

typedef
BOOLEAN
(NTAPI *PHW_STARTIO) (
    IN PVOID DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

typedef
BOOLEAN
(NTAPI *PHW_INTERRUPT) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(NTAPI *PHW_TIMER) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(NTAPI *PHW_DMA_STARTED) (
    IN PVOID DeviceExtension
    );

typedef
ULONG
(NTAPI *PHW_FIND_ADAPTER) (
    IN PVOID DeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

typedef
BOOLEAN
(NTAPI *PHW_RESET_BUS) (
    IN PVOID DeviceExtension,
    IN ULONG PathId
    );

typedef
BOOLEAN
(NTAPI *PHW_ADAPTER_STATE) (
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState
    );

typedef
SCSI_ADAPTER_CONTROL_STATUS
(NTAPI *PHW_ADAPTER_CONTROL) (
    IN PVOID DeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

//
// Port driver error codes
//

#define SP_BUS_PARITY_ERROR         0x0001
#define SP_UNEXPECTED_DISCONNECT    0x0002
#define SP_INVALID_RESELECTION      0x0003
#define SP_BUS_TIME_OUT             0x0004
#define SP_PROTOCOL_ERROR           0x0005
#define SP_INTERNAL_ADAPTER_ERROR   0x0006
#define SP_REQUEST_TIMEOUT          0x0007
#define SP_IRQ_NOT_RESPONDING       0x0008
#define SP_BAD_FW_WARNING           0x0009
#define SP_BAD_FW_ERROR             0x000a
#define SP_LOST_WMI_MINIPORT_REQUEST 0x000b


//
// Return values for SCSI_HW_FIND_ADAPTER.
//

#define SP_RETURN_NOT_FOUND     0
#define SP_RETURN_FOUND         1
#define SP_RETURN_ERROR         2
#define SP_RETURN_BAD_CONFIG    3

//
// Notification Event Types
//

typedef enum _SCSI_NOTIFICATION_TYPE {
    RequestComplete,
    NextRequest,
    NextLuRequest,
    ResetDetected,
    CallDisableInterrupts,
    CallEnableInterrupts,
    RequestTimerCall,
    BusChangeDetected,     /* New */
    WMIEvent,
    WMIReregister
} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;

//
// Structure passed between miniport initialization
// and SCSI port initialization
//

typedef struct _HW_INITIALIZATION_DATA {

    ULONG HwInitializationDataSize;
    // Adapter interface type:
    //
    // Internal
    // Isa
    // Eisa
    // MicroChannel
    // TurboChannel
    // PCIBus
    // VMEBus
    // NuBus
    // PCMCIABus
    // CBus
    // MPIBus
    // MPSABus
    INTERFACE_TYPE  AdapterInterfaceType;
	// Miniport driver routines
    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_FIND_ADAPTER HwFindAdapter;
    PHW_RESET_BUS HwResetBus;
    PHW_DMA_STARTED HwDmaStarted;
    PHW_ADAPTER_STATE HwAdapterState;
    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;
    ULONG NumberOfAccessRanges;
    PVOID Reserved;

    BOOLEAN MapBuffers;				// Data buffers must be mapped into virtual address space.
    BOOLEAN NeedPhysicalAddresses;	// We need to tranlate virtual to physical addresses.
    BOOLEAN TaggedQueuing;			// Supports tagged queuing
    BOOLEAN AutoRequestSense;		// Supports auto request sense.
    BOOLEAN MultipleRequestPerLu;	// Supports multiple requests per logical unit.
    BOOLEAN ReceiveEvent;			// Support receive event function.
    USHORT VendorIdLength;			// Vendor identification length
    PVOID VendorId;					// Vendor identification
    USHORT ReservedUshort;			// Pad for alignment and future use.
    USHORT DeviceIdLength;			// Device identification length
    PVOID DeviceId;					// Device identification

} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;

typedef struct _HW_INITIALIZATION_DATA_2K {
    // Stop adapter routine.
    PHW_ADAPTER_CONTROL HwAdapterControl;

} HW_INITIALIZATION_DATA_2K, *PHW_INITIALIZATION_DATA_2K;

typedef struct _HW_INITIALIZATION_DATA_COMMON {
  HW_INITIALIZATION_DATA      comm;
  HW_INITIALIZATION_DATA_2K   w2k;
}HW_INITIALIZATION_DATA_COMMON, *PHW_INITIALIZATION_DATA_COMMON;

// begin_ntminitape

#ifndef _NTDDK_
#define SCSIPORT_API DECLSPEC_IMPORT
#else
#define SCSIPORT_API
#endif

// end_ntminitape

//
// Port driver routines called by miniport driver
//

SCSIPORT_API
ULONG NTAPI
ScsiPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
    IN PVOID HwContext
    );

SCSIPORT_API
VOID NTAPI
ScsiPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    );

SCSIPORT_API
ULONG NTAPI
ScsiPortGetBusData(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

SCSIPORT_API
ULONG NTAPI
ScsiPortSetBusDataByOffset(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

SCSIPORT_API
PVOID NTAPI
ScsiPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    );

SCSIPORT_API
PVOID NTAPI
ScsiPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

SCSIPORT_API
PSCSI_REQUEST_BLOCK NTAPI
ScsiPortGetSrb(
    IN PVOID DeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    );

SCSIPORT_API
SCSI_PHYSICAL_ADDRESS NTAPI
ScsiPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    );

SCSIPORT_API
PVOID NTAPI
ScsiPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN SCSI_PHYSICAL_ADDRESS PhysicalAddress
    );

SCSIPORT_API
PVOID NTAPI
ScsiPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    );

SCSIPORT_API
VOID NTAPI
ScsiPortFlushDma(
    IN PVOID DeviceExtension
    );

SCSIPORT_API
VOID NTAPI
ScsiPortIoMapTransfer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID LogicalAddress,
    IN ULONG Length
    );

SCSIPORT_API
VOID __cdecl
ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    );

SCSIPORT_API
VOID NTAPI
ScsiPortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    );

SCSIPORT_API
VOID NTAPI
ScsiPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    );

SCSIPORT_API
VOID NTAPI
ScsiPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    );

SCSIPORT_API
UCHAR NTAPI
ScsiPortReadPortUchar(
    IN PUCHAR Port
    );

SCSIPORT_API
USHORT NTAPI
ScsiPortReadPortUshort(
    IN PUSHORT Port
    );

SCSIPORT_API
ULONG NTAPI
ScsiPortReadPortUlong(
    IN PULONG Port
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

SCSIPORT_API
UCHAR NTAPI
ScsiPortReadRegisterUchar(
    IN PUCHAR Register
    );

SCSIPORT_API
USHORT NTAPI
ScsiPortReadRegisterUshort(
    IN PUSHORT Register
    );

SCSIPORT_API
ULONG NTAPI
ScsiPortReadRegisterUlong(
    IN PULONG Register
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortStallExecution(
    IN ULONG Delay
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

SCSIPORT_API
VOID NTAPI
ScsiPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

SCSIPORT_API
SCSI_PHYSICAL_ADDRESS NTAPI
ScsiPortConvertUlongToPhysicalAddress(
    ULONG UlongAddress
    );

SCSIPORT_API
ULONG NTAPI
ScsiPortConvertPhysicalAddressToUlong(
    SCSI_PHYSICAL_ADDRESS Address
    );

#define ScsiPortConvertPhysicalAddressToUlong(Address) ((Address).LowPart)

SCSIPORT_API
BOOLEAN NTAPI
ScsiPortValidateRange(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    );

// begin_ntminitape

SCSIPORT_API
VOID __cdecl
ScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

// end_ntminitape

#endif //USER_MODE

#pragma pack(pop)

#endif //
