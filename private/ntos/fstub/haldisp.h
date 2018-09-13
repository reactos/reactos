/*++ BUILD Version: 0001

Copyright (c) 1994  Microsoft Corporation

Module Name:

    haldisp.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT haldisp

Author:


Revision History:


--*/

#if defined(NO_LEGACY_DRIVERS)
#define xHalIoAssignDriveLetters IoAssignDriveLetters
#define xHalIoReadPartitionTable IoReadPartitionTable
#define xHalIoSetPartitionInformation IoSetPartitionInformation
#define xHalIoWritePartitionTable IoWritePartitionTable
#endif // NO_LEGACY_DRIVERS

//
// Strcuture definitions.
//

typedef struct _ADAPTER_OBJECT {
    DMA_ADAPTER DmaAdapter;
    PADAPTER_OBJECT RealAdapterObject;
}ADAPTER_OBJECT;

typedef struct _XHAL_WAIT_CONTEXT_BLOCK {
    PMDL Mdl;
    PVOID CurrentVa;
    ULONG Length;
    PADAPTER_OBJECT RealAdapterObject;
    PDRIVER_LIST_CONTROL DriverExecutionRoutine;
    PVOID DriverContext;
    PIRP CurrentIrp;
    ULONG MapRegisterLock;
    UCHAR WriteToDevice;
    UCHAR MdlCount;
    PVOID MapRegisterBase[];
} XHAL_WAIT_CONTEXT_BLOCK, *PXHAL_WAIT_CONTEXT_BLOCK;

//
// Function prototypes
//

NTSTATUS
xHalQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    );

NTSTATUS
xHalSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer
    );

NTSTATUS
xHalQueryBusSlots(
    IN PBUS_HANDLER         BusHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

VOID
xHalSetWakeEnable(
    IN BOOLEAN              Enable
    );

VOID
xHalSetWakeAlarm(
    IN ULONGLONG        WakeTime,
    IN PTIME_FIELDS     WakeTimeFields
    );

VOID
xHalLocateHiberRanges(
    IN PVOID MemoryMap
    );

NTSTATUS
xHalRegisterBusHandler(
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           ConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

PBUS_HANDLER
FASTCALL
xHalHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

VOID
FASTCALL
xHalExamineMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    );

VOID
FASTCALL
xHalIoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

NTSTATUS
FASTCALL
xHalIoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

NTSTATUS
FASTCALL
xHalIoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTSTATUS
FASTCALL
xHalIoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );


VOID
FASTCALL
xHalReferenceHandler (
    IN PBUS_HANDLER     Handler
    );

NTSTATUS
xHalInitPnpDriver(
    VOID
    );

NTSTATUS
xHalInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

PDMA_ADAPTER
xHalGetDmaAdapter (
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

VOID
xHalPutDmaAdapter (
    PDMA_ADAPTER DmaAdapter
    );

PVOID
xHalAllocateCommonBuffer (
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

VOID
xHalFreeCommonBuffer (
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

NTSTATUS
xHalAllocateAdapterChannel (
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

BOOLEAN
xHalFlushAdapterBuffers (
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

VOID
xHalFreeAdapterChannel (
    IN PDMA_ADAPTER DmaAdapter
    );

VOID
xHalFreeMapRegisters (
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

PHYSICAL_ADDRESS
xHalMapTransfer (
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

ULONG
xHalGetDmaAlignment (
    IN PDMA_ADAPTER DmaAdapter
    );

ULONG
xHalReadDmaCounter (
    IN PDMA_ADAPTER DmaAdapter
    );

NTSTATUS
xHalGetScatterGatherList (
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );


VOID
xHalPutScatterGatherList (
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

IO_ALLOCATION_ACTION
xHalpAllocateAdapterCallback (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );
    
NTSTATUS
xHalGetInterruptTranslator(
	IN INTERFACE_TYPE ParentInterfaceType,
	IN ULONG ParentBusNumber,
	IN INTERFACE_TYPE BridgeInterfaceType,
	IN USHORT Size,
	IN USHORT Version,
	OUT PTRANSLATOR_INTERFACE Translator,
	OUT PULONG BridgeBusNumber
	);

BOOLEAN
xHalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );
    
NTSTATUS
xHalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

VOID
xHalHaltSystem(
    VOID
    );
    
