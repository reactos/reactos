/* $Id: stubs.c,v 1.2 1999/12/27 22:27:39 ea Exp $
 *
 * services/net/ndis/ndis/stubs.c
 *
 * NOTE: remove the stub from this file, if you 
 * implement the function.
 *
 */
#include <ntos.h>

/*
NDIS_BUFFER_TO_SPAN_PAGES
*/

VOID
STDCALL
NdisAcquireSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


VOID
STDCALL
NdisAdjustBufferLength (
	IN	PNDIS_BUFFER	Buffer,
	IN	UINT		Length
	)
{
}


VOID
STDCALL
NdisAllocateBuffer (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_BUFFER	* Buffer,
	IN	NDIS_HANDLE	PoolHandle,
	IN	PVOID		VirtualAddress,
	IN	UINT		Length
	)
{
}


VOID
STDCALL
NdisAllocateBufferPool (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_HANDLE	PoolHandle,
	IN	UINT		NumberOfDescriptors
	)
{
}


NDIS_STATUS
STDCALL
NdisAllocateMemory (
	OUT	PVOID			* VirtualAddress,
	IN	UINT			Length,
	IN	UINT			MemoryFlags,
	IN	NDIS_PHYSICAL_ADDRESS	HighestAcceptableAddress
	)
{
	return 0;
}


VOID
STDCALL
NdisAllocatePacket (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_PACKET	* Packet,
	IN	NDIS_HANDLE	PoolHandle
	)
{
}


VOID
STDCALL
NdisAllocatePacketPool (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_HANDLE	PoolHandle,
	IN	UINT		NumberOfDescriptors,
	IN	UINT		ProtocolReservedLength
        )
{
}


VOID
STDCALL
NdisAllocateSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


NDIS_STATUS
STDCALL
NdisAnsiStringToUnicodeString (
	IN OUT	PNDIS_STRING		DestinationString,
        IN	PNDIS_ANSI_STRING	SourceString
	)
{
	return 0;
}


VOID
STDCALL
NdisChainBufferAtBack (
	IN OUT	PNDIS_PACKET	Packet,
	IN OUT	PNDIS_BUFFER	Buffer
	)
{
}


VOID
STDCALL
NdisChainBufferAtFront (
	IN OUT	PNDIS_PACKET	Packet,
	IN OUT	PNDIS_BUFFER	Buffer
	)
{
}


VOID
STDCALL
NdisCloseConfiguration (
	IN	NDIS_HANDLE	ConfigurationHandle
	)
{
}


VOID
STDCALL
NdisCloseFile (
	IN	NDIS_HANDLE	FileHandle
	)
{
}


VOID
STDCALL
NdisCopyBuffer (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_BUFFER	* Buffer,
	IN	NDIS_HANDLE	PoolHandle,
	IN	PVOID		MemoryDescriptor,
	IN	UINT		Offset,
	IN	UINT		Length
	)
{
}


VOID
STDCALL
NdisCopyFromPacketToPacket (
	IN	PNDIS_PACKET	Destination,
	IN	UINT		DestinationOffset,
	IN	UINT		BytesToCopy,
	IN	PNDIS_PACKET	Source,
	IN	UINT		SourceOffset,
	OUT	PUINT		BytesCopied
	)
{
}


VOID
STDCALL
NdisCreateLookaheadBufferFromSharedMemory (
	IN	PVOID	pSharedMemory,
	IN	UINT	LookaheadLength,
	OUT	PVOID	* pLookaheadBuffer
	)
{
}


VOID
STDCALL
NdisDestroyLookaheadBufferFromSharedMemory (
	IN	PVOID	pLookaheadBuffer
	)
{
}


VOID
STDCALL
NdisDprAcquireSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


VOID
STDCALL
NdisDprAllocatePacket (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_PACKET	* Packet,
	IN	NDIS_HANDLE	PoolHandle
	)
{
}


VOID
STDCALL
NdisDprFreePacket (
	IN	PNDIS_PACKET	Packet
	)
{
}


VOID
STDCALL
NdisDprReleaseSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


BOOLEAN
STDCALL
NdisEqualString (
	IN	PNDIS_STRING	String1,
	IN	PNDIS_STRING	String2,
	IN	BOOLEAN		CaseInsensitive
	)
{
}


VOID
STDCALL
NdisFlushBuffer (
	IN	PNDIS_BUFFER	Buffer,
	IN	BOOLEAN		WriteToDevice
	)
{
}


VOID
STDCALL
NdisFreeBuffer(
	IN	PNDIS_BUFFER	Buffer
	)
{
}


VOID
STDCALL
NdisFreeBufferPool (
	IN	NDIS_HANDLE	PoolHandle
	)
{
}


VOID
STDCALL
NdisFreeMemory (
	IN	PVOID	VirtualAddress,
	IN	UINT	Length,
	IN	UINT	MemoryFlags
	)
{
}


VOID
STDCALL
NdisFreePacket (
	IN	PNDIS_PACKET	Packet
	)
{
}


VOID
STDCALL
NdisFreePacketPool (
	IN	NDIS_HANDLE	PoolHandle
	)
{
}


VOID
STDCALL
NdisFreeSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


VOID
STDCALL
NdisFreeString (
	IN	NDIS_STRING	String
	)
{
}


VOID
STDCALL
NdisGetBufferPhysicalArraySize (
	IN	PNDIS_BUFFER	Buffer,
	OUT	PUINT		ArraySize
	)
{
}


ULONG
STDCALL
NdisGetCacheFillSize (
	VOID
	)
{
	return 0;
}


VOID
STDCALL
NdisGetCurrentProcessorCpuUsage (
	IN OUT	PULONG	pCpuUsage
	)
{
}


VOID
STDCALL
NdisGetCurrentSystemTime (
	IN OUT	PLONGLONG	pSystemTime
	)
{
}


VOID
STDCALL
NdisGetFirstBufferFromPacket (
	IN	PNDIS_PACKET	_Packet,
	OUT	PNDIS_BUFFER	* _FirstBuffer,
	OUT	PVOID		* _FirstBufferVA,
	OUT	PUINT		_FirstBufferLength,
	OUT	PUINT		_TotalBufferLength
	)
{
}


VOID
STDCALL
NdisGetNextBuffer (
	IN	PNDIS_BUFFER	CurrentBuffer,
	OUT	PNDIS_BUFFER	* NextBuffer
	)
{
}


UINT
STDCALL
NdisGetPacketFlags (
	IN	PNDIS_PACKET	Packet
	)
{
}


ULONG
STDCALL
NdisGetPhysicalAddressHigh (
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
{
	return 0;
}


ULONG
STDCALL
NdisGetPhysicalAddressLow (
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
{
	retrun 0;
}


NDIS_STATUS
STDCALL
NdisIMDeInitializeDeviceInstance (
	IN	NDIS_HANDLE	NdisMiniportHandle
	)
{
	return 0;
}


NDIS_STATUS
STDCALL
NdisIMInitializeDeviceInstance (
	IN	NDIS_HANDLE	DriverHandle,
	IN	PNDIS_STRING	DeviceInstance
	)
{
	return 0;
}


ULONG
STDCALL
NdisImmediateReadPciSlotInformation (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		SlotNumber,
	IN	ULONG		Offset,
	IN	PVOID		Buffer,
	IN	ULONG		Length
	)
{
}


VOID
STDCALL
NdisImmediateReadPortUchar (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	OUT	PUCHAR		Data
	)
{
}


VOID
STDCALL
NdisImmediateReadPortUlong (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	OUT	PULONG		Data
	)
{
}


VOID
STDCALL
NdisImmediateReadPortUshort (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	OUT	PUSHORT		Data
	)
{
}


VOID
STDCALL
NdisImmediateReadSharedMemory (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		SharedMemoryAddress,
	OUT	PUCHAR		Buffer,
	IN	ULONG		Length
	)
{
}


ULONG 
STDCALL
NdisImmediateWritePciSlotInformation (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		SlotNumber,
	IN	ULONG		Offset,
	IN	PVOID		Buffer,
	IN	ULONG		Length
	)
{
	return 0;
}


VOID
STDCALL
NdisImmediateWritePortUchar (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	IN	UCHAR		Data
	)
{
}


VOID
STDCALL
NdisImmediateWritePortUlong (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	IN	ULONG		Data
	)
{
}


VOID
STDCALL
NdisImmediateWritePortUshort (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		Port,
	IN	USHORT		Data
	)
{
}


VOID
STDCALL
NdisImmediateWriteSharedMemory (
	IN	NDIS_HANDLE	WrapperConfigurationContext,
	IN	ULONG		SharedMemoryAddress,
	IN	PUCHAR		Buffer,
	IN	ULONG		Length
	)
{
}


NDIS_STATUS
STDCALL
NdisIMQueueMiniportCallback (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	W_MINIPORT_CALLBACK	CallbackRoutine,
	IN	PVOID			CallbackContext
	)
{
}


NDIS_STATUS
STDCALL
NdisIMRegisterLayeredMiniport (
	IN	NDIS_HANDLE			NdisWrapperHandle,
	IN	PNDIS_MINIPORT_CHARACTERISTICS	MiniportCharacteristics,
	IN	UINT				CharacteristicsLength,
	OUT	PNDIS_HANDLE			DriverHandle
	)
{
	return 0;
}


VOID
STDCALL
NdisIMRevertBack (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_HANDLE	SwitchHandle
	)
{
}


BOOLEAN
STDCALL
NdisIMSwitchToMiniport (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	OUT	PNDIS_HANDLE	SwitchHandle
	)
{
	return FALSE;
}


VOID
STDCALL
NdisInitAnsiString (
	IN	OUT PNDIS_ANSI_STRING	DestinationString,
	IN	PCSTR			SourceString
	)
{
}


VOID
STDCALL
NdisInitializeEvent (
	IN	PNDIS_EVENT	Event
	)
{
}


VOID
STDCALL
NdisInitializeListHead (
	IN	PLIST_ENTRY	ListHead
	)
{
}


VOID
STDCALL
NdisInitializeString (
	IN	OUT PNDIS_STRING	DestinationString,
	IN	PUCHAR			SourceString
	)
{
}


VOID
STDCALL
NdisInitUnicodeString (
	IN OUT	PNDIS_STRING	DestinationString,
	IN	PCWSTR		SourceString
	)
{
}


VOID
STDCALL
NdisInterlockedAddUlong (
	IN	PULONG		Addend,
	IN	ULONG		Increment,
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


PLIST_ENTRY
STDCALL
NdisInterlockedInsertHeadList (
	IN	PLIST_ENTRY	ListHead,
	IN	PLIST_ENTRY	ListEntry,
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
	return NULL;
}


PLIST_ENTRY
STDCALL
NdisInterlockedInsertTailList(
	IN	PLIST_ENTRY	ListHead,
	IN	PLIST_ENTRY	ListEntry,
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
	return NULL;
}


PLIST_ENTRY
STDCALL
NdisInterlockedRemoveHeadList (
	IN	PLIST_ENTRY	ListHead,
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
	return NULL;
}


NDIS_STATUS
STDCALL
NdisMAllocateMapRegisters (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	UINT		DmaChannel,
	IN	BOOLEAN		Dma32BitAddresses,
	IN	ULONG		PhysicalMapRegistersNeeded,
	IN	ULONG		MaximumPhysicalMapping
	)
{
	return 0;
}


VOID
STDCALL
NdisMAllocateSharedMemory (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	ULONG			Length,
	IN	BOOLEAN			Cached,
	OUT	PVOID			* VirtualAddress,
	OUT	PNDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
{
}


NDIS_STATUS
STDCALL
NdisMAllocateSharedMemoryAsync (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	ULONG		Length,
	IN	BOOLEAN		Cached,
	IN	PVOID		Context
	)
{
	return 0;
}


VOID
STDCALL
NdisMapFile (
	OUT	PNDIS_STATUS	Status,
	OUT	PVOID		* MappedBuffer,
	IN	NDIS_HANDLE	FileHandle
	)
{
}


VOID
STDCALL
NdisMArcIndicateReceive (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	PUCHAR		HeaderBuffer,
	IN	PUCHAR		DataBuffer,
	IN	UINT		Length
	)
{
}


VOID
STDCALL
NdisMArcIndicateReceiveComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMCancelTimer (
	IN	PNDIS_MINIPORT_TIMER	Timer,
	OUT	PBOOLEAN		TimerCancelled
	)
{
}


VOID
STDCALL
NdisMCloseLog (
	IN	NDIS_HANDLE	LogHandle
	)
{
}


VOID
STDCALL
NdisMCompleteBufferPhysicalMapping (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	PNDIS_BUFFER	Buffer,
	IN	ULONG		PhysicalMapRegister
	)
{
}


VOID
STDCALL
NdisMCompleteDmaTransfer (
	OUT	PNDIS_STATUS	Status,
	IN	PNDIS_HANDLE	MiniportDmaHandle,
	IN	PNDIS_BUFFER	Buffer,
	IN	ULONG		Offset,
	IN	ULONG		Length,
	IN	BOOLEAN		WriteToDevice
	)
{
}


NDIS_STATUS
STDCALL
NdisMCreateLog (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	UINT		Size,
	OUT	PNDIS_HANDLE	LogHandle
	)
{
	return 0;
}


VOID
STDCALL
NdisMDeregisterAdapterShutdownHandler (
	IN	NDIS_HANDLE	MiniportHandle
	)
{
}


VOID
STDCALL
NdisMDeregisterDmaChannel (
	IN	PNDIS_HANDLE	MiniportDmaHandle
	)
{
}


VOID
STDCALL
NdisMDeregisterInterrupt (
	IN	PNDIS_MINIPORT_INTERRUPT	Interrupt
	)
{
}


VOID
STDCALL
NdisMDeregisterIoPortRange (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	UINT		InitialPort,
	IN	UINT		NumberOfPorts,
	IN	PVOID		PortOffset
	)
{
}


VOID
STDCALL
NdisMEthIndicateReceive (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_HANDLE	MiniportReceiveContext,
	IN	PVOID		HeaderBuffer,
	IN	UINT		HeaderBufferSize,
	IN	PVOID		LookaheadBuffer,
	IN	UINT		LookaheadBufferSize,
	IN	UINT		PacketSize
	)
{
}


VOID
STDCALL
NdisMEthIndicateReceiveComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMFddiIndicateReceive (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_HANDLE	MiniportReceiveContext,
	IN	PVOID		HeaderBuffer,
	IN	UINT		HeaderBufferSize,
	IN	PVOID		LookaheadBuffer,
	IN	UINT		LookaheadBufferSize,
	IN	UINT		PacketSize
	)
{
}


VOID
STDCALL
NdisMFddiIndicateReceiveComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMFlushLog (
	IN	NDIS_HANDLE	LogHandle
	)
{
}


VOID
STDCALL
NdisMFreeMapRegisters (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMFreeSharedMemory (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	ULONG			Length,
	IN	BOOLEAN			Cached,
	IN	PVOID			VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
{
}


VOID
STDCALL
NdisMIndicateStatus (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_STATUS	GeneralStatus,
	IN	PVOID		StatusBuffer,
	IN	UINT		StatusBufferSize
	)
{
}


VOID
STDCALL
NdisMIndicateStatusComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMInitializeTimer (
	IN OUT	PNDIS_MINIPORT_TIMER	Timer,
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	PNDIS_TIMER_FUNCTION	TimerFunction,
	IN	PVOID			FunctionContext
	)
{
}


VOID
STDCALL
NdisMInitializeWrapper (
	OUT	PNDIS_HANDLE	NdisWrapperHandle,
	IN	PVOID		SystemSpecific1,
	IN	PVOID		SystemSpecific2,
	IN	PVOID		SystemSpecific3
	)
{
}


NDIS_STATUS
STDCALL
NdisMMapIoSpace (
	OUT	PVOID			* VirtualAddress,
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress,
	IN	UINT			Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
NdisMoveFromMappedMemory (
	OUT	PVOID	Destination,
	IN	PVOID	Source,
	IN	ULONG	Length
	)
{
}


VOID
STDCALL
NdisMoveMappedMemory (
	OUT	PVOID	Destination,
	IN	PVOID	Source,
	IN	ULONG	Length
	)
{
}


VOID
STDCALL
NdisMoveMemory (
	OUT	PVOID	Destination,
	IN	PVOID	Source,
	IN	ULONG	Length
	)
{
}


VOID
STDCALL
NdisMoveToMappedMemory (
	OUT	PVOID	Destination,
	IN	PVOID	Source,
	IN	ULONG	Length
	)
{
}


NDIS_STATUS
STDCALL
NdisMPciAssignResources (
	IN	NDIS_HANDLE		MiniportHandle,
	IN	ULONG			SlotNumber,
	OUT	PNDIS_RESOURCE_LIST	* AssignedResources
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
NdisMQueryAdapterResources (
	OUT	PNDIS_STATUS		Status,
	IN	NDIS_HANDLE		WrapperConfigurationContext,
	OUT	PNDIS_RESOURCE_LIST	ResourceList,
	IN OUT	PUINT			BufferSize
	)
{
}


VOID
STDCALL
NdisMQueryInformationComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_STATUS	Status
	)
{
}


ULONG
STDCALL
NdisMReadDmaCounter (
	IN	NDIS_HANDLE	MiniportDmaHandle
	)
{
	return 0;
}


VOID
STDCALL
NdisMRegisterAdapterShutdownHandler (
	IN	NDIS_HANDLE			MiniportHandle,
	IN	PVOID				ShutdownContext,
	IN	ADAPTER_SHUTDOWN_HANDLER	ShutdownHandler
	)
{
}


NDIS_STATUS
STDCALL
NdisMRegisterDmaChannel (
	OUT	PNDIS_HANDLE		MiniportDmaHandle,
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	UINT			DmaChannel,
	IN	BOOLEAN			Dma32BitAddresses,
	IN	PNDIS_DMA_DESCRIPTION	DmaDescription,
	IN	ULONG			MaximumLength
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


NDIS_STATUS
STDCALL
NdisMRegisterInterrupt (
	OUT	PNDIS_MINIPORT_INTERRUPT	Interrupt,
	IN	NDIS_HANDLE			MiniportAdapterHandle,
	IN	UINT				InterruptVector,
	IN	UINT				InterruptLevel,
	IN	BOOLEAN				RequestIsr,
	IN	BOOLEAN				SharedInterrupt,
	IN	NDIS_INTERRUPT_MODE		InterruptMode
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


NDIS_STATUS
STDCALL
NdisMRegisterIoPortRange (
	OUT	PVOID		* PortOffset,
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	UINT		InitialPort,
	IN	UINT		NumberOfPorts
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


NDIS_STATUS
STDCALL
NdisMRegisterMiniport (
	IN	NDIS_HANDLE			NdisWrapperHandle,
	IN	PNDIS_MINIPORT_CHARACTERISTICS	MiniportCharacteristics,
	IN	UINT				CharacteristicsLength
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
NdisMResetComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_STATUS	Status,
	IN	BOOLEAN		AddressingReset
	)
{
}


VOID
STDCALL
NdisMSendComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	PNDIS_PACKET	Packet,
	IN	NDIS_STATUS	Status
	)
{
}


VOID
STDCALL
NdisMSendResourcesAvailable (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMSetAttributes (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	BOOLEAN			BusMaster,
	IN	NDIS_INTERFACE_TYPE	AdapterType
	)
{
}


VOID
STDCALL
NdisMSetAttributesEx (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	UINT			CheckForHangTimeInSeconds	OPTIONAL,
	IN	ULONG			AttributeFlags,
	IN	NDIS_INTERFACE_TYPE	AdapterType
	)
{
}


VOID
STDCALL
NdisMSetInformationComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_STATUS	Status
	)
{
}


VOID
STDCALL
NdisMSetPeriodicTimer (
	IN	PNDIS_MINIPORT_TIMER	Timer,
	IN	UINT			MillisecondsPeriod
	)
{
}


VOID
STDCALL
NdisMSetTimer (
	IN	PNDIS_MINIPORT_TIMER	Timer,
	IN	UINT			MillisecondsToDelay
	)
{
}


VOID
STDCALL
NdisMSetupDmaTransfer (
	OUT	PNDIS_STATUS	Status,
	IN	PNDIS_HANDLE	MiniportDmaHandle,
	IN	PNDIS_BUFFER	Buffer,
	IN	ULONG		Offset,
	IN	ULONG		Length,
	IN	BOOLEAN		WriteToDevice
	)
{
}


VOID
STDCALL
NdisMSleep (
	IN	ULONG	MicrosecondsToSleep
	)
{
}


VOID
STDCALL
NdisMStartBufferPhysicalMapping (
	IN	NDIS_HANDLE			MiniportAdapterHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG				PhysicalMapRegister,
	IN	BOOLEAN				WriteToDevice,
	OUT	PNDIS_PHYSICAL_ADDRESS_UNIT	PhysicalAddressArray,
	OUT	PUINT				ArraySize
	)
{
}


BOOLEAN
STDCALL
NdisMSynchronizeWithInterrupt (
	IN	PNDIS_MINIPORT_INTERRUPT	Interrupt,
	IN	PVOID				SynchronizeFunction,
	IN	PVOID				SynchronizeContext
	)
{
}


VOID
STDCALL
NdisMTrIndicateReceive (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	NDIS_HANDLE	MiniportReceiveContext,
	IN	PVOID		HeaderBuffer,
	IN	UINT		HeaderBufferSize,
	IN	PVOID		LookaheadBuffer,
	IN	UINT		LookaheadBufferSize,
        IN	UINT		PacketSize
	)
{
}


VOID
STDCALL
NdisMTrIndicateReceiveComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
}


VOID
STDCALL
NdisMTransferDataComplete (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	PNDIS_PACKET	Packet,
	IN	NDIS_STATUS	Status,
	IN	UINT		BytesTransferred
	)
{
}


VOID
STDCALL
NdisMUnmapIoSpace (
	IN	NDIS_HANDLE	MiniportAdapterHandle,
	IN	PVOID		VirtualAddress,
	IN	UINT		Length
	)
{
}


VOID
STDCALL
NdisMUpdateSharedMemory (
	IN	NDIS_HANDLE		MiniportAdapterHandle,
	IN	ULONG			Length,
	IN	PVOID			VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
{
}


NDIS_STATUS
STDCALL
NdisMWriteLogData (
	IN	NDIS_HANDLE	LogHandle,
	IN	PVOID		LogBuffer,
	IN	UINT		LogBufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
NdisMapFile
NdisMapIoSpace
NdisOpenAdapter
*/


VOID
STDCALL
NdisOpenConfiguration (
	OUT	PNDIS_STATUS	Status,
	OUT	PNDIS_HANDLE	ConfigurationHandle,
	IN	NDIS_HANDLE	WrapperConfigurationContext
	)
{
}


/*
NdisOpenConfigurationKeyByIndex
NdisOpenConfigurationKeyByName
*/


VOID
STDCALL
NdisOpenFile (
	OUT	PNDIS_STATUS		Status,
	OUT	PNDIS_HANDLE		FileHandle,
	OUT	PUINT			FileLength,
	IN	PNDIS_STRING		FileName,
	IN	NDIS_PHYSICAL_ADDRESS	HighestAcceptableAddress
	)
{
}


/*
NdisOpenGlobalConfiguration
NdisOpenProtocolConfiguration
NdisOverrideBusNumber
NdisPciAssignResources
*/


VOID
STDCALL
NdisQueryBuffer (
	IN	PNDIS_BUFFER	Buffer,
	OUT	PVOID		* VirtualAddress	OPTIONAL,
	OUT	PUINT		Length
	)
{
}


VOID
STDCALL
NdisQueryBufferOffset (
	IN	PNDIS_BUFFER	Buffer,
	OUT	PUINT		Offset,
	OUT	PUINT		Length
	)
{
}


NDIS_STATUS
STDCALL
NdisQueryMapRegisterCount (
	IN	NDIS_INTERFACE_TYPE	BusType,
	OUT	PUINT			MapRegisterCount
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
NdisQueryReceiveInformation
NdisReadBindingInformation
*/


VOID
STDCALL
NdisReadConfiguration (
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_CONFIGURATION_PARAMETER	* ParameterValue,
	IN	NDIS_HANDLE			ConfigurationHandle,
	IN	PNDIS_STRING			Keyword,
	IN	NDIS_PARAMETER_TYPE		ParameterType
	)
{
}


VOID
STDCALL
NdisReadEisaSlotInformation (
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE			WrapperConfigurationContext,
	OUT	PUINT				SlotNumber,
	OUT	PNDIS_EISA_FUNCTION_INFORMATION	EisaData
	)
{
}


VOID
STDCALL
NdisReadEisaSlotInformationEx (
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE			WrapperConfigurationContext,
	OUT	PUINT				SlotNumber,
	OUT	PNDIS_EISA_FUNCTION_INFORMATION	* EisaData,
	OUT	PUINT				NumberOfFunctions
	)
{
}


VOID
STDCALL
NdisReadMcaPosInformation (
	OUT	PNDIS_STATUS		Status,
	IN	NDIS_HANDLE		WrapperConfigurationContext,
	IN	PUINT			ChannelNumber,
	OUT	PNDIS_MCA_POS_DATA	McaData
	)
{
}


VOID
STDCALL
NdisReadNetworkAddress (
	OUT	PNDIS_STATUS	Status,
	OUT	PVOID		* NetworkAddress,
	OUT	PUINT		NetworkAddressLength,
	IN	NDIS_HANDLE	ConfigurationHandle
	)
{
}


ULONG
STDCALL
NdisReadPciSlotInformation (
	IN	NDIS_HANDLE	NdisAdapterHandle,
	IN	ULONG		SlotNumber,
	IN	ULONG		Offset,
	IN	PVOID		Buffer,
	IN	ULONG		Length
	)
{
	return 0;
}


/*
NdisRegisterAdapter
NdisRegisterAdapterShutdownHandler
NdisRegisterMac
NdisRegisterProtocol
NdisRegisterTdiCallBack
NdisReleaseAdapterResources
*/


VOID
STDCALL
NdisReleaseSpinLock (
	IN	PNDIS_SPIN_LOCK	SpinLock
	)
{
}


/*
NdisRemoveInterrupt
NdisRequest
NdisReset
*/


VOID
STDCALL
NdisResetEvent (
	IN	PNDIS_EVENT	Event
	)
{
}


/*
NdisReturnPackets
NdisScheduleWorkItem
NdisSend
NdisSendPackets
*/


VOID
STDCALL
NdisSetEvent (
	IN	PNDIS_EVENT	Event
	)
{
}


/*
NdisSetProtocolFilter
NdisSetTimer
NdisSetupDmaTransfer
NdisSystemProcessorCount
*/


VOID
STDCALL
NdisTerminateWrapper (
	IN	NDIS_HANDLE	NdisWrapperHandle,
	IN	PVOID		SystemSpecific
	)
{
}


/*
NdisTransferData
*/


VOID
STDCALL
NdisUnchainBufferAtBack (
	IN OUT	PNDIS_PACKET	Packet,
	OUT	PNDIS_BUFFER	* Buffer
	)
{
}


VOID
STDCALL
NdisUnchainBufferAtFront (
	IN OUT	PNDIS_PACKET	Packet,
        OUT	PNDIS_BUFFER	* Buffer
	)
{
}


NDIS_STATUS
STDCALL
NdisUnicodeStringToAnsiString (
	IN	OUT PNDIS_ANSI_STRING	DestinationString,
	IN	PNDIS_STRING		SourceString
        )
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
NdisUnmapFile (
	IN	NDIS_HANDLE	FileHandle
        )
{
}


/*
NdisUpcaseUnicodeString
NdisUpdateSharedMemory
*/


BOOLEAN
STDCALL
NdisWaitEvent (
	IN	PNDIS_EVENT	Event,
	IN	UINT		MsToWait
	)
{
	return FALSE;
}


VOID
STDCALL
NdisWriteConfiguration (
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE			WrapperConfigurationContext,
	IN	PNDIS_STRING			Keyword,
	IN	PNDIS_CONFIGURATION_PARAMETER	* ParameterValue
	)
{
}


VOID
STDCALL
NdisWriteErrorLogEntry (
	IN	NDIS_HANDLE	NdisAdapterHandle,
	IN	NDIS_ERROR_CODE	ErrorCode,
	IN	ULONG		NumberOfErrorValues,
	IN	ULONG		...
	)
{
}


/*
NdisWriteEventLogEntry
*/


ULONG
STDCALL
NdisWritePciSlotInformation (
	IN	NDIS_HANDLE	NdisAdapterHandle,
	IN	ULONG		SlotNumber,
	IN	ULONG		Offset,
	IN	PVOID		Buffer,
	IN	ULONG		Length
	)
{
	return 0;
}


/* EOF */
