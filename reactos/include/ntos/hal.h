/* $Id: hal.h,v 1.1 2002/09/07 15:12:20 chorns Exp $
 *
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     include/ddk/haltypes.h
 * PURPOSE:                  HAL provided defintions for device drivers
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              23/06/98:   Taken from linux system.h
 */


#ifndef __HAL_H
#define __HAL_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif


typedef struct _ADAPTER_OBJECT {
  int Channel;
  PVOID PagePort;
  PVOID CountPort;
  PVOID OffsetPort;
  KSPIN_LOCK SpinLock;
  PVOID Buffer;
  BOOLEAN Inuse;
} ADAPTER_OBJECT, *PADAPTER_OBJECT;


VOID
STDCALL
IoAssignDriveLetters (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	);

/* HalReturnToFirmware */
#define FIRMWARE_HALT   1
#define FIRMWARE_REBOOT 3


/* PCI bus definitions */

#define PCI_TYPE0_ADDRESSES	6
#define PCI_TYPE1_ADDRESSES	2
#define PCI_TYPE2_ADDRESSES	5

#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8

#define PCI_INVALID_VENDORID                0xFFFF


/* Bit encodings for  PCI_COMMON_CONFIG.HeaderType */

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01


/* Bit encodings for PCI_COMMON_CONFIG.Command */

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040
#define PCI_ENABLE_WAIT_CYCLE               0x0080
#define PCI_ENABLE_SERR                     0x0100
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200


/* Bit encodings for PCI_COMMON_CONFIG.Status */

#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  /* 2 bits wide */
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses */

#define PCI_ADDRESS_IO_SPACE                0x00000001
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses */

#define PCI_ROMADDRESS_ENABLED              0x00000001


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


typedef struct _BUS_HANDLER *PBUS_HANDLER;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;


typedef BOOLEAN STDCALL
(*PHAL_RESET_DISPLAY_PARAMETERS)(ULONG Columns, ULONG Rows);

typedef NTSTATUS STDCALL
(*pHalQuerySystemInformation)(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			      IN ULONG BufferSize,
			      IN OUT PVOID Buffer,
			      OUT PULONG ReturnedLength);


typedef NTSTATUS STDCALL
(*pHalSetSystemInformation)(IN HAL_SET_INFORMATION_CLASS InformationClass,
			    IN ULONG BufferSize,
			    IN PVOID Buffer);


typedef NTSTATUS STDCALL
(*pHalQueryBusSlots)(IN PBUS_HANDLER BusHandler,
		     IN ULONG BufferSize,
		     OUT PULONG SlotNumbers,
		     OUT PULONG ReturnedLength);


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
  BOOLEAN PowerSupported;
  BOOLEAN ResumeSupported;
  BOOLEAN LockSupported;
  BOOLEAN EjectSupported;
  BOOLEAN Removable;
} BCTL_DEVICE_CAPABILITIES, *PBCTL_DEVICE_CAPABILITIES;


typedef struct _DEVICE_CONTROL_CONTEXT
{
  NTSTATUS Status;
  PDEVICE_HANDLER_OBJECT DeviceHandler;
  PDEVICE_OBJECT DeviceObject;
  ULONG ControlCode;
  PVOID Buffer;
  PULONG BufferLength;
  PVOID Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;


typedef VOID STDCALL
(*PDEVICE_CONTROL_COMPLETION)(IN PDEVICE_CONTROL_CONTEXT ControlContext);


typedef NTSTATUS STDCALL
(*pHalDeviceControl)(IN PDEVICE_HANDLER_OBJECT DeviceHandler,
		     IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG ControlCode,
		     IN OUT PVOID Buffer OPTIONAL,
		     IN OUT PULONG BufferLength OPTIONAL,
		     IN PVOID Context,
		     IN PDEVICE_CONTROL_COMPLETION CompletionRoutine);

typedef VOID FASTCALL
(*pHalExamineMBR)(IN PDEVICE_OBJECT DeviceObject,
		  IN ULONG SectorSize,
		  IN ULONG MBRTypeIdentifier,
		  OUT PVOID *Buffer);

typedef VOID FASTCALL
(*pHalIoAssignDriveLetters)(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
			    IN PSTRING NtDeviceName,
			    OUT PUCHAR NtSystemPath,
			    OUT PSTRING NtSystemPathString);

typedef NTSTATUS FASTCALL
(*pHalIoReadPartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			    IN ULONG SectorSize,
			    IN BOOLEAN ReturnRecognizedPartitions,
			    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef NTSTATUS FASTCALL
(*pHalIoSetPartitionInformation)(IN PDEVICE_OBJECT DeviceObject,
				 IN ULONG SectorSize,
				 IN ULONG PartitionNumber,
				 IN ULONG PartitionType);

typedef NTSTATUS FASTCALL
(*pHalIoWritePartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			     IN ULONG SectorSize,
			     IN ULONG SectorsPerTrack,
			     IN ULONG NumberOfHeads,
			     IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer);

typedef PBUS_HANDLER FASTCALL
(*pHalHandlerForBus)(IN INTERFACE_TYPE InterfaceType,
		     IN ULONG BusNumber);

typedef VOID FASTCALL
(*pHalReferenceBusHandler)(IN PBUS_HANDLER BusHandler);


typedef struct _HAL_DISPATCH_TABLE
{
  ULONG				Version;
  pHalQuerySystemInformation	HalQuerySystemInformation;
  pHalSetSystemInformation	HalSetSystemInformation;
  pHalQueryBusSlots		HalQueryBusSlots;
  pHalDeviceControl		HalDeviceControl;
  pHalExamineMBR		HalExamineMBR;
  pHalIoAssignDriveLetters	HalIoAssignDriveLetters;
  pHalIoReadPartitionTable	HalIoReadPartitionTable;
  pHalIoSetPartitionInformation	HalIoSetPartitionInformation;
  pHalIoWritePartitionTable	HalIoWritePartitionTable;
  pHalHandlerForBus		HalReferenceHandlerForBus;
  pHalReferenceBusHandler	HalReferenceBusHandler;
  pHalReferenceBusHandler	HalDereferenceBusHandler;
} HAL_DISPATCH_TABLE, *PHAL_DISPATCH_TABLE;

#define HAL_DISPATCH_VERSION 1

extern NTOSAPI PHAL_DISPATCH_TABLE HalDispatchTable;

/* Hal private dispatch table */

typedef struct _HAL_PRIVATE_DISPATCH_TABLE
{
  ULONG Version;
} HAL_PRIVATE_DISPATCH_TABLE, *PHAL_PRIVATE_DISPATCH_TABLE;

#define HAL_PRIVATE_DISPATCH_VERSION 1

extern NTOSAPI PHAL_PRIVATE_DISPATCH_TABLE HalPrivateDispatchTable;


/*
 * Kernel debugger section
 */

typedef struct _KD_PORT_INFORMATION
{
  ULONG ComPort;
  ULONG BaudRate;
  ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;


#ifdef __NTHAL__
extern ULONG KdComPortInUse;
#else
extern ULONG KdComPortInUse;
#endif


VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTSTATUS STDCALL
HalAdjustResourceList(PCM_RESOURCE_LIST	Resources);

NTSTATUS STDCALL
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
			  IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG NumberOfMapRegisters,
			  IN PDRIVER_CONTROL ExecutionRoutine,
			  IN PVOID Context);

PVOID STDCALL
HalAllocateCommonBuffer(PADAPTER_OBJECT AdapterObject,
			ULONG Length,
			PPHYSICAL_ADDRESS LogicalAddress,
			BOOLEAN CacheEnabled);

PVOID STDCALL
HalAllocateCrashDumpRegisters(IN PADAPTER_OBJECT AdapterObject,
			      IN OUT PULONG NumberOfMapRegisters);

BOOLEAN STDCALL
HalAllProcessorsStarted(VOID);

NTSTATUS STDCALL
HalAssignSlotResources(
	PUNICODE_STRING		RegistryPath,
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PDEVICE_OBJECT		DeviceObject,
	INTERFACE_TYPE		BusType,
	ULONG			BusNumber,
	ULONG			SlotNumber,
	PCM_RESOURCE_LIST	*AllocatedResources
	);

BOOLEAN STDCALL
HalBeginSystemInterrupt(ULONG Vector,
			KIRQL Irql,
			PKIRQL OldIrql);

VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count);

/*
FASTCALL
HalClearSoftwareInterrupt
*/

BOOLEAN STDCALL
HalDisableSystemInterrupt(ULONG Vector,
			  ULONG Unknown2);

VOID STDCALL
HalDisplayString(IN PCH String);

BOOLEAN STDCALL
HalEnableSystemInterrupt(ULONG Vector,
			 ULONG Unknown2,
			 ULONG Unknown3);

VOID STDCALL
HalEndSystemInterrupt(KIRQL Irql,
		      ULONG Unknown2);

BOOLEAN STDCALL
HalFlushCommonBuffer(ULONG Unknown1,
		     ULONG Unknown2,
		     ULONG Unknown3,
		     ULONG Unknown4,
		     ULONG Unknown5,
		     ULONG Unknown6,
		     ULONG Unknown7,
		     ULONG Unknown8);

VOID STDCALL
HalFreeCommonBuffer(PADAPTER_OBJECT AdapterObject,
		    ULONG Length,
		    PHYSICAL_ADDRESS LogicalAddress,
		    PVOID VirtualAddress,
		    BOOLEAN CacheEnabled);

PADAPTER_OBJECT STDCALL
HalGetAdapter(PDEVICE_DESCRIPTION DeviceDescription,
	      PULONG NumberOfMapRegisters);

ULONG STDCALL
HalGetBusData(BUS_DATA_TYPE BusDataType,
	      ULONG BusNumber,
	      ULONG SlotNumber,
	      PVOID Buffer,
	      ULONG Length);

ULONG STDCALL
HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType,
		      ULONG BusNumber,
		      ULONG SlotNumber,
		      PVOID Buffer,
		      ULONG Offset,
		      ULONG Length);

BOOLEAN STDCALL
HalGetEnvironmentVariable(IN PCH Name,
			  OUT PCH Value,
			  IN USHORT ValueLength);

ULONG STDCALL
HalGetInterruptVector(INTERFACE_TYPE InterfaceType,
		      ULONG BusNumber,
		      ULONG BusInterruptLevel,
		      ULONG BusInterruptVector,
		      PKIRQL Irql,
		      PKAFFINITY Affinity);

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
		       PVOID ProcessorStack);

BOOLEAN STDCALL
HalInitSystem(ULONG BootPhase,
	      PLOADER_PARAMETER_BLOCK LoaderBlock);

BOOLEAN STDCALL
HalMakeBeep(ULONG Frequency);

VOID STDCALL
HalQueryDisplayParameters(PULONG DispSizeX,
			  PULONG DispSizeY,
			  PULONG CursorPosX,
			  PULONG CursorPosY);

VOID STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time);

/* Is this function really exported ?? */
VOID
HalQuerySystemInformation(VOID);

ULONG STDCALL
HalReadDmaCounter(PADAPTER_OBJECT AdapterObject);

VOID STDCALL
HalReportResourceUsage(VOID);

VOID STDCALL
HalRequestIpi(ULONG Unknown);

/*
FASTCALL
HalRequestSoftwareInterrupt
*/

VOID STDCALL
HalReturnToFirmware(ULONG Action);

ULONG STDCALL
HalSetBusData(BUS_DATA_TYPE BusDataType,
	      ULONG BusNumber,
	      ULONG SlotNumber,
	      PVOID Buffer,
	      ULONG Length);

ULONG STDCALL
HalSetBusDataByOffset(BUS_DATA_TYPE BusDataType,
		      ULONG BusNumber,
		      ULONG SlotNumber,
		      PVOID Buffer,
		      ULONG Offset,
		      ULONG Length);

VOID STDCALL
HalSetDisplayParameters(ULONG CursorPosX,
			ULONG CursorPosY);

BOOLEAN STDCALL
HalSetEnvironmentVariable(IN PCH Name,
			  IN PCH Value);

/*
HalSetProfileInterval
*/

VOID STDCALL
HalSetRealTimeClock(PTIME_FIELDS Time);

/*
HalSetTimeIncrement
*/

BOOLEAN STDCALL
HalStartNextProcessor(ULONG Unknown1,
		      ULONG Unknown2);

/*
HalStartProfileInterrupt
*/

/*
HalStopProfileInterrupt
*/

ULONG FASTCALL
HalSystemVectorDispatchEntry(ULONG Unknown1,
			     ULONG Unknown2,
			     ULONG Unknown3);

BOOLEAN STDCALL
HalTranslateBusAddress(INTERFACE_TYPE InterfaceType,
		       ULONG BusNumber,
		       PHYSICAL_ADDRESS BusAddress,
		       PULONG AddressSpace,
		       PPHYSICAL_ADDRESS TranslatedAddress);


/*
 * Kernel debugger support functions
 */

BOOLEAN STDCALL
KdPortInitialize(PKD_PORT_INFORMATION PortInformation,
		 DWORD Unknown1,
		 DWORD Unknown2);

BOOLEAN STDCALL
KdPortGetByte(PUCHAR ByteRecieved);

BOOLEAN STDCALL
KdPortPollByte(PUCHAR ByteRecieved);

VOID STDCALL
KdPortPutByte(UCHAR ByteToSend);


/*
 * Port I/O functions
 */

VOID STDCALL
READ_PORT_BUFFER_UCHAR(PUCHAR Port,
		       PUCHAR Value,
		       ULONG Count);

VOID STDCALL
READ_PORT_BUFFER_ULONG(PULONG Port,
		       PULONG Value,
		       ULONG Count);

VOID STDCALL
READ_PORT_BUFFER_USHORT(PUSHORT Port,
			PUSHORT Value,
			ULONG Count);

UCHAR STDCALL
READ_PORT_UCHAR(PUCHAR Port);

ULONG STDCALL
READ_PORT_ULONG(PULONG Port);

USHORT STDCALL
READ_PORT_USHORT(PUSHORT Port);

VOID STDCALL
WRITE_PORT_BUFFER_UCHAR(PUCHAR Port,
			PUCHAR Value,
			ULONG Count);

VOID STDCALL
WRITE_PORT_BUFFER_ULONG(PULONG Port,
			PULONG Value,
			ULONG Count);

VOID STDCALL
WRITE_PORT_BUFFER_USHORT(PUSHORT Port,
			 PUSHORT Value,
			 ULONG Count);

VOID STDCALL
WRITE_PORT_UCHAR(PUCHAR Port,
		 UCHAR Value);

VOID STDCALL
WRITE_PORT_ULONG(PULONG Port,
		 ULONG Value);

VOID STDCALL
WRITE_PORT_USHORT(PUSHORT Port,
		  USHORT Value);

BOOLEAN STDCALL
KdPortInitializeEx(
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2);

BOOLEAN
STDCALL
KdPortGetByteEx(
  PKD_PORT_INFORMATION PortInformation,
  PUCHAR  ByteRecieved);

BOOLEAN STDCALL
KdPortPollByteEx(
  PKD_PORT_INFORMATION PortInformation,
  PUCHAR  ByteRecieved);

VOID STDCALL
KdPortPutByteEx(
  PKD_PORT_INFORMATION PortInformation,
  UCHAR ByteToSend);

#endif /* __HAL_H */

/* EOF */
