/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/srb.c
 * PURPOSE:         SCSI port driver definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_SRB_H
#define __STORAGE_INCLUDE_SRB_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

#include "ntddk.h"

#if defined(_SCSIPORT_)
  #define SCSIPORTAPI 
#else
  #define SCSIPORTAPI 
#endif

#define DDKAPI STDCALL

/* Obsolete. For backward compatibility only. */

typedef PHYSICAL_ADDRESS SCSI_PHYSICAL_ADDRESS, *PSCSI_PHYSICAL_ADDRESS;

#define SP_UNINITIALIZED_VALUE ((ULONG) ~0)
#define SP_UNTAGGED            ((ULONG) ~0)

#define SRB_SIMPLE_TAG_REQUEST              0x20
#define SRB_HEAD_OF_QUEUE_TAG_REQUEST       0x21
#define SRB_ORDERED_QUEUE_TAG_REQUEST       0x22

/* SRB Status Masks */

#define SRB_STATUS_QUEUE_FROZEN             0x40
#define SRB_STATUS_AUTOSENSE_VALID          0x80

#define SRB_STATUS(Status) \
  (Status & ~(SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN))

#define MAXIMUM_CDB_SIZE 12

/* Define SCSI maximum configuration parameters. */

#define SCSI_MAXIMUM_LOGICAL_UNITS 8
#define SCSI_MAXIMUM_TARGETS_PER_BUS 32
#define SCSI_MAXIMUM_BUSES 8
#define SCSI_MINIMUM_PHYSICAL_BREAKS  16
#define SCSI_MAXIMUM_PHYSICAL_BREAKS 255
#define SCSI_MAXIMUM_TARGETS 8



/* Port driver error codes */

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

/* SCSI_REQUEST_BLOCK.Function constants */
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

/* SCSI_REQUEST_BLOCK.SrbStatus constants */
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

/* SCSI_REQUEST_BLOCK.SrbFlags constants */
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
#define SRB_FLAGS_IS_ACTIVE                 0x00010000
#define SRB_FLAGS_ALLOCATED_FROM_ZONE       0x00020000
#define SRB_FLAGS_SGLIST_FROM_POOL          0x00040000

typedef struct _SCSI_REQUEST_BLOCK {
  USHORT Length;			/* 0x00 */
  UCHAR Function;			/* 0x02 */
  UCHAR SrbStatus;			/* 0x03 */
  UCHAR ScsiStatus;			/* 0x04 */
  UCHAR PathId;				/* 0x05 */
  UCHAR TargetId;			/* 0x06 */
  UCHAR Lun;				/* 0x07 */
  UCHAR QueueTag;			/* 0x08 */
  UCHAR QueueAction;			/* 0x09 */
  UCHAR CdbLength;			/* 0x0A */
  UCHAR SenseInfoBufferLength;		/* 0x0B */
  ULONG SrbFlags;			/* 0x0C */
  ULONG DataTransferLength;		/* 0x10 */
  ULONG TimeOutValue;			/* 0x14 */
  PVOID DataBuffer;			/* 0x18 */
  PVOID SenseInfoBuffer;		/* 0x1C */
  struct _SCSI_REQUEST_BLOCK *NextSrb;	/* 0x20 */
  PVOID OriginalRequest;		/* 0x24 */
  PVOID SrbExtension;			/* 0x28 */
  ULONG QueueSortKey;			/* 0x2C */
  UCHAR Cdb[16];			/* 0x30 */
} SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

#define SCSI_REQUEST_BLOCK_SIZE sizeof(SCSI_REQUEST_BLOCK)

typedef struct _ACCESS_RANGE {
  SCSI_PHYSICAL_ADDRESS RangeStart;
  ULONG RangeLength;
  BOOLEAN RangeInMemory;
}ACCESS_RANGE, *PACCESS_RANGE;


typedef struct _PORT_CONFIGURATION_INFORMATION {
  ULONG Length;
  ULONG SystemIoBusNumber;
  INTERFACE_TYPE  AdapterInterfaceType;
  ULONG BusInterruptLevel;
  ULONG BusInterruptVector;
  KINTERRUPT_MODE InterruptMode;
  ULONG MaximumTransferLength;
  ULONG NumberOfPhysicalBreaks;
  ULONG DmaChannel;
  ULONG DmaPort;
  DMA_WIDTH DmaWidth;
  DMA_SPEED DmaSpeed;
  ULONG AlignmentMask;
  ULONG NumberOfAccessRanges;
#ifdef __GNUC__
  ACCESS_RANGE *AccessRanges;
#else
  ACCESS_RANGE (*AccessRanges)[];
#endif
  PVOID Reserved;
  UCHAR NumberOfBuses;
  CCHAR InitiatorBusId[8];
  BOOLEAN ScatterGather;
  BOOLEAN Master;
  BOOLEAN CachesData;
  BOOLEAN AdapterScansDown;
  BOOLEAN AtdiskPrimaryClaimed;
  BOOLEAN AtdiskSecondaryClaimed;
  BOOLEAN Dma32BitAddresses;
  BOOLEAN DemandMode;
  BOOLEAN MapBuffers;
  BOOLEAN NeedPhysicalAddresses;
  BOOLEAN TaggedQueuing;
  BOOLEAN AutoRequestSense;
  BOOLEAN MultipleRequestPerLu;
  BOOLEAN ReceiveEvent;
  BOOLEAN RealModeInitialized;
  BOOLEAN BufferAccessScsiPortControlled;
  UCHAR MaximumNumberOfTargets;
  UCHAR ReservedUchars[2];
  ULONG SlotNumber;
  ULONG BusInterruptLevel2;
  ULONG BusInterruptVector2;
  KINTERRUPT_MODE InterruptMode2;
  ULONG DmaChannel2;
  ULONG DmaPort2;
  DMA_WIDTH DmaWidth2;
  DMA_SPEED DmaSpeed2;
  ULONG DeviceExtensionSize;
  ULONG SpecificLuExtensionSize;
  ULONG SrbExtensionSize;
} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

#define CONFIG_INFO_VERSION_2 sizeof(PORT_CONFIGURATION_INFORMATION)

typedef enum _SCSI_NOTIFICATION_TYPE {
  RequestComplete,
  NextRequest,
  NextLuRequest,
  ResetDetected,
  CallDisableInterrupts,
  CallEnableInterrupts,
  RequestTimerCall
} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;


typedef BOOLEAN DDKAPI
(*PHW_ADAPTER_STATE)(
  IN PVOID DeviceExtension,
  IN PVOID Context,
  IN BOOLEAN SaveState);

/* Return values for SCSI_HW_FIND_ADAPTER. */

#define SP_RETURN_NOT_FOUND     0
#define SP_RETURN_FOUND         1
#define SP_RETURN_ERROR         2
#define SP_RETURN_BAD_CONFIG    3

typedef ULONG DDKAPI
(*PHW_FIND_ADAPTER)(
    IN PVOID DeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again);

typedef BOOLEAN DDKAPI
(*PHW_INITIALIZE)(
  IN PVOID DeviceExtension);

typedef BOOLEAN DDKAPI
(*PHW_INTERRUPT)(
  IN PVOID DeviceExtension);

typedef BOOLEAN DDKAPI
(*PHW_RESET_BUS)(
  IN PVOID DeviceExtension,
  IN ULONG PathId);

typedef VOID DDKAPI
(*PHW_DMA_STARTED)(
  IN PVOID DeviceExtension);

typedef BOOLEAN DDKAPI
(*PHW_STARTIO)(
  IN PVOID DeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb);

typedef VOID DDKAPI
(*PHW_TIMER)(
  IN PVOID DeviceExtension);

typedef struct _HW_INITIALIZATION_DATA {
  ULONG HwInitializationDataSize;
  INTERFACE_TYPE AdapterInterfaceType;
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
  BOOLEAN MapBuffers;
  BOOLEAN NeedPhysicalAddresses;
//  BOOLEAN TaggedQueuing;
  BOOLEAN  TaggedQueueing; 
  BOOLEAN AutoRequestSense;
  BOOLEAN MultipleRequestPerLu;
  BOOLEAN ReceiveEvent;
  USHORT VendorIdLength;
  PVOID VendorId;
  USHORT ReservedUshort;
  USHORT DeviceIdLength;
  PVOID DeviceId;
} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;


/* FUNCTIONS ****************************************************************/

VOID
DDKAPI
ScsiPortCompleteRequest(
  IN PVOID HwDeviceExtension,
  IN UCHAR PathId,
  IN UCHAR TargetId,
  IN UCHAR Lun,
  IN UCHAR SrbStatus);

ULONG
DDKAPI
ScsiPortConvertPhysicalAddressToUlong(
  IN SCSI_PHYSICAL_ADDRESS Address);

SCSIPORTAPI
SCSI_PHYSICAL_ADDRESS
DDKAPI
ScsiPortConvertUlongToPhysicalAddress(
  IN ULONG UlongAddress);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortFlushDma(
  IN PVOID HwDeviceExtension);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortFreeDeviceBase(
  IN PVOID HwDeviceExtension,
  IN PVOID MappedAddress);

SCSIPORTAPI
ULONG
DDKAPI
ScsiPortGetBusData(
  IN PVOID DeviceExtension,
  IN ULONG BusDataType,
  IN ULONG SystemIoBusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Length);

SCSIPORTAPI
PVOID
DDKAPI
ScsiPortGetDeviceBase(
  IN PVOID HwDeviceExtension,
  IN INTERFACE_TYPE BusType,
  IN ULONG SystemIoBusNumber,
  IN SCSI_PHYSICAL_ADDRESS IoAddress,
  IN ULONG NumberOfBytes,
  IN BOOLEAN InIoSpace);

SCSIPORTAPI
PVOID
DDKAPI
ScsiPortGetLogicalUnit(
  IN PVOID HwDeviceExtension,
  IN UCHAR PathId,
  IN UCHAR TargetId,
  IN UCHAR Lun);

SCSIPORTAPI
SCSI_PHYSICAL_ADDRESS
DDKAPI
ScsiPortGetPhysicalAddress(
  IN PVOID HwDeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
  IN PVOID VirtualAddress,
  OUT PULONG Length);

SCSIPORTAPI
PSCSI_REQUEST_BLOCK
DDKAPI
ScsiPortGetSrb(
  IN PVOID DeviceExtension,
  IN UCHAR PathId,
  IN UCHAR TargetId,
  IN UCHAR Lun,
  IN LONG QueueTag);

SCSIPORTAPI
PVOID
DDKAPI
ScsiPortGetUncachedExtension(
  IN PVOID HwDeviceExtension,
  IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
  IN ULONG NumberOfBytes);

SCSIPORTAPI
PVOID
DDKAPI
ScsiPortGetVirtualAddress(
  IN PVOID HwDeviceExtension,
  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress);

SCSIPORTAPI
ULONG
DDKAPI
ScsiPortInitialize(
  IN PVOID Argument1,
  IN PVOID Argument2,
  IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
  IN PVOID HwContext);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortIoMapTransfer(
  IN PVOID HwDeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb,
  IN ULONG LogicalAddress,
  IN ULONG Length);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortLogError(
  IN PVOID HwDeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
  IN UCHAR PathId,
  IN UCHAR TargetId,
  IN UCHAR Lun,
  IN ULONG ErrorCode,
  IN ULONG UniqueId);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortMoveMemory(
  OUT PVOID Destination,
  IN PVOID Source,
  IN ULONG Length);

SCSIPORTAPI
VOID
ScsiPortNotification(
  IN SCSI_NOTIFICATION_TYPE NotificationType,
  IN PVOID HwDeviceExtension,
  ...);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadPortBufferUchar(
  IN PUCHAR Port,
  IN PUCHAR Value,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadPortBufferUlong(
  IN PULONG Port,
  IN PULONG Value,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadPortBufferUshort(
  IN PUSHORT Port,
  IN PUSHORT Value,
  IN ULONG Count);

SCSIPORTAPI
UCHAR
DDKAPI
ScsiPortReadPortUchar(
  IN PUCHAR Port);

SCSIPORTAPI
ULONG
DDKAPI
ScsiPortReadPortUlong(
  IN PULONG Port);

SCSIPORTAPI
USHORT
DDKAPI
ScsiPortReadPortUshort(
  IN PUSHORT Port);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadRegisterBufferUchar(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadRegisterBufferUlong(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortReadRegisterBufferUshort(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count);

SCSIPORTAPI
UCHAR
DDKAPI
ScsiPortReadRegisterUchar(
  IN PUCHAR Register);

SCSIPORTAPI
ULONG
DDKAPI
ScsiPortReadRegisterUlong(
  IN PULONG Register);

SCSIPORTAPI
USHORT
DDKAPI
ScsiPortReadRegisterUshort(
  IN PUSHORT Register);

SCSIPORTAPI
ULONG
DDKAPI
ScsiPortSetBusDataByOffset(
  IN PVOID DeviceExtension,
  IN ULONG BusDataType,
  IN ULONG SystemIoBusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortStallExecution(
  IN ULONG MicroSeconds);

SCSIPORTAPI
BOOLEAN
DDKAPI
ScsiPortValidateRange(
  IN PVOID HwDeviceExtension,
  IN INTERFACE_TYPE BusType,
  IN ULONG SystemIoBusNumber,
  IN SCSI_PHYSICAL_ADDRESS IoAddress,
  IN ULONG NumberOfBytes,
  IN BOOLEAN InIoSpace);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortBufferUchar(
  IN PUCHAR Port,
  IN PUCHAR Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortBufferUlong(
  IN PULONG Port,
  IN PULONG Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortBufferUshort(
  IN PUSHORT Port,
  IN PUSHORT Value,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortUchar(
  IN PUCHAR Port,
  IN UCHAR Value);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortUlong(
  IN PULONG Port,
  IN ULONG Value);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWritePortUshort(
  IN PUSHORT Port,
			IN USHORT Value);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterBufferUchar(
  IN PUCHAR Register,
  IN PUCHAR Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterBufferUlong(
  IN PULONG Register,
  IN PULONG Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterBufferUshort(
  IN PUSHORT Register,
  IN PUSHORT Buffer,
  IN ULONG Count);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterUchar(
  IN PUCHAR Register,
  IN ULONG Value);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterUlong(
  IN PULONG Register,
  IN ULONG Value);

SCSIPORTAPI
VOID
DDKAPI
ScsiPortWriteRegisterUshort(
  IN PUSHORT Register,
  IN USHORT Value);

SCSIPORTAPI
VOID
ScsiDebugPrint(
  IN ULONG DebugPrintLevel,
  IN PCHAR DebugMessage,
  IN ...);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_INCLUDE_SRB_H */
