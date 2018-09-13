//
// Function types for NDIS_PROTOCOL_CHARACTERISTICS
//

typedef
VOID
(*OPEN_ADAPTER_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
	);

typedef
VOID
(*CLOSE_ADAPTER_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*RESET_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*REQUEST_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*STATUS_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

typedef
VOID
(*STATUS_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

typedef
VOID
(*SEND_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*WAN_SEND_COMPLETE_HANDLER) (
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_WAN_PACKET		Packet,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*TRANSFER_DATA_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
	);

typedef
VOID
(*WAN_TRANSFER_DATA_COMPLETE_HANDLER)(
	VOID
    );

typedef
NDIS_STATUS
(*RECEIVE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookAheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

typedef
NDIS_STATUS
(*WAN_RECEIVE_HANDLER)(
	IN	NDIS_HANDLE				NdisLinkHandle,
	IN	PUCHAR					Packet,
	IN	ULONG					PacketSize
    );

typedef
VOID
(*RECEIVE_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

//
// Protocol characteristics for down-level NDIS 3.0 protocols
//
typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS
{
	UCHAR							MajorNdisVersion;
	UCHAR							MinorNdisVersion;
	USHORT							Filler;
	union
	{
		UINT						Reserved;
		UINT						Flags;
	};
	OPEN_ADAPTER_COMPLETE_HANDLER	OpenAdapterCompleteHandler;
	CLOSE_ADAPTER_COMPLETE_HANDLER	CloseAdapterCompleteHandler;
	union
	{
		SEND_COMPLETE_HANDLER		SendCompleteHandler;
		WAN_SEND_COMPLETE_HANDLER	WanSendCompleteHandler;
	};
	union
	{
	 TRANSFER_DATA_COMPLETE_HANDLER	TransferDataCompleteHandler;
	 WAN_TRANSFER_DATA_COMPLETE_HANDLER WanTransferDataCompleteHandler;
	};

	RESET_COMPLETE_HANDLER			ResetCompleteHandler;
	REQUEST_COMPLETE_HANDLER		RequestCompleteHandler;
	union
	{
		RECEIVE_HANDLER				ReceiveHandler;
		WAN_RECEIVE_HANDLER			WanReceiveHandler;
	};
	RECEIVE_COMPLETE_HANDLER		ReceiveCompleteHandler;
	STATUS_HANDLER					StatusHandler;
	STATUS_COMPLETE_HANDLER			StatusCompleteHandler;
	NDIS_STRING						Name;
} NDIS30_PROTOCOL_CHARACTERISTICS;

//
// Function types extensions for NDIS 4.0 Protocols
//
typedef
INT
(*RECEIVE_PACKET_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet
	);

typedef
VOID
(*BIND_HANDLER)(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			DeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
	);

typedef
VOID
(*UNBIND_HANDLER)(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
	);

typedef
NDIS_STATUS
(*PNP_EVENT_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			NetPnPEvent
	);

typedef
VOID
(*UNLOAD_PROTOCOL_HANDLER)(
	VOID
	);

//
// Protocol characteristics for NDIS 4.0 protocols
//
typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS30_PROTOCOL_CHARACTERISTICS	Ndis30Chars;
#else
	NDIS30_PROTOCOL_CHARACTERISTICS;
#endif

	//
	// Start of NDIS 4.0 extensions.
	//
	RECEIVE_PACKET_HANDLER			ReceivePacketHandler;

	//
	// PnP protocol entry-points
	//
	BIND_HANDLER					BindAdapterHandler;
	UNBIND_HANDLER					UnbindAdapterHandler;
	PNP_EVENT_HANDLER				PnPEventHandler;
	UNLOAD_PROTOCOL_HANDLER			UnloadHandler;

} NDIS40_PROTOCOL_CHARACTERISTICS;


//
// Protocol (5.0) handler proto-types - used by clients as well as call manager modules
//
typedef
VOID
(*CO_SEND_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	);

typedef
VOID
(*CO_STATUS_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

typedef
UINT
(*CO_RECEIVE_PACKET_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	);

typedef
NDIS_STATUS
(*CO_REQUEST_HANDLER)(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	);

typedef
VOID
(*CO_REQUEST_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST			NdisRequest
	);

//
// CO_CREATE_VC_HANDLER and CO_DELETE_VC_HANDLER are synchronous calls
//
typedef
NDIS_STATUS
(*CO_CREATE_VC_HANDLER)(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisVcHandle,
	OUT	PNDIS_HANDLE			ProtocolVcContext
	);

typedef
NDIS_STATUS
(*CO_DELETE_VC_HANDLER)(
	IN	NDIS_HANDLE				ProtocolVcContext
	);

typedef
VOID
(*CO_AF_REGISTER_NOTIFY_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY		AddressFamily
	);

typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS40_PROTOCOL_CHARACTERISTICS	Ndis40Chars;
#else
	NDIS40_PROTOCOL_CHARACTERISTICS;
#endif
	
	//
	// Placeholders for protocol extensions for PnP/PM etc.
	//
	PVOID							ReservedHandlers[4];

	//
	// Start of NDIS 5.0 extensions.
	//

	CO_SEND_COMPLETE_HANDLER		CoSendCompleteHandler;
	CO_STATUS_HANDLER				CoStatusHandler;
	CO_RECEIVE_PACKET_HANDLER		CoReceivePacketHandler;
	CO_AF_REGISTER_NOTIFY_HANDLER	CoAfRegisterNotifyHandler;

} NDIS50_PROTOCOL_CHARACTERISTICS;

#if NDIS50
typedef NDIS50_PROTOCOL_CHARACTERISTICS  NDIS_PROTOCOL_CHARACTERISTICS;
#else
#if NDIS40
typedef NDIS40_PROTOCOL_CHARACTERISTICS  NDIS_PROTOCOL_CHARACTERISTICS;
#else
typedef NDIS30_PROTOCOL_CHARACTERISTICS  NDIS_PROTOCOL_CHARACTERISTICS;
#endif
#endif
typedef NDIS_PROTOCOL_CHARACTERISTICS *PNDIS_PROTOCOL_CHARACTERISTICS;

//
// Requests used by Protocol Modules
//

EXPORT
VOID
NdisRegisterProtocol(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_HANDLE			NdisProtocolHandle,
	IN	PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
	IN	UINT					CharacteristicsLength
	);

EXPORT
VOID
NdisDeregisterProtocol(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisProtocolHandle
	);


EXPORT
VOID
NdisOpenAdapter(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_STATUS			OpenErrorStatus,
	OUT	PNDIS_HANDLE			NdisBindingHandle,
	OUT	PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				NdisProtocolHandle,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_STRING			AdapterName,
	IN	UINT					OpenOptions,
	IN	PSTRING					AddressingInformation OPTIONAL
	);


EXPORT
VOID
NdisCloseAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle
	);


EXPORT
VOID
NdisCompleteBindAdapter(
	IN	 NDIS_HANDLE			BindAdapterContext,
	IN	 NDIS_STATUS			Status,
	IN	 NDIS_STATUS			OpenStatus
	);

EXPORT
VOID
NdisCompleteUnbindAdapter(
	IN	 NDIS_HANDLE			UnbindAdapterContext,
	IN	 NDIS_STATUS			Status
	);

EXPORT
VOID
NdisSetProtocolFilter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	RECEIVE_HANDLER			ReceiveHandler,
	IN	RECEIVE_PACKET_HANDLER	ReceivePacketHandler,
	IN	NDIS_MEDIUM				Medium,
	IN	UINT					Offset,
	IN	UINT					Size,
	IN	PUCHAR					Pattern
	);

EXPORT
VOID
NdisOpenProtocolConfiguration(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_HANDLE			ConfigurationHandle,
	IN	PNDIS_STRING			ProtocolSection
);

EXPORT
VOID
NdisGetDriverHandle(
	IN	NDIS_HANDLE				NdisBindingHandle,
	OUT	PNDIS_HANDLE			NdisDriverHandle
	);

EXPORT
VOID
NdisReEnumerateProtocolBindings(
	IN	NDIS_HANDLE				NdisProtocolHandle
	);

EXPORT
NDIS_STATUS
NdisWriteEventLogEntry(
	IN	PVOID					LogHandle,
	IN	NDIS_STATUS				EventCode,
	IN	ULONG					UniqueEventValue,
	IN	USHORT					NumStrings,
	IN	PVOID					StringsList		OPTIONAL,
	IN	ULONG					DataSize,
	IN	PVOID					Data			OPTIONAL
	);

//
//	The following routine is used by transports to complete pending
//	network PnP events.
//
EXPORT
VOID
NdisCompletePnPEvent(
	IN	NDIS_STATUS		Status,
	IN	NDIS_HANDLE		NdisBindingHandle,
	IN	PNET_PNP_EVENT	NetPnPEvent
	);

//
//	The following routine is used by a transport to query the localized
//	friendly instance name of the adapter that they are bound to. There
//	are two variations of this, one uses the binding handle and the other
//	the binding context. Some transports need this before they bind - like
//	TCP/IP for instance.
//
EXPORT
NDIS_STATUS
NdisQueryAdapterInstanceName(
	OUT	PNDIS_STRING	pAdapterInstanceName,
	IN	NDIS_HANDLE		NdisBindingHandle
	);

EXPORT
NDIS_STATUS
NdisQueryBindInstanceName(
	OUT	PNDIS_STRING	pAdapterInstanceName,
	IN	NDIS_HANDLE		BindingContext
	);

//
// The following is used by TDI/NDIS interface as part of Network PnP.
// For use by TDI alone.
//
typedef
NTSTATUS
(*TDI_REGISTER_CALLBACK)(
	IN	PUNICODE_STRING			DeviceName,
	OUT	HANDLE	*				TdiHandle
	);

typedef
NTSTATUS
(*TDI_PNP_HANDLER)(
	IN	PUNICODE_STRING			UpperComponent,
	IN	PUNICODE_STRING			LowerComponent,
	IN	PUNICODE_STRING			BindList,
	IN	PVOID					ReconfigBuffer,
	IN	UINT					ReconfigBufferSize,
	IN	UINT					Operation
	);

EXPORT
VOID
NdisRegisterTdiCallBack(
	IN	TDI_REGISTER_CALLBACK	RegsterCallback,
	IN	TDI_PNP_HANDLER			PnPHandler
	);

EXPORT
VOID
NdisRegisterTdiPnpHandler(
	IN	TDI_PNP_HANDLER			PnPHandler
	);

#if BINARY_COMPATIBLE

VOID
NdisSend(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	PNDIS_PACKET			Packet
	);

VOID
NdisSendPackets(
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

VOID
NdisTransferData(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer,
	IN OUT	PNDIS_PACKET		Packet,
	OUT	PUINT					BytesTransferred
	);

VOID
NdisReset(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle
	);

VOID
NdisRequest(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	PNDIS_REQUEST			NdisRequest
	);

#else // BINARY_COMPATIBLE

#define NdisSend(Status, NdisBindingHandle, Packet)							\
{																			\
	*(Status) =																\
		(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->SendHandler)(				\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,		\
		(Packet));															\
}

#define NdisSendPackets(NdisBindingHandle, PacketArray, NumberOfPackets)	\
{																			\
	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->SendPacketsHandler)( 			\
		(PNDIS_OPEN_BLOCK)(NdisBindingHandle),								\
		(PacketArray), 														\
		(NumberOfPackets)); 												\
}

#define WanMiniportSend(Status,												\
						NdisBindingHandle,									\
						NdisLinkHandle,										\
						WanPacket)											\
{																			\
	*(Status) =																\
		((((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->WanSendHandler))( 		\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,		\
			(NdisLinkHandle),												\
			(PNDIS_PACKET)(WanPacket));										\
}

#define NdisTransferData(Status,											\
						 NdisBindingHandle, 								\
						 MacReceiveContext, 								\
						 ByteOffset,										\
						 BytesToTransfer,									\
						 Packet,											\
						 BytesTransferred)									\
{																			\
	*(Status) =																\
		(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->TransferDataHandler)( 	\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,		\
			(MacReceiveContext),											\
			(ByteOffset),													\
			(BytesToTransfer),												\
			(Packet),														\
			(BytesTransferred));											\
}


#define NdisReset(Status, NdisBindingHandle)								\
{																			\
	*(Status) =																\
		(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ResetHandler)(			\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle);		\
}

#define NdisRequest(Status, NdisBindingHandle, NdisRequest)					\
{																			\
	*(Status) =																\
		(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->RequestHandler)(			\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,		\
			(NdisRequest));													\
}

#endif // BINARY_COMPATIBLE

//
// Routines to access packet flags
//

/*++

VOID
NdisSetSendFlags(
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	);

--*/

#define NdisSetSendFlags(_Packet,_Flags)	(_Packet)->Private.Flags = (_Flags)

/*++

VOID
NdisQuerySendFlags(
	IN	PNDIS_PACKET			Packet,
	OUT	PUINT					Flags
	);

--*/

#define NdisQuerySendFlags(_Packet,_Flags)	*(_Flags) = (_Packet)->Private.Flags

//
// The following is the minimum size of packets a miniport must allocate
// when it indicates packets via NdisMIndicatePacket or NdisMCoIndicatePacket
//
#define	PROTOCOL_RESERVED_SIZE_IN_PACKET	(4 * sizeof(PVOID))

EXPORT
VOID
NdisReturnPackets(
	IN	PNDIS_PACKET	*		PacketsToReturn,
	IN	UINT					NumberOfPackets
	);

EXPORT
PNDIS_PACKET
NdisGetReceivedPacket(
	IN	NDIS_HANDLE 			NdisBindingHandle,
	IN	NDIS_HANDLE 			MacContext
	);

//
// Macros to portably manipulate NDIS buffers.
//
#if	BINARY_COMPATIBLE

EXPORT
ULONG
NdisBufferLength(
	IN	PNDIS_BUFFER			Buffer
	);

EXPORT
PVOID
NdisBufferVirtualAddress(
	IN	PNDIS_BUFFER			Buffer
	);

#else // BINARY_COMPATIBLE

#ifdef	NDIS_NT
#define NdisBufferLength(Buffer)							MmGetMdlByteCount(Buffer)
#define NdisBufferVirtualAddress(_Buffer)					MmGetSystemAddressForMdl(_Buffer)
#define NdisBufferVirtualAddressSafe(_Buffer, _Priority)	MmGetSystemAddressForMdlSafe(_Buffer, _Priority)
#else
#define NdisBufferLength(_Buffer)							(_Buffer)->Length
#define NdisBufferVirtualAddress(_Buffer)					(_Buffer)->VirtualAddress
#define NdisBufferVirtualAddressSafe(_Buffer, _Priority)	(_Buffer)->VirtualAddress
#endif

#endif	// BINARY_COMPATIBLE

