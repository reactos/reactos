/* $Id: srb.h,v 1.1 2001/07/21 07:29:53 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/srb.c
 * PURPOSE:         SCSI port driver definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_SRB_H
#define __STORAGE_INCLUDE_SRB_H

typedef PHYSICAL_ADDRESS SCSI_PHYSICAL_ADDRESS, *PSCSI_PHYSICAL_ADDRESS;


typedef struct _ACCESS_RANGE
{
  SCSI_PHYSICAL_ADDRESS RangeStart;
  ULONG RangeLength;
  BOOLEAN RangeInMemory;
}ACCESS_RANGE, *PACCESS_RANGE;


typedef struct _PORT_CONFIGURATION_INFORMATION
{
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
  ACCESS_RANGE (*AccessRanges)[];
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


typedef struct _SCSI_REQUEST_BLOCK
{
  USHORT Length;			// 0x00
  UCHAR Function;			// 0x02
  UCHAR SrbStatus;			// 0x03
  UCHAR ScsiStatus;			// 0x04
  UCHAR PathId;				// 0x05
  UCHAR TargetId;			// 0x06
  UCHAR Lun;				// 0x07
  UCHAR QueueTag;			// 0x08
  UCHAR QueueAction;			// 0x09
  UCHAR CdbLength;			// 0x0A
  UCHAR SenseInfoBufferLength;		// 0x0B
  ULONG SrbFlags;			// 0x0C
  ULONG DataTransferLength;		// 0x10
  ULONG TimeOutValue;			// 0x14
  PVOID DataBuffer;			// 0x18
  PVOID SenseInfoBuffer;		// 0x1C
  struct _SCSI_REQUEST_BLOCK *NextSrb;	// 0x20
  PVOID OriginalRequest;		// 0x24
  PVOID SrbExtension;			// 0x28
  ULONG QueueSortKey;			// 0x2C
  UCHAR Cdb[16];			// 0x30
} SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

#define SCSI_REQUEST_BLOCK_SIZE sizeof(SCSI_REQUEST_BLOCK)


typedef enum _SCSI_NOTIFICATION_TYPE
{
  RequestComplete,
  NextRequest,
  NextLuRequest,
  ResetDetected,
  CallDisableInterrupts,
  CallEnableInterrupts,
  RequestTimerCall
} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;


typedef BOOLEAN STDCALL
(*PHW_INITIALIZE)(IN PVOID DeviceExtension);

typedef BOOLEAN STDCALL
(*PHW_STARTIO)(IN PVOID DeviceExtension,
	       IN PSCSI_REQUEST_BLOCK Srb);

typedef BOOLEAN STDCALL
(*PHW_INTERRUPT)(IN PVOID DeviceExtension);

typedef VOID STDCALL
(*PHW_TIMER)(IN PVOID DeviceExtension);

typedef VOID STDCALL
(*PHW_DMA_STARTED)(IN PVOID DeviceExtension);

typedef ULONG STDCALL
(*PHW_FIND_ADAPTER)(IN PVOID DeviceExtension,
		    IN PVOID HwContext,
		    IN PVOID BusInformation,
		    IN PCHAR ArgumentString,
		    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
		    OUT PBOOLEAN Again);

typedef BOOLEAN STDCALL
(*PHW_RESET_BUS)(IN PVOID DeviceExtension,
		 IN ULONG PathId);

typedef BOOLEAN STDCALL
(*PHW_ADAPTER_STATE)(IN PVOID DeviceExtension,
		     IN PVOID Context,
		     IN BOOLEAN SaveState);

typedef struct _HW_INITIALIZATION_DATA
{
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
  BOOLEAN TaggedQueuing;
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
ScsiDebugPrint (
	IN	ULONG	DebugPrintLevel,
	IN	PCHAR	DebugMessage,
	...
	);

VOID
STDCALL
ScsiPortCompleteRequest (
	IN	PVOID	HwDeviceExtension,
	IN	UCHAR	PathId,
	IN	UCHAR	TargetId,
	IN	UCHAR	Lun,
	IN	UCHAR	SrbStatus
	);

ULONG
STDCALL
ScsiPortConvertPhysicalAddressToUlong (
	IN	SCSI_PHYSICAL_ADDRESS	Address
	);

SCSI_PHYSICAL_ADDRESS
STDCALL
ScsiPortConvertUlongToPhysicalAddress (
	IN	ULONG	UlongAddress
	);

VOID
STDCALL
ScsiPortFlushDma (
	IN	PVOID	HwDeviceExtension
	);

VOID
STDCALL
ScsiPortFreeDeviceBase (
	IN	PVOID	HwDeviceExtension,
	IN	PVOID	MappedAddress
	);

ULONG
STDCALL
ScsiPortGetBusData (
	IN	PVOID	DeviceExtension,
	IN	ULONG	BusDataType,
	IN	ULONG	SystemIoBusNumber,
	IN	ULONG	SlotNumber,
	IN	PVOID	Buffer,
	IN	ULONG	Length
	);

PVOID
STDCALL
ScsiPortGetDeviceBase (
	IN	PVOID			HwDeviceExtension,
	IN	INTERFACE_TYPE		BusType,
	IN	ULONG			SystemIoBusNumber,
	IN	SCSI_PHYSICAL_ADDRESS	IoAddress,
	IN	ULONG			NumberOfBytes,
	IN	BOOLEAN			InIoSpace
	);

PVOID
STDCALL
ScsiPortGetLogicalUnit (
	IN	PVOID	HwDeviceExtension,
	IN	UCHAR	PathId,
	IN	UCHAR	TargetId,
	IN	UCHAR	Lun
	);

SCSI_PHYSICAL_ADDRESS
STDCALL
ScsiPortGetPhysicalAddress (
	IN	PVOID			HwDeviceExtension,
	IN	PSCSI_REQUEST_BLOCK	Srb OPTIONAL,
	IN	PVOID			VirtualAddress,
	OUT	PULONG			Length
	);

PSCSI_REQUEST_BLOCK
STDCALL
ScsiPortGetSrb (
	IN	PVOID	DeviceExtension,
	IN	UCHAR	PathId,
	IN	UCHAR	TargetId,
	IN	UCHAR	Lun,
	IN	LONG	QueueTag
	);

PVOID
STDCALL
ScsiPortGetUncachedExtension (
	IN	PVOID				HwDeviceExtension,
	IN	PPORT_CONFIGURATION_INFORMATION	ConfigInfo,
	IN	ULONG				NumberOfBytes
	);

PVOID
STDCALL
ScsiPortGetVirtualAddress (
	IN	PVOID			HwDeviceExtension,
	IN	SCSI_PHYSICAL_ADDRESS	PhysicalAddress
	);

ULONG
STDCALL
ScsiPortInitialize (
	IN	PVOID				Argument1,
	IN	PVOID				Argument2,
	IN	struct _HW_INITIALIZATION_DATA	*HwInitializationData,
	IN	PVOID				HwContext
	);

VOID
STDCALL
ScsiPortIoMapTransfer (
	IN	PVOID			HwDeviceExtension,
	IN	PSCSI_REQUEST_BLOCK	Srb,
	IN	ULONG			LogicalAddress,
	IN	ULONG			Length
	);

VOID
STDCALL
ScsiPortLogError (
	IN	PVOID			HwDeviceExtension,
	IN	PSCSI_REQUEST_BLOCK	Srb OPTIONAL,
	IN	UCHAR			PathId,
	IN	UCHAR			TargetId,
	IN	UCHAR			Lun,
	IN	ULONG			ErrorCode,
	IN	ULONG			UniqueId
	);

VOID
STDCALL
ScsiPortMoveMemory (
	OUT	PVOID	Destination,
	IN	PVOID	Source,
	IN	ULONG	Length
	);

VOID
ScsiPortNotification (
	IN	SCSI_NOTIFICATION_TYPE	NotificationType,
	IN	PVOID			HwDeviceExtension,
	...
	);

UCHAR
STDCALL
ScsiPortReadPortUchar (
	IN	PUCHAR	Port
	);

ULONG
STDCALL
ScsiPortReadPortUlong (
	IN	PULONG	Port
	);

USHORT
STDCALL
ScsiPortReadPortUshort (
	IN	PUSHORT	Port
	);

#endif /* __STORAGE_INCLUDE_SRB_H */

/* EOF */