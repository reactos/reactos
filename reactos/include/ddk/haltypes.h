/* $Id: haltypes.h,v 1.1 2001/08/27 01:18:57 ekohl Exp $
 *
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     include/ddk/haltypes.h
 * PURPOSE:                  HAL provided defintions for device drivers
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              23/06/98:   Taken from linux system.h
 */


#ifndef __INCLUDE_DDK_HALTYPES_H
#define __INCLUDE_DDK_HALTYPES_H


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


typedef struct _HAL_DISPATCH
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
  ULONG Version;
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

#define HAL_PRIVATE_DISPATCH_VERSION 1


#ifdef __NTOSKRNL__
extern HAL_PRIVATE_DISPATCH EXPORTED HalPrivateDispatchTable;
#else
extern HAL_PRIVATE_DISPATCH IMPORTED HalPrivateDispatchTable;
#endif



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
extern ULONG EXPORTED KdComPortInUse;
#else
extern ULONG IMPORTED KdComPortInUse;
#endif

#endif /* __INCLUDE_DDK_HALTYPES_H */

/* EOF */