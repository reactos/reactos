/* $Id: halddk.h,v 1.3 2000/07/01 18:20:41 ekohl Exp $
 *
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     include/internal/hal/ddk.h
 * PURPOSE:                  HAL provided defintions for device drivers
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              23/06/98:   Taken from linux system.h
 */


#ifndef __INCLUDE_INTERNAL_HAL_DDK_H
#define __INCLUDE_INTERNAL_HAL_DDK_H

/* HalReturnToFirmware */
#define FIRMWARE_HALT   1
#define FIRMWARE_REBOOT 3


enum
{
   DEVICE_DESCRIPTION_VERSION,
   DEVICE_DESCRIPTION_VERSION1,
};

typedef ULONG DMA_WIDTH;
typedef ULONG DMA_SPEED;

/*
 * PURPOSE: Types for HalGetBusData
 */
typedef enum _BUS_DATA_TYPE
{
   ConfigurationSpaceUndefined = -1,
   Cmos,
   EisaConfiguration,
   Pos,
   CbusConfiguration,
   PCIConfiguration,
   VMEConfiguration,
   NuBusConfiguration,
   PCMCIAConfiguration,
   MPIConfiguration,
   MPSAConfiguration,
   PNPISAConfiguration,
   MaximumBusDataType,
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _DEVICE_DESCRIPTION
{
   ULONG Version;
   BOOLEAN Master;
   BOOLEAN ScatterGather;
   BOOLEAN DemandMode;
   BOOLEAN AutoInitialize;
   BOOLEAN Dma32BitAddress;
   BOOLEAN IgnoreCount;
   BOOLEAN Reserved1;
   BOOLEAN Reserved2;
   ULONG BusNumber;
   ULONG DmaChannel;
   INTERFACE_TYPE InterfaceType;
   DMA_WIDTH DmaWidth;
   DMA_SPEED DmaSpeed;
   ULONG MaximumLength;
   ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

typedef BOOLEAN (*PHAL_RESET_DISPLAY_PARAMETERS)(ULONG Columns, ULONG Rows);

/* Hal dispatch table */

typedef enum _HAL_QUERY_INFORMATION_CLASS
{
	HalInstalledBusInformation,
	HalProfileSourceInformation,
	HalSystemDockInformation,
	HalPowerInformation,
	HalProcessorSpeedInformation,
	HalCallbackInformation,
	HalMapRegisterInformation,
	HalMcaLogInformation,
	HalFrameBufferCachingInformation,
	HalDisplayBiosInformation
	/* information levels >= 0x8000000 reserved for OEM use */
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS
{
	HalProfileSourceInterval,
	HalProfileSourceInterruptHandler,
	HalMcaRegisterDriver
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;


typedef
NTSTATUS
(*pHalQuerySystemInformation) (
	IN	HAL_QUERY_INFORMATION_CLASS	InformationClass,
	IN	ULONG				BufferSize,
	IN OUT	PVOID				Buffer,
	OUT	PULONG				ReturnedLength
	);


typedef
NTSTATUS
(*pHalSetSystemInformation) (
	IN	HAL_SET_INFORMATION_CLASS	InformationClass,
	IN	ULONG				BufferSize,
	IN	PVOID				Buffer
	);


typedef
NTSTATUS
(*pHalQueryBusSlots) (
//	IN	PBUS_HANDLER	BusHandler,
	IN	PVOID		BusHandler,
	IN	ULONG		BufferSize,
	OUT	PULONG		SlotNumbers,
	OUT	PULONG		ReturnedLength
	);


/* Control codes of HalDeviceControl function */
#define BCTL_EJECT				0x0001
#define BCTL_QUERY_DEVICE_ID			0x0002
#define BCTL_QUERY_DEVICE_UNIQUE_ID		0x0003
#define BCTL_QUERY_DEVICE_CAPABILITIES		0x0004
#define BCTL_QUERY_DEVICE_RESOURCES		0x0005
#define BCTL_QUERY_DEVICE_RESOURCE_REQUIREMENTS	0x0006
#define BCTL_QUERY_EJECT                            0x0007
#define BCTL_SET_LOCK                               0x0008
#define BCTL_SET_POWER                              0x0009
#define BCTL_SET_RESUME                             0x000A
#define BCTL_SET_DEVICE_RESOURCES                   0x000B

/* Defines for BCTL structures */
typedef struct
{
	BOOLEAN	PowerSupported;
	BOOLEAN	ResumeSupported;
	BOOLEAN	LockSupported;
	BOOLEAN	EjectSupported;
	BOOLEAN	Removable;
} BCTL_DEVICE_CAPABILITIES, *PBCTL_DEVICE_CAPABILITIES;


typedef struct _DEVICE_CONTROL_CONTEXT
{
	NTSTATUS		Status;
//	PDEVICE_HANDLER_OBJECT	DeviceHandler;
	PVOID			DeviceHandler;
	PDEVICE_OBJECT		DeviceObject;
	ULONG			ControlCode;
	PVOID			Buffer;
	PULONG			BufferLength;
	PVOID			Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;


typedef
VOID
(*PDEVICE_CONTROL_COMPLETION) (
	IN	PDEVICE_CONTROL_CONTEXT	ControlContext
	);


typedef
NTSTATUS
(*pHalDeviceControl) (
//	IN	PDEVICE_HANDLER_OBJECT		DeviceHandler,
	IN	PVOID				DeviceHandler,
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	ULONG				ControlCode,
	IN OUT	PVOID				Buffer OPTIONAL,
	IN OUT	PULONG				BufferLength OPTIONAL,
	IN	PVOID				Context,
	IN	PDEVICE_CONTROL_COMPLETION	CompletionRoutine
	);


typedef
VOID
(FASTCALL *pHalExamineMBR) (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	ULONG		SectorSize,
	IN	ULONG		MBRTypeIdentifier,
	OUT	PVOID		* Buffer
	);

typedef
VOID
(FASTCALL *pHalIoAssignDriveLetters) (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	);

typedef
NTSTATUS
(FASTCALL *pHalIoReadPartitionTable) (
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	ULONG				SectorSize,
	IN	BOOLEAN				ReturnRecognizedPartitions,
	OUT	PDRIVE_LAYOUT_INFORMATION	* PartitionBuffer
	);

typedef
NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation) (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	ULONG		SectorSize,
	IN	ULONG		PartitionNumber,
	IN	ULONG		PartitionType
	);

typedef
NTSTATUS
(FASTCALL *pHalIoWritePartitionTable) (
	IN	PDEVICE_OBJECT			DeviceObject,
	IN	ULONG				SectorSize,
	IN	ULONG				SectorsPerTrack,
	IN	ULONG				NumberOfHeads,
	IN	PDRIVE_LAYOUT_INFORMATION	PartitionBuffer
	);

typedef
//PBUS_HANDLER
PVOID
(FASTCALL *pHalHandlerForBus) (
	IN	INTERFACE_TYPE	InterfaceType,
	IN	ULONG		BusNumber
	);

typedef
VOID
(FASTCALL *pHalReferenceBusHandler) (
//	IN	PBUS_HANDLER	BusHandler
	IN	PVOID		BusHandler
	);

typedef struct _HAL_DISPATCH
{
	ULONG				Version;
	pHalQuerySystemInformation	HalQuerySystemInformation;
	pHalSetSystemInformation	HalSetSystemInformation;
	pHalQueryBusSlots		HalQueryBusSlots;
	pHalDeviceControl		HalDeviceControl;
	pHalExamineMBR			HalExamineMBR;
	pHalIoAssignDriveLetters	HalIoAssignDriveLetters;
	pHalIoReadPartitionTable	HalIoReadPartitionTable;
	pHalIoSetPartitionInformation	HalIoSetPartitionInformation;
	pHalIoWritePartitionTable	HalIoWritePartitionTable;
	pHalHandlerForBus		HalReferenceHandlerForBus;
	pHalReferenceBusHandler		HalReferenceBusHandler;
	pHalReferenceBusHandler		HalDereferenceBusHandler;
} HAL_DISPATCH, *PHAL_DISPATCH;

#define HAL_DISPATCH_VERSION 1

#ifdef __NTOSKRNL__
extern HAL_DISPATCH EXPORTED HalDispatchTable;
#else
extern HAL_DISPATCH IMPORTED HalDispatchTable;
#endif


/* Hal private dispatch table */

typedef struct _HAL_PRIVATE_DISPATCH
{
	ULONG				Version;


} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

#define HAL_PRIVATE_DISPATCH_VERSION 1

#ifdef __NTOSKRNL__
extern HAL_PRIVATE_DISPATCH EXPORTED HalPrivateDispatchTable;
#else
extern HAL_PRIVATE_DISPATCH IMPORTED HalPrivateDispatchTable;
#endif


VOID
STDCALL
HalAcquireDisplayOwnership (
	PHAL_RESET_DISPLAY_PARAMETERS	ResetDisplayParameters
	);

NTSTATUS
STDCALL
HalAdjustResourceList (
	PCM_RESOURCE_LIST	Resources
	);

NTSTATUS
STDCALL
HalAllocateAdapterChannel (
	IN	PADAPTER_OBJECT	AdapterObject,
	ULONG	Unknown2,
	ULONG	Unknown3,
	ULONG	Unknown4
	);

PVOID
STDCALL
HalAllocateCommonBuffer (
	PADAPTER_OBJECT		AdapterObject,
	ULONG			Length,
	PPHYSICAL_ADDRESS	LogicalAddress,
	BOOLEAN			CacheEnabled
	);

PVOID
STDCALL
HalAllocateCrashDumpRegisters (
	IN	PADAPTER_OBJECT	AdapterObject,
	IN OUT	PULONG		NumberOfMapRegisters
	);

BOOLEAN
STDCALL
HalAllProcessorsStarted (
	VOID
	);

NTSTATUS
STDCALL
HalAssignSlotResources (
	PUNICODE_STRING		RegistryPath,
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PDEVICE_OBJECT		DeviceObject,
	INTERFACE_TYPE		BusType,
	ULONG			BusNumber,
	ULONG			SlotNumber,
	PCM_RESOURCE_LIST	*AllocatedResources
	);

/*
HalBeginSystemInterrupt
*/

/*
HalCalibratePerformanceCounter
*/

/*
FASTCALL
HalClearSoftwareInterrupt
*/

/*
HalDisableSystemInterrupt
*/

VOID
STDCALL
HalDisplayString (
	IN	PCH	String
	);

/*
HalEnableSystemInterrupt
*/

/*
HalEndSystemInterrupt
*/

/* Is this function really exported ?? */
VOID
HalExamineMBR (
	PDEVICE_OBJECT	DeviceObject,
	ULONG		SectorSize,
	ULONG		MBRTypeIdentifier,
	PVOID		Buffer
	);

BOOLEAN
STDCALL
HalFlushCommonBuffer (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3,
	ULONG	Unknown4,
	ULONG	Unknown5,
	ULONG	Unknown6,
	ULONG	Unknown7,
	ULONG	Unknown8
	);

VOID
STDCALL
HalFreeCommonBuffer (
	PADAPTER_OBJECT		AdapterObject,
	ULONG			Length,
	PHYSICAL_ADDRESS	LogicalAddress,
	PVOID			VirtualAddress,
	BOOLEAN			CacheEnabled
	);

PADAPTER_OBJECT
STDCALL
HalGetAdapter (
	PDEVICE_DESCRIPTION	DeviceDescription,
	PULONG			NumberOfMapRegisters
	);

ULONG
STDCALL
HalGetBusData (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Length
	);

ULONG
STDCALL
HalGetBusDataByOffset (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
	);

/* Is this function really exported ?? */
ULONG
HalGetDmaAlignmentRequirement (
	VOID
	);

/*
HalGetEnvironmentVariable
*/

ULONG
STDCALL
HalGetInterruptVector (
	INTERFACE_TYPE	InterfaceType,
	ULONG		BusNumber,
	ULONG		BusInterruptLevel,
	ULONG		BusInterruptVector,
	PKIRQL		Irql,
	PKAFFINITY	Affinity
	);

BOOLEAN
STDCALL
HalInitSystem (
	ULONG			BootPhase,
	PLOADER_PARAMETER_BLOCK	LoaderBlock
	);

VOID
STDCALL
HalInitializeProcessor (
	ULONG	ProcessorNumber
	);

BOOLEAN
STDCALL
HalMakeBeep (
	ULONG Frequency
	);

VOID
STDCALL
HalQueryDisplayParameters (
	PULONG	DispSizeX,
	PULONG	DispSizeY,
	PULONG	CursorPosX,
	PULONG	CursorPosY
	);

VOID
STDCALL
HalQueryRealTimeClock (
	PTIME_FIELDS	Time
	);

/* Is this function really exported ?? */
VOID
HalQuerySystemInformation(VOID);

ULONG
STDCALL
HalReadDmaCounter (
	PADAPTER_OBJECT	AdapterObject
	);

VOID
STDCALL
HalReportResourceUsage (
	VOID
	);

VOID
STDCALL
HalRequestIpi (
	ULONG	Unknown
	);

/*
FASTCALL
HalRequestSoftwareInterrupt
*/

VOID
STDCALL
HalReturnToFirmware (
	ULONG	Action
	);

ULONG
STDCALL
HalSetBusData (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Length
	);

ULONG
STDCALL
HalSetBusDataByOffset (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
	);

VOID
STDCALL
HalSetDisplayParameters (
	ULONG	CursorPosX,
	ULONG	CursorPosY
	);

/*
HalSetEnvironmentVariable
*/

/*
HalSetProfileInterval
*/

VOID
STDCALL
HalSetRealTimeClock (
	PTIME_FIELDS	Time
	);

/*
HalSetTimeIncrement
*/

BOOLEAN
STDCALL
HalStartNextProcessor (
	ULONG	Unknown1,
	ULONG	Unknown2
	);

/*
HalStartProfileInterrupt
*/

/*
HalStopProfileInterrupt
*/

ULONG
FASTCALL
HalSystemVectorDispatchEntry (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	);

BOOLEAN
STDCALL
HalTranslateBusAddress (
	INTERFACE_TYPE		InterfaceType,
	ULONG			BusNumber,
	PHYSICAL_ADDRESS	BusAddress,
	PULONG			AddressSpace,
	PPHYSICAL_ADDRESS	TranslatedAddress
	);

/*
 * Kernel debugger section
 */

typedef struct _KD_PORT_INFORMATION
{
	ULONG ComPort;
	ULONG BaudRate;
	ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;


#if defined(__HAL__) || defined(__NTOSKRNL__)
extern ULONG KdComPortInUse __declspec(dllexport);
#else
extern ULONG KdComPortInUse __declspec(dllimport);
#endif

BOOLEAN
STDCALL
KdPortInitialize (PKD_PORT_INFORMATION PortInformation,
		  DWORD Unknown1,
		  DWORD Unknown2);

BOOLEAN
STDCALL
KdPortGetByte (PUCHAR ByteRecieved);

BOOLEAN
STDCALL
KdPortPollByte (PUCHAR ByteRecieved);

VOID
STDCALL
KdPortPutByte (UCHAR ByteToSend);


/*
 * Port I/O functions
 */

VOID
STDCALL
READ_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, ULONG Count);

VOID
STDCALL
READ_PORT_BUFFER_ULONG (PULONG Port, PULONG Value, ULONG Count);

VOID
STDCALL
READ_PORT_BUFFER_USHORT (PUSHORT Port, PUSHORT Value, ULONG Count);

UCHAR
STDCALL
READ_PORT_UCHAR (PUCHAR Port);

ULONG
STDCALL
READ_PORT_ULONG (PULONG Port);

USHORT
STDCALL
READ_PORT_USHORT (PUSHORT Port);

VOID
STDCALL
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_BUFFER_ULONG (PULONG Port, PULONG Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_BUFFER_USHORT (PUSHORT Port, PUSHORT Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_UCHAR (PUCHAR Port, UCHAR Value);

VOID
STDCALL
WRITE_PORT_ULONG (PULONG Port, ULONG Value);

VOID
STDCALL
WRITE_PORT_USHORT (PUSHORT Port, USHORT Value);


#endif /* __INCLUDE_INTERNAL_HAL_DDK_H */
