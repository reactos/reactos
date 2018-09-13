#if !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

typedef
BOOLEAN
(*PNDIS_INTERRUPT_SERVICE)(
	IN	PVOID					InterruptContext
	);

typedef
VOID
(*PNDIS_DEFERRED_PROCESSING)(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					InterruptContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);


typedef struct _NDIS_INTERRUPT
{
	PKINTERRUPT					InterruptObject;
	KSPIN_LOCK					DpcCountLock;
	PNDIS_INTERRUPT_SERVICE		MacIsr;	 		// Pointer to Mac ISR routine
	PNDIS_DEFERRED_PROCESSING	MacDpc;				// Pointer to Mac DPC routine
	KDPC						InterruptDpc;
	PVOID						InterruptContext;	// Pointer to context for calling
													// adapters ISR and DPC.
	UCHAR						DpcCount;
	BOOLEAN						Removing;			// TRUE if removing interrupt

	//
	// This is used to tell when all the Dpcs for the adapter are completed.
	//
	KEVENT						DpcsCompletedEvent;

} NDIS_INTERRUPT, *PNDIS_INTERRUPT;

//
// Ndis Adapter Information
//
typedef
NDIS_STATUS
(*PNDIS_ACTIVATE_CALLBACK)(
	IN	NDIS_HANDLE				NdisAdatperHandle,
	IN	NDIS_HANDLE				MacAdapterContext,
	IN	ULONG					DmaChannel
	);

typedef struct _NDIS_PORT_DESCRIPTOR
{
	ULONG						InitialPort;
	ULONG						NumberOfPorts;
	PVOID *						PortOffset;
} NDIS_PORT_DESCRIPTOR, *PNDIS_PORT_DESCRIPTOR;

typedef struct _NDIS_ADAPTER_INFORMATION
{
	ULONG						DmaChannel;
	BOOLEAN						Master;
	BOOLEAN						Dma32BitAddresses;
	PNDIS_ACTIVATE_CALLBACK		ActivateCallback;
	NDIS_INTERFACE_TYPE			AdapterType;
	ULONG						PhysicalMapRegistersNeeded;
	ULONG						MaximumPhysicalMapping;
	ULONG						NumberOfPortDescriptors;
	NDIS_PORT_DESCRIPTOR		PortDescriptors[1];	// as many as needed
} NDIS_ADAPTER_INFORMATION, *PNDIS_ADAPTER_INFORMATION;

//
// Function types for NDIS_MAC_CHARACTERISTICS
//
typedef
NDIS_STATUS
(*OPEN_ADAPTER_HANDLER)(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT NDIS_HANDLE *			MacBindingHandle,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	NDIS_HANDLE				MacAdapterContext,
	IN	UINT					OpenOptions,
	IN	PSTRING					AddressingInformation OPTIONAL
	);

typedef
NDIS_STATUS
(*CLOSE_ADAPTER_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle
	);

typedef
NDIS_STATUS
(*WAN_TRANSFER_DATA_HANDLER)(
	VOID
	);

typedef
NDIS_STATUS
(*QUERY_GLOBAL_STATISTICS_HANDLER)(
	IN	NDIS_HANDLE				MacAdapterContext,
	IN	PNDIS_REQUEST			NdisRequest
	);

typedef
VOID
(*UNLOAD_MAC_HANDLER)(
	IN	NDIS_HANDLE				MacMacContext
	);

typedef
NDIS_STATUS
(*ADD_ADAPTER_HANDLER)(
	IN	NDIS_HANDLE				MacMacContext,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	PNDIS_STRING			AdapterName
	);

typedef
VOID
(*REMOVE_ADAPTER_HANDLER)(
	IN	NDIS_HANDLE				MacAdapterContext
	);

#endif // !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

//
// The following handlers are used in the OPEN_BLOCK
//
typedef
NDIS_STATUS
(*WAN_SEND_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	NDIS_HANDLE				LinkHandle,
	IN	PVOID					Packet
	);

typedef
NDIS_STATUS
(*SEND_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet
	);

typedef
NDIS_STATUS
(*TRANSFER_DATA_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer,
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred
	);

typedef
NDIS_STATUS
(*RESET_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle
	);

typedef
NDIS_STATUS
(*REQUEST_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_REQUEST			NdisRequest
	);

//
// NDIS 4.0 extension - however available for miniports only
//
typedef
VOID
(*SEND_PACKETS_HANDLER)(
	IN NDIS_HANDLE				MiniportAdapterContext,
	IN PPNDIS_PACKET			PacketArray,
	IN UINT						NumberOfPackets
	);


#if !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

typedef struct _NDIS_MAC_CHARACTERISTICS
{
	UCHAR						MajorNdisVersion;
	UCHAR						MinorNdisVersion;
	USHORT						Filler;
	UINT						Reserved;
	OPEN_ADAPTER_HANDLER		OpenAdapterHandler;
	CLOSE_ADAPTER_HANDLER		CloseAdapterHandler;
	SEND_HANDLER				SendHandler;
	TRANSFER_DATA_HANDLER		TransferDataHandler;
	RESET_HANDLER				ResetHandler;
	REQUEST_HANDLER				RequestHandler;
	QUERY_GLOBAL_STATISTICS_HANDLER QueryGlobalStatisticsHandler;
	UNLOAD_MAC_HANDLER			UnloadMacHandler;
	ADD_ADAPTER_HANDLER			AddAdapterHandler;
	REMOVE_ADAPTER_HANDLER		RemoveAdapterHandler;
	NDIS_STRING					Name;

} NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;

typedef	NDIS_MAC_CHARACTERISTICS		NDIS_WAN_MAC_CHARACTERISTICS;
typedef	NDIS_WAN_MAC_CHARACTERISTICS *	PNDIS_WAN_MAC_CHARACTERISTICS;

//
// one of these per adapter registered on a MAC
//
struct _NDIS_ADAPTER_BLOCK
{
	PDEVICE_OBJECT				DeviceObject;		// created by NdisRegisterAdapter
	PNDIS_MAC_BLOCK				MacHandle;			// pointer to our MAC block
	NDIS_HANDLE					MacAdapterContext;	// context when calling MacOpenAdapter
	NDIS_STRING					AdapterName;		// how NdisOpenAdapter refers to us
	PNDIS_OPEN_BLOCK			OpenQueue;			// queue of opens for this adapter
	PNDIS_ADAPTER_BLOCK			NextAdapter;		// used by MAC's AdapterQueue
	REFERENCE					Ref;				// contains spinlock for OpenQueue
	PVOID						BusDataContext;		// context to pass to bus driver when
													// reading or writing config space
	BOOLEAN						BeingRemoved;		// TRUE if adapter is being removed
	UCHAR						Flags;

	//
	// Resource information
	//
	PCM_RESOURCE_LIST			Resources;

	PNDIS_STRING				pAdapterInstanceName;

	//
	// Wrapper context.
	//
	PVOID						WrapperContext;

	//
	// contains adapter information
	//
	ULONG						BusNumber;
	NDIS_INTERFACE_TYPE			BusType;
	ULONG						ChannelNumber;
	NDIS_INTERFACE_TYPE			AdapterType;
	BOOLEAN						Master;
	UCHAR						AssignedProcessor;
	ULONG						PhysicalMapRegistersNeeded;
	ULONG						MaximumPhysicalMapping;
	ULONG						InitialPort;
	ULONG						NumberOfPorts;

	//
	// Holds the mapping for ports, if needed.
	//
	PUCHAR						InitialPortMapping;

	//
	// TRUE if InitialPortMapping was mapped with NdisMapIoSpace.
	//
	BOOLEAN						InitialPortMapped;

	//
	// This is the offset added to the port passed to NdisXXXPort to
	// get to the real value to be passed to the NDIS_XXX_PORT macros.
	// It equals InitialPortMapping - InitialPort; that is, the
	// mapped "address" of port 0, even if we didn't actually
	// map port 0.
	//
	PUCHAR						PortOffset;

	//
	// Holds the map registers for this adapter.
	//
	PMAP_REGISTER_ENTRY			MapRegisters;

	//
	// These two are used temporarily while allocating
	// the map registers.
	//
	KEVENT						AllocationEvent;
	UINT						CurrentMapRegister;
	PADAPTER_OBJECT				SystemAdapterObject;

#if defined(NDIS_WRAPPER)
	//
	// Store information here to track adapters
	//
	ULONG						BusId;
	ULONG						SlotNumber;

	//
	// Needed for PnP. Upcased version. The buffer is allocated as part of the
	// NDIS_ADAPTER_BLOCK itself.
	//
	NDIS_STRING					BaseName;

	//
	//	PNPPM
	//
	PDEVICE_OBJECT				PhysicalDeviceObject;
	PDEVICE_OBJECT				NextDeviceObject;
	PCM_RESOURCE_LIST			AllocatedResources;
	PCM_RESOURCE_LIST			AllocatedResourcesTranslated;
	
	//
	// this event will be set at the end of adapter initialization
	//
	NDIS_EVENT					OpenReadyEvent;

	//
	// current PnP state of the device, ex. started, stopped, query_removed, etc.
	//
	NDIS_PNP_DEVICE_STATE		PnPDeviceState;
	
	//
	// Handlers to Write/Read Bus data
	//
	PGET_SET_DEVICE_DATA 		SetBusData;
	PGET_SET_DEVICE_DATA 		GetBusData;
	
	POID_LIST					OidList;
	
	//
	// flag to specify PnP capabilities of the device. we need this to fail query_stop
	// query_remove or suspend request if the device can not handle it
	//
	ULONG						PnPCapabilities;
#endif // NDIS_WRAPPER defined
};

#endif // !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

//
// one of these per open on an adapter/protocol
//
struct _NDIS_OPEN_BLOCK
{
	PNDIS_MAC_BLOCK				MacHandle;			// pointer to our MAC
	NDIS_HANDLE					MacBindingHandle;	// context when calling MacXX funcs
	PNDIS_ADAPTER_BLOCK			AdapterHandle;		// pointer to our adapter
	PNDIS_PROTOCOL_BLOCK		ProtocolHandle;		// pointer to our protocol
	NDIS_HANDLE					ProtocolBindingContext;// context when calling ProtXX funcs
	PNDIS_OPEN_BLOCK			AdapterNextOpen;	// used by adapter's OpenQueue
	PNDIS_OPEN_BLOCK			ProtocolNextOpen;	// used by protocol's OpenQueue
	PNDIS_OPEN_BLOCK			NextGlobalOpen;
	BOOLEAN						Closing;			// TRUE when removing this struct
	BOOLEAN						Unbinding;			// TRUE when starting to unbind the adapter
	BOOLEAN						NoProtRsvdOnRcvPkt;	// Reflect the protocol_options
	BOOLEAN						ProcessingOpens;
	PNDIS_STRING				BindDeviceName;
	KSPIN_LOCK					SpinLock;			// guards Closing
	PNDIS_STRING				RootDeviceName;

	//
	// These are optimizations for getting to MAC routines.	They are not
	// necessary, but are here to save a dereference through the MAC block.
	//
	union
	{
		SEND_HANDLER			SendHandler;
		WAN_SEND_HANDLER		WanSendHandler;
	};
	TRANSFER_DATA_HANDLER		TransferDataHandler;

	//
	// These are optimizations for getting to PROTOCOL routines.  They are not
	// necessary, but are here to save a dereference through the PROTOCOL block.
	//
	SEND_COMPLETE_HANDLER		SendCompleteHandler;
	TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
	RECEIVE_HANDLER				ReceiveHandler;
	RECEIVE_COMPLETE_HANDLER	ReceiveCompleteHandler;

	//
	// Extentions to the OPEN_BLOCK since Product 1.
	//
	union
	{
		RECEIVE_HANDLER			PostNt31ReceiveHandler;
		WAN_RECEIVE_HANDLER		WanReceiveHandler;
	};
	RECEIVE_COMPLETE_HANDLER	PostNt31ReceiveCompleteHandler;

	//
	// NDIS 4.0 extensions
	//
	RECEIVE_PACKET_HANDLER		ReceivePacketHandler;
	SEND_PACKETS_HANDLER		SendPacketsHandler;

	//
	// More NDIS 3.0 Cached Handlers
	//
	RESET_HANDLER				ResetHandler;
	REQUEST_HANDLER				RequestHandler;
	RESET_COMPLETE_HANDLER		ResetCompleteHandler;
	STATUS_HANDLER		 		StatusHandler;
	STATUS_COMPLETE_HANDLER		StatusCompleteHandler;
	REQUEST_COMPLETE_HANDLER	RequestCompleteHandler;
};


EXPORT
VOID
NdisInitializeTimer(
	IN OUT PNDIS_TIMER			Timer,
	IN	PNDIS_TIMER_FUNCTION	TimerFunction,
	IN	PVOID					FunctionContext
	);

VOID
NdisCancelTimer(
	IN	PNDIS_TIMER				Timer,
	OUT PBOOLEAN				TimerCancelled
	);

#if !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

//
// Shared memory
//

EXPORT
VOID
NdisAllocateSharedMemory(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Length,
	IN	BOOLEAN 				Cached,
	OUT PVOID *					VirtualAddress,
	OUT PNDIS_PHYSICAL_ADDRESS	PhysicalAddress
	);

EXPORT
VOID
NdisFreeSharedMemory(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Length,
	IN	BOOLEAN					Cached,
	IN	PVOID					VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	);

//
// Requests Used by MAC Drivers
//

EXPORT
VOID
NdisRegisterMac(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			NdisMacHandle,
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	NDIS_HANDLE				MacMacContext,
	IN	PNDIS_MAC_CHARACTERISTICS MacCharacteristics,
	IN	UINT					CharacteristicsLength
	);

EXPORT
VOID
NdisDeregisterMac(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisMacHandle
	);


EXPORT
NDIS_STATUS
NdisRegisterAdapter(
	OUT PNDIS_HANDLE			NdisAdapterHandle,
	IN	NDIS_HANDLE				NdisMacHandle,
	IN	NDIS_HANDLE				MacAdapterContext,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	PNDIS_STRING			AdapterName,
	IN	PVOID					AdapterInformation
	);

EXPORT
NDIS_STATUS
NdisDeregisterAdapter(
	IN	NDIS_HANDLE				NdisAdapterHandle
	);

EXPORT
VOID
NdisRegisterAdapterShutdownHandler(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PVOID					ShutdownContext,
	IN	ADAPTER_SHUTDOWN_HANDLER ShutdownHandler
	);

EXPORT
VOID
NdisDeregisterAdapterShutdownHandler(
	IN	NDIS_HANDLE				NdisAdapterHandle
	);

EXPORT
VOID
NdisReleaseAdapterResources(
	IN	NDIS_HANDLE				NdisAdapterHandle
	);

EXPORT
VOID
NdisCompleteOpenAdapter(
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
	);


EXPORT
VOID
NdisCompleteCloseAdapter(
	IN	NDIS_HANDLE NdisBindingContext,
	IN	NDIS_STATUS Status
	);


/*++

VOID
NdisCompleteSend(
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status
	);

--*/
#define NdisCompleteSend(NdisBindingContext, Packet, Status)				\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->SendCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,	\
		(Packet),															\
		(Status));															\
}


#define NdisWanSendComplete(NdisBindingContext, WanPacket, Status)			\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->SendCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,	\
		(PNDIS_PACKET)(WanPacket),											\
		(Status));															\
}

/*++

VOID
NdisCompleteTransferData(
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
	);

--*/
#define NdisCompleteTransferData(NdisBindingContext,						\
								 Packet,									\
								 Status,									\
								 BytesTransferred)							\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->TransferDataCompleteHandler)(\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,	\
		(Packet),															\
		(Status),															\
		(BytesTransferred));												\
}

/*++

VOID
NdisCompleteReset(
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	NDIS_STATUS 			Status
	);

--*/
#define NdisCompleteReset(NdisBindingContext, Status)						\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ResetCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,	\
		Status);															\
}


/*++

VOID
NdisCompleteRequest(
	IN NDIS_HANDLE				NdisBindingContext,
	IN PNDIS_REQUEST			NdisRequest,
	IN NDIS_STATUS				Status
	);

--*/
#define NdisCompleteRequest(NdisBindingContext, NdisRequest, Status)		\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->RequestCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,	\
		NdisRequest,														\
		Status);															\
}

/*++

VOID
NdisIndicateReceive(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingContext,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

--*/
#define NdisIndicateReceive(Status,											\
							NdisBindingContext,								\
							MacReceiveContext,								\
							HeaderBuffer,									\
							HeaderBufferSize,								\
							LookaheadBuffer,								\
							LookaheadBufferSize,							\
							PacketSize)										\
{																			\
	KIRQL oldIrql;															\
																			\
	KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );								\
	*(Status) =																\
		(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->PostNt31ReceiveHandler)( \
			((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,\
			(MacReceiveContext),											\
			(HeaderBuffer),													\
			(HeaderBufferSize),												\
			(LookaheadBuffer),												\
			(LookaheadBufferSize),											\
			(PacketSize));													\
																			\
	KeLowerIrql( oldIrql );													\
}

#define NdisWanIndicateReceive(Status,												\
							   NdisBindingContext,									\
							   NdisLinkContext,										\
							   Packet,												\
							   PacketSize)											\
{																					\
	*(Status) = (((PNDIS_OPEN_BLOCK)(NdisBindingContext))->WanReceiveHandler)(		\
			(NdisLinkContext),														\
			(Packet),																\
			(PacketSize));															\
}

//
// Used by the filter packages for indicating receives
//

#define FilterIndicateReceive(Status,												\
							  NdisBindingContext,									\
							  MacReceiveContext,									\
							  HeaderBuffer,											\
							  HeaderBufferSize,										\
							  LookaheadBuffer,										\
							  LookaheadBufferSize,									\
							  PacketSize)											\
{\
	*(Status) =																		\
		(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->PostNt31ReceiveHandler)(			\
			((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,		\
			(MacReceiveContext),													\
			(HeaderBuffer),															\
			(HeaderBufferSize),														\
			(LookaheadBuffer),														\
			(LookaheadBufferSize),													\
			(PacketSize));															\
}


/*++

VOID
NdisIndicateReceiveComplete(
	IN	NDIS_HANDLE				NdisBindingContext
	);

--*/
#define NdisIndicateReceiveComplete(NdisBindingContext)								\
{																					\
	KIRQL oldIrql;																	\
																					\
	KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );										\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->PostNt31ReceiveCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext);			\
	KeLowerIrql( oldIrql );															\
}

#define NdisWanIndicateReceiveComplete(NdisBindingContext,							\
									   NdisLinkContext)								\
{																					\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->PostNt31ReceiveCompleteHandler)(		\
									(NdisLinkContext));								\
}

//
// Used by the filter packages for indicating receive completion
//

#define FilterIndicateReceiveComplete(NdisBindingContext)							\
{																					\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->PostNt31ReceiveCompleteHandler)(		\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext);			\
}

/*++

VOID
NdisIndicateStatus(
	IN NDIS_HANDLE				NdisBindingContext,
	IN NDIS_STATUS				GeneralStatus,
	IN PVOID					StatusBuffer,
	IN UINT						StatusBufferSize
	);

--*/
#define NdisIndicateStatus(NdisBindingContext,										\
						   GeneralStatus,											\
						   StatusBuffer,											\
						   StatusBufferSize)										\
{																					\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->StatusHandler)(						\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext,			\
		(GeneralStatus),															\
		(StatusBuffer),																\
		(StatusBufferSize));														\
}


/*++

VOID
NdisIndicateStatusComplete(
	IN	NDIS_HANDLE				NdisBindingContext
	);

--*/
#define NdisIndicateStatusComplete(NdisBindingContext)								\
{																					\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->StatusCompleteHandler)(				\
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext);			\
}

EXPORT
VOID
NdisCompleteQueryStatistics(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status
	);

//
// Operating System Requests
//

EXPORT
VOID
NdisMapIoSpace(
	OUT PNDIS_STATUS			Status,
	OUT PVOID *					VirtualAddress,
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	NDIS_PHYSICAL_ADDRESS 	PhysicalAddress,
	IN	UINT					Length
	);

#if defined(_ALPHA_)

/*++
VOID
NdisUnmapIoSpace(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PVOID					VirtualAddress,
	IN	UINT					Length
	)
--*/
#define NdisUnmapIoSpace(Handle,VirtualAddress,Length)

#else

/*++
VOID
NdisUnmapIoSpace(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PVOID					VirtualAddress,
	IN	UINT					Length
	)
--*/
#define NdisUnmapIoSpace(Handle,VirtualAddress,Length)	MmUnmapIoSpace((VirtualAddress), (Length));

#endif

EXPORT
VOID
NdisInitializeInterrupt(
	OUT PNDIS_STATUS			Status,
	IN OUT PNDIS_INTERRUPT		Interrupt,
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PNDIS_INTERRUPT_SERVICE	InterruptServiceRoutine,
	IN	PVOID					InterruptContext,
	IN	PNDIS_DEFERRED_PROCESSING DeferredProcessingRoutine,
	IN	UINT					InterruptVector,
	IN	UINT					InterruptLevel,
	IN	BOOLEAN					SharedInterrupt,
	IN	NDIS_INTERRUPT_MODE		InterruptMode
	);

EXPORT
VOID
NdisRemoveInterrupt(
	IN	PNDIS_INTERRUPT			Interrupt
	);

/*++

BOOLEAN
NdisSynchronizeWithInterrupt(
	IN	PNDIS_INTERRUPT			Interrupt,
	IN	PVOID					SynchronizeFunction,
	IN	PVOID					SynchronizeContext
	)

--*/

#define NdisSynchronizeWithInterrupt(Interrupt,Function,Context)					\
			KeSynchronizeExecution((Interrupt)->InterruptObject,					\
								   (PKSYNCHRONIZE_ROUTINE)Function,					\
								   Context)

//
// Physical Mapping
//

/*++

VOID
NdisStartBufferPhysicalMapping(
	IN	NDIS_HANDLE NdisAdapterHandle,
	IN	PNDIS_BUFFER Buffer,
	IN	ULONG PhysicalMapRegister,
	IN	BOOLEAN WriteToDevice,
	OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
	OUT	PUINT ArraySize
 );

--*/
#define NdisStartBufferPhysicalMapping(_NdisAdapterHandle,						\
									   _Buffer,									\
									   _PhysicalMapRegister,					\
									   _Write,									\
									   _PhysicalAddressArray,					\
									   _ArraySize)								\
{																				\
	PNDIS_ADAPTER_BLOCK _AdaptP = (PNDIS_ADAPTER_BLOCK)(_NdisAdapterHandle);	\
	PHYSICAL_ADDRESS _LogicalAddress;											\
	PUCHAR _VirtualAddress;														\
	ULONG _LengthRemaining;														\
	ULONG _LengthMapped;														\
	UINT _CurrentArrayLocation;													\
																				\
	_VirtualAddress = (PUCHAR)MmGetMdlVirtualAddress(_Buffer);					\
	_LengthRemaining = MmGetMdlByteCount(_Buffer);								\
	_CurrentArrayLocation = 0;													\
	while (_LengthRemaining > 0)												\
	{																			\
		_LengthMapped = _LengthRemaining;										\
		_LogicalAddress = IoMapTransfer(_AdaptP->SystemAdapterObject,			\
										_Buffer,								\
										_AdaptP->MapRegisters[_PhysicalMapRegister].MapRegister,\
										_VirtualAddress,						\
										&_LengthMapped,							\
										_Write);								\
		(_PhysicalAddressArray)[_CurrentArrayLocation].PhysicalAddress = _LogicalAddress;\
		(_PhysicalAddressArray)[_CurrentArrayLocation].Length = _LengthMapped;	\
		_LengthRemaining -= _LengthMapped;										\
		_VirtualAddress += _LengthMapped;										\
		++_CurrentArrayLocation;												\
	}																			\
	_AdaptP->MapRegisters[_PhysicalMapRegister].WriteToDevice = (_Write);		\
	*(_ArraySize) = _CurrentArrayLocation;										\
}


/*++

VOID
NdisCompleteBufferPhysicalMapping(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					PhysicalMapRegister
	);

--*/
#define NdisCompleteBufferPhysicalMapping(_NdisAdapterHandle,					\
										  _Buffer,								\
										  _PhysicalMapRegister					\
	)																			\
{																				\
	PNDIS_ADAPTER_BLOCK _AdaptP = (PNDIS_ADAPTER_BLOCK)(_NdisAdapterHandle);	\
	IoFlushAdapterBuffers(_AdaptP->SystemAdapterObject,							\
						  _Buffer,												\
						  _AdaptP->MapRegisters[_PhysicalMapRegister].MapRegister,\
						  MmGetMdlVirtualAddress(_Buffer),						\
						  MmGetMdlByteCount(_Buffer),							\
						  _AdaptP->MapRegisters[_PhysicalMapRegister].WriteToDevice);\
}

#endif // !defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

