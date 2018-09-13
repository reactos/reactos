#include <afilter.h>
#include <efilter.h>
#include <tfilter.h>
#include <ffilter.h>

#define NDIS_M_MAX_LOOKAHEAD 526

//
// declare these first since they point to each other
//

typedef struct _NDIS_M_DRIVER_BLOCK		NDIS_M_DRIVER_BLOCK, *PNDIS_M_DRIVER_BLOCK;
typedef struct _NDIS_MINIPORT_BLOCK		NDIS_MINIPORT_BLOCK,*PNDIS_MINIPORT_BLOCK;
typedef struct _CO_CALL_PARAMETERS		CO_CALL_PARAMETERS, *PCO_CALL_PARAMETERS;
typedef struct _CO_MEDIA_PARAMETERS		CO_MEDIA_PARAMETERS, *PCO_MEDIA_PARAMETERS;
typedef	struct _NDIS_CALL_MANAGER_CHARACTERISTICS *PNDIS_CALL_MANAGER_CHARACTERISTICS;
typedef	struct _NDIS_AF_LIST			NDIS_AF_LIST, *PNDIS_AF_LIST;
typedef	struct _NULL_FILTER				NULL_FILTER, *PNULL_FILTER;

//
// Function types for NDIS_MINIPORT_CHARACTERISTICS
//


typedef
BOOLEAN
(*W_CHECK_FOR_HANG_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
VOID
(*W_DISABLE_INTERRUPT_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
VOID
(*W_ENABLE_INTERRUPT_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
VOID
(*W_HALT_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
VOID
(*W_HANDLE_INTERRUPT_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
NDIS_STATUS
(*W_INITIALIZE_HANDLER)(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				WrapperConfigurationContext
	);

typedef
VOID
(*W_ISR_HANDLER)(
	OUT PBOOLEAN				InterruptRecognized,
	OUT PBOOLEAN				QueueMiniportHandleInterrupt,
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
NDIS_STATUS
(*W_QUERY_INFORMATION_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesWritten,
	OUT PULONG					BytesNeeded
	);

typedef
NDIS_STATUS
(*W_RECONFIGURE_HANDLER)(
	OUT PNDIS_STATUS			OpenErrorStatus,
	IN	NDIS_HANDLE				MiniportAdapterContext	OPTIONAL,
	IN	NDIS_HANDLE				WrapperConfigurationContext
	);

typedef
NDIS_STATUS
(*W_RESET_HANDLER)(
	OUT PBOOLEAN				AddressingReset,
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

typedef
NDIS_STATUS
(*W_SEND_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	);

typedef
NDIS_STATUS
(*WM_SEND_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				NdisLinkHandle,
	IN	PNDIS_WAN_PACKET		Packet
	);

typedef
NDIS_STATUS
(*W_SET_INFORMATION_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
	);

typedef
NDIS_STATUS
(*W_TRANSFER_DATA_HANDLER)(
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer
	);

typedef
NDIS_STATUS
(*WM_TRANSFER_DATA_HANDLER)(
	VOID
	);

typedef struct _NDIS30_MINIPORT_CHARACTERISTICS
{
	UCHAR						MajorNdisVersion;
	UCHAR						MinorNdisVersion;
	USHORT						Filler;
	UINT						Reserved;
	W_CHECK_FOR_HANG_HANDLER	CheckForHangHandler;
	W_DISABLE_INTERRUPT_HANDLER	DisableInterruptHandler;
	W_ENABLE_INTERRUPT_HANDLER	EnableInterruptHandler;
	W_HALT_HANDLER				HaltHandler;
	W_HANDLE_INTERRUPT_HANDLER	HandleInterruptHandler;
	W_INITIALIZE_HANDLER		InitializeHandler;
	W_ISR_HANDLER				ISRHandler;
	W_QUERY_INFORMATION_HANDLER QueryInformationHandler;
	W_RECONFIGURE_HANDLER		ReconfigureHandler;
	W_RESET_HANDLER				ResetHandler;
	union
	{
		W_SEND_HANDLER			SendHandler;
		WM_SEND_HANDLER			WanSendHandler;
	};
	W_SET_INFORMATION_HANDLER	SetInformationHandler;
	union
	{
		W_TRANSFER_DATA_HANDLER	TransferDataHandler;
		WM_TRANSFER_DATA_HANDLER WanTransferDataHandler;
	};
} NDIS30_MINIPORT_CHARACTERISTICS;

//
// Miniport extensions for NDIS 4.0
//
typedef
VOID
(*W_RETURN_PACKET_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet
	);

//
// NDIS 4.0 extension
//
typedef
VOID
(*W_SEND_PACKETS_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

typedef
VOID
(*W_ALLOCATE_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PVOID					VirtualAddress,
	IN	PNDIS_PHYSICAL_ADDRESS	PhysicalAddress,
	IN	ULONG					Length,
	IN	PVOID					Context
	);

typedef struct _NDIS40_MINIPORT_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS30_MINIPORT_CHARACTERISTICS	Ndis30Chars;
#else
	NDIS30_MINIPORT_CHARACTERISTICS;
#endif
	//
	// Extensions for NDIS 4.0
	//
	W_RETURN_PACKET_HANDLER		ReturnPacketHandler;
	W_SEND_PACKETS_HANDLER		SendPacketsHandler;
	W_ALLOCATE_COMPLETE_HANDLER	AllocateCompleteHandler;

} NDIS40_MINIPORT_CHARACTERISTICS;


//
// Miniport extensions for NDIS 5.0
//
//
// NDIS 5.0 extension - however available for miniports only
//

//
// W_CO_CREATE_VC_HANDLER is a synchronous call
//
typedef
NDIS_STATUS
(*W_CO_CREATE_VC_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				NdisVcHandle,
	OUT	PNDIS_HANDLE			MiniportVcContext
	);

typedef
NDIS_STATUS
(*W_CO_DELETE_VC_HANDLER)(
	IN	NDIS_HANDLE				MiniportVcContext
	);

typedef
NDIS_STATUS
(*W_CO_ACTIVATE_VC_HANDLER)(
	IN	NDIS_HANDLE				MiniportVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters
	);

typedef
NDIS_STATUS
(*W_CO_DEACTIVATE_VC_HANDLER)(
	IN	NDIS_HANDLE				MiniportVcContext
	);

typedef
VOID
(*W_CO_SEND_PACKETS_HANDLER)(
	IN	NDIS_HANDLE				MiniportVcContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

typedef
NDIS_STATUS
(*W_CO_REQUEST_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportVcContext	OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	);

typedef struct _NDIS50_MINIPORT_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS40_MINIPORT_CHARACTERISTICS	Ndis40Chars;
#else
	NDIS40_MINIPORT_CHARACTERISTICS;
#endif
	//
	// Extensions for NDIS 5.0
	//
	W_CO_CREATE_VC_HANDLER		CoCreateVcHandler;
	W_CO_DELETE_VC_HANDLER		CoDeleteVcHandler;
	W_CO_ACTIVATE_VC_HANDLER	CoActivateVcHandler;
	W_CO_DEACTIVATE_VC_HANDLER	CoDeactivateVcHandler;
	W_CO_SEND_PACKETS_HANDLER	CoSendPacketsHandler;
	W_CO_REQUEST_HANDLER		CoRequestHandler;
} NDIS50_MINIPORT_CHARACTERISTICS;

#ifdef NDIS50_MINIPORT
typedef struct _NDIS50_MINIPORT_CHARACTERISTICS	NDIS_MINIPORT_CHARACTERISTICS;
#else
#ifdef NDIS40_MINIPORT
typedef struct _NDIS40_MINIPORT_CHARACTERISTICS	NDIS_MINIPORT_CHARACTERISTICS;
#else
typedef struct _NDIS30_MINIPORT_CHARACTERISTICS	NDIS_MINIPORT_CHARACTERISTICS;
#endif
#endif
typedef	NDIS_MINIPORT_CHARACTERISTICS *PNDIS_MINIPORT_CHARACTERISTICS;
typedef	NDIS_MINIPORT_CHARACTERISTICS	NDIS_WAN_MINIPORT_CHARACTERISTICS;
typedef	NDIS_WAN_MINIPORT_CHARACTERISTICS *	PNDIS_MINIPORT_CHARACTERISTICS;

typedef struct _NDIS_MINIPORT_INTERRUPT
{
	PKINTERRUPT					InterruptObject;
	KSPIN_LOCK					DpcCountLock;
	PVOID						MiniportIdField;
	W_ISR_HANDLER				MiniportIsr;
	W_HANDLE_INTERRUPT_HANDLER	MiniportDpc;
	KDPC						InterruptDpc;
	PNDIS_MINIPORT_BLOCK		Miniport;

	UCHAR						DpcCount;
	BOOLEAN						Filler1;

	//
	// This is used to tell when all the Dpcs for the adapter are completed.
	//

	KEVENT						DpcsCompletedEvent;

	BOOLEAN						SharedInterrupt;
	BOOLEAN						IsrRequested;

} NDIS_MINIPORT_INTERRUPT, *PNDIS_MINIPORT_INTERRUPT;


typedef struct _NDIS_MINIPORT_TIMER
{
	KTIMER						Timer;
	KDPC						Dpc;
	PNDIS_TIMER_FUNCTION		MiniportTimerFunction;
	PVOID						MiniportTimerContext;
	PNDIS_MINIPORT_BLOCK		Miniport;
	struct _NDIS_MINIPORT_TIMER	*NextTimer;
} NDIS_MINIPORT_TIMER, *PNDIS_MINIPORT_TIMER;

typedef
VOID
(*FILTER_PACKET_INDICATION_HANDLER)(
	IN	NDIS_HANDLE				Miniport,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

typedef
VOID
(*ETH_RCV_INDICATE_HANDLER)(
	IN	PETH_FILTER				Filter,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PCHAR					Address,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT 					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

typedef
VOID
(*ETH_RCV_COMPLETE_HANDLER)(
	IN	PETH_FILTER				Filter
	);

typedef
VOID
(*FDDI_RCV_INDICATE_HANDLER)(
	IN	PFDDI_FILTER			Filter,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PCHAR					Address,
	IN	UINT					AddressLength,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

typedef
VOID
(*FDDI_RCV_COMPLETE_HANDLER)(
	IN	PFDDI_FILTER			Filter
	);

typedef
VOID
(*TR_RCV_INDICATE_HANDLER)(
	IN	PTR_FILTER				Filter,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT 					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

typedef
VOID
(*TR_RCV_COMPLETE_HANDLER)(
	IN	PTR_FILTER				Filter
	);

typedef
VOID
(*WAN_RCV_HANDLER)(
	OUT PNDIS_STATUS			Status,
	IN NDIS_HANDLE				MiniportAdapterHandle,
	IN NDIS_HANDLE				NdisLinkContext,
	IN PUCHAR					Packet,
	IN ULONG					PacketSize
	);

typedef
VOID
(*WAN_RCV_COMPLETE_HANDLER)(
	IN NDIS_HANDLE				MiniportAdapterHandle,
	IN NDIS_HANDLE				NdisLinkContext
	);

typedef
VOID
(*NDIS_M_SEND_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*NDIS_WM_SEND_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PVOID					Packet,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*NDIS_M_TD_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
	);

typedef
VOID
(*NDIS_M_SEND_RESOURCES_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle
	);

typedef
VOID
(*NDIS_M_STATUS_HANDLER)(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

typedef
VOID
(*NDIS_M_STS_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle
	);

typedef
VOID
(*NDIS_M_REQ_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_STATUS				Status
	);

typedef
VOID
(*NDIS_M_RESET_COMPLETE_HANDLER)(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_STATUS				Status,
	IN	BOOLEAN					AddressingReset
	);

typedef
VOID
(FASTCALL *NDIS_M_PROCESS_DEFERRED)(
	IN	PNDIS_MINIPORT_BLOCK	Miniport
	);

typedef
BOOLEAN
(FASTCALL *NDIS_M_START_SENDS)(
	IN	PNDIS_MINIPORT_BLOCK	Miniport
	);

//
//  Defines the type of work item.
//
typedef enum _NDIS_WORK_ITEM_TYPE
{
	NdisWorkItemRequest,
	NdisWorkItemSend,
	NdisWorkItemReturnPackets,
	NdisWorkItemResetRequested,
	NdisWorkItemResetInProgress,
	NdisWorkItemHalt,
#if !(NDIS_NT)
	NdisWorkItemSendLoopback,
#endif
	NdisWorkItemMiniportCallback,
	NdisMaxWorkItems
} NDIS_WORK_ITEM_TYPE, *PNDIS_WORK_ITEM_TYPE;


#define	NUMBER_OF_WORK_ITEM_TYPES	NdisMaxWorkItems
#define	NUMBER_OF_SINGLE_WORK_ITEMS	6

//
//	Work item structure
//
typedef struct _NDIS_MINIPORT_WORK_ITEM
{
	//
	//	Link for the list of work items of this type.
	//
	SINGLE_LIST_ENTRY 	Link;

	//
	//	type of work item and context information.
	//
	NDIS_WORK_ITEM_TYPE WorkItemType;
	PVOID 				WorkItemContext;
} NDIS_MINIPORT_WORK_ITEM, *PNDIS_MINIPORT_WORK_ITEM;

typedef
NDIS_STATUS
(FASTCALL *NDIS_M_QUEUE_WORK_ITEM)(
	IN	PNDIS_MINIPORT_BLOCK	Miniport,
	IN	NDIS_WORK_ITEM_TYPE		WorkItemType,
	IN	PVOID					WorkItemContext
	);

typedef
NDIS_STATUS
(FASTCALL *NDIS_M_QUEUE_NEW_WORK_ITEM)(
	IN	PNDIS_MINIPORT_BLOCK	Miniport,
	IN	NDIS_WORK_ITEM_TYPE 	WorkItemType,
	IN	PVOID					WorkItemContext
	);

typedef
VOID
(FASTCALL *NDIS_M_DEQUEUE_WORK_ITEM)(
	IN	PNDIS_MINIPORT_BLOCK	Miniport,
	IN	NDIS_WORK_ITEM_TYPE		WorkItemType,
	OUT PVOID	*				WorkItemContext
	);

#if defined(NDIS_WRAPPER)

//
// Structure used by the logging apis
//
typedef struct _NDIS_LOG
{
	PNDIS_MINIPORT_BLOCK		Miniport;	// The owning miniport block
	KSPIN_LOCK					LogLock;	// For serialization
	PIRP						Irp;		// Pending Irp to consume this log
	UINT						TotalSize;	// Size of the log buffer
	UINT						CurrentSize;// Size of the log buffer
	UINT						InPtr;		// IN part of the circular buffer
	UINT						OutPtr;		// OUT part of the circular buffer
	UCHAR						LogBuf[1];	// The circular buffer
} NDIS_LOG, *PNDIS_LOG;

//
// Arcnet specific stuff
//
#define ARC_SEND_BUFFERS			8
#define ARC_HEADER_SIZE				4

typedef struct _NDIS_ARC_BUF
{
	NDIS_HANDLE					ArcnetBufferPool;
	PUCHAR						ArcnetLookaheadBuffer;
	UINT						NumFree;
	ARC_BUFFER_LIST				ArcnetBuffers[ARC_SEND_BUFFERS];
} NDIS_ARC_BUF, *PNDIS_ARC_BUF;

#endif

typedef struct _NDIS_BIND_PATHS
{
	UINT						Number;
	NDIS_STRING					Paths[1];
} NDIS_BIND_PATHS, *PNDIS_BIND_PATHS;

//
// Do not change the structure below !!!
//
typedef struct
{
	union
	{
		PETH_FILTER				EthDB;
		PNULL_FILTER			NullDB;				// Default Filter
	};
	PTR_FILTER					TrDB;
	PFDDI_FILTER				FddiDB;
	PARC_FILTER					ArcDB;
} FILTERDBS, *PFILTERDBS;

//
// one of these per mini-port registered on a Driver
//
struct _NDIS_MINIPORT_BLOCK
{
	ULONG						NullValue;			// used to distinquish between MACs and mini-ports
	PNDIS_MINIPORT_BLOCK		NextMiniport;		// used by driver's MiniportQueue
	PNDIS_M_DRIVER_BLOCK		DriverHandle;		// pointer to our Driver block
	NDIS_HANDLE					MiniportAdapterContext; // context when calling mini-port functions
	UNICODE_STRING				MiniportName;		// how mini-port refers to us
	PNDIS_BIND_PATHS			BindPaths;
	NDIS_HANDLE					OpenQueue;			// queue of opens for this mini-port
	REFERENCE					Ref;				// contains spinlock for OpenQueue

	NDIS_HANDLE					DeviceContext;		// Context associated with the intermediate driver

	UCHAR						Padding1;			// DO NOT REMOVE OR NDIS WILL BREAK!!!

	//
	// Synchronization stuff.
	//
	// The boolean is used to lock out several DPCs from running at the same time.
	//
	UCHAR						LockAcquired;		// EXPOSED via macros. Do not move

	UCHAR						PmodeOpens;			// Count of opens which turned on pmode/all_local

	//
	//	This is the processor number that the miniport's
	//	interrupt DPC and timers are running on.
	//
	UCHAR						AssignedProcessor;

	KSPIN_LOCK					Lock;

	PNDIS_REQUEST				MediaRequest;

	PNDIS_MINIPORT_INTERRUPT	Interrupt;

	ULONG						Flags;				// Flags to keep track of the
													// miniport's state.
	ULONG						PnPFlags;

	//
	// Send information
	//
	LIST_ENTRY					PacketList;
	PNDIS_PACKET				FirstPendingPacket; // This is head of the queue of packets
													// waiting to be sent to miniport.
	PNDIS_PACKET				ReturnPacketsQueue;

	//
	// Space used for temp. use during request processing
	//
	ULONG						RequestBuffer;
	PVOID						SetMCastBuffer;

	PNDIS_MINIPORT_BLOCK		PrimaryMiniport;
	PVOID						WrapperContext;

	//
	// context to pass to bus driver when reading or writing config space
	//
	PVOID						BusDataContext;
	//
	// flag to specify PnP capabilities of the device. we need this to fail query_stop
	// query_remove or suspend request if the device can not handle it
	//
	ULONG						PnPCapabilities;

	//
	// Resource information
	//
	PCM_RESOURCE_LIST			Resources;

	//
	// Watch-dog timer
	//
	NDIS_TIMER					WakeUpDpcTimer;

	//
	// Needed for PnP. Upcased version. The buffer is allocated as part of the
	// NDIS_MINIPORT_BLOCK itself.
	//
	// Note:
	// the following two fields should be explicitly UNICODE_STRING because
	// under Win9x the NDIS_STRING is an ANSI_STRING
	//
	UNICODE_STRING				BaseName;
	UNICODE_STRING				SymbolicLinkName;

	//
	// Check for hang stuff
	//
	ULONG						CheckForHangSeconds;
	USHORT						CFHangTicks;
	USHORT						CFHangCurrentTick;

	//
	// Reset information
	//
	NDIS_STATUS					ResetStatus;
	NDIS_HANDLE					ResetOpen;

	//
	// Holds media specific information.
	//
#ifdef __cplusplus
	FILTERDBS					FilterDbs;			// EXPOSED via macros. Do not move
#else
	FILTERDBS;										// EXPOSED via macros. Do not move
#endif

	FILTER_PACKET_INDICATION_HANDLER PacketIndicateHandler;
	NDIS_M_SEND_COMPLETE_HANDLER	SendCompleteHandler;
	NDIS_M_SEND_RESOURCES_HANDLER	SendResourcesHandler;
	NDIS_M_RESET_COMPLETE_HANDLER	ResetCompleteHandler;

	NDIS_MEDIUM					MediaType;

	//
	// contains mini-port information
	//
	ULONG						BusNumber;
	NDIS_INTERFACE_TYPE			BusType;
	NDIS_INTERFACE_TYPE			AdapterType;

	PDEVICE_OBJECT				DeviceObject;
	PDEVICE_OBJECT				PhysicalDeviceObject;
	PDEVICE_OBJECT				NextDeviceObject;

	//
	// Holds the map registers for this mini-port.
	//
	PMAP_REGISTER_ENTRY			MapRegisters;	// EXPOSED via macros. Do not move

	//
	// List of registered address families. Valid for the call-manager, Null for the client
	//
	PNDIS_AF_LIST				CallMgrAfList;

	PVOID						MiniportThread;
	PVOID						SetInfoBuf;
	USHORT						SetInfoBufLen;
	USHORT						MaxSendPackets;

	//
	//	Status code that is returned from the fake handlers.
	//
	NDIS_STATUS					FakeStatus;

	PVOID						LockHandler;		// For the filter lock

	//
	// the following field should be explicitly UNICODE_STRING because
	// under Win9x the NDIS_STRING is an ANSI_STRING
	//
	PUNICODE_STRING				pAdapterInstanceName;	//	Instance specific name for the adapter.

	PADAPTER_OBJECT				SystemAdapterObject;

	UINT						MacOptions;

	//
	// RequestInformation
	//
	PNDIS_REQUEST				PendingRequest;
	UINT						MaximumLongAddresses;
	UINT						MaximumShortAddresses;
	UINT						CurrentLookahead;
	UINT						MaximumLookahead;

	//
	//	For efficiency
	//
	W_HANDLE_INTERRUPT_HANDLER	HandleInterruptHandler;
	W_DISABLE_INTERRUPT_HANDLER	DisableInterruptHandler;
	W_ENABLE_INTERRUPT_HANDLER	EnableInterruptHandler;
	W_SEND_PACKETS_HANDLER		SendPacketsHandler;
	NDIS_M_START_SENDS			DeferredSendHandler;

	//
	// The following cannot be unionized.
	//
	ETH_RCV_INDICATE_HANDLER	EthRxIndicateHandler;	// EXPOSED via macros. Do not move
	TR_RCV_INDICATE_HANDLER		TrRxIndicateHandler;	// EXPOSED via macros. Do not move
	FDDI_RCV_INDICATE_HANDLER	FddiRxIndicateHandler;	// EXPOSED via macros. Do not move

	ETH_RCV_COMPLETE_HANDLER	EthRxCompleteHandler;	// EXPOSED via macros. Do not move
	TR_RCV_COMPLETE_HANDLER		TrRxCompleteHandler;	// EXPOSED via macros. Do not move
	FDDI_RCV_COMPLETE_HANDLER	FddiRxCompleteHandler;	// EXPOSED via macros. Do not move

	NDIS_M_STATUS_HANDLER		StatusHandler;			// EXPOSED via macros. Do not move
	NDIS_M_STS_COMPLETE_HANDLER	StatusCompleteHandler;	// EXPOSED via macros. Do not move
	NDIS_M_TD_COMPLETE_HANDLER	TDCompleteHandler;		// EXPOSED via macros. Do not move
	NDIS_M_REQ_COMPLETE_HANDLER	QueryCompleteHandler;	// EXPOSED via macros. Do not move
	NDIS_M_REQ_COMPLETE_HANDLER	SetCompleteHandler;		// EXPOSED via macros. Do not move

	NDIS_WM_SEND_COMPLETE_HANDLER WanSendCompleteHandler;// EXPOSED via macros. Do not move
	WAN_RCV_HANDLER				WanRcvHandler;			// EXPOSED via macros. Do not move
	WAN_RCV_COMPLETE_HANDLER	WanRcvCompleteHandler;	// EXPOSED via macros. Do not move

	/********************************************************************************************/
	/****************                                                                  **********/
	/**************** STUFF ABOVE IS POTENTIALLY ACCESSED BY MACROS. ADD STUFF BELOW   **********/
	/**************** SEVERE POSSIBILITY OF BREAKING SOMETHING IF STUFF ABOVE IS MOVED **********/
	/****************                                                                  **********/
	/********************************************************************************************/
#if defined(NDIS_WRAPPER)

	//
	// Work that the miniport needs to do.
	//
	SINGLE_LIST_ENTRY			WorkQueue[NUMBER_OF_WORK_ITEM_TYPES];
	SINGLE_LIST_ENTRY			SingleWorkItems[NUMBER_OF_SINGLE_WORK_ITEMS];

	PNDIS_MAC_BLOCK				FakeMac;

	UCHAR						SendFlags;
	UCHAR						TrResetRing;
	UCHAR						ArcnetAddress;

	union
	{
		PNDIS_ARC_BUF			ArcBuf;
		//
		// the following fiels has a different use under NT and Memphis
		//
#if NDIS_NT
		PVOID					BusInterface;
#else
		PVOID					PhysicalAddressArray;
#endif
	};

	//
	// Temp stuff for using the old NDIS functions
	//
	ULONG						ChannelNumber;

	PNDIS_LOG					Log;

	//
	// Store information here to track adapters
	//
	ULONG						BusId;
	ULONG						SlotNumber;

	PCM_RESOURCE_LIST			AllocatedResources;
	PCM_RESOURCE_LIST			AllocatedResourcesTranslated;

	//
	//	Contains a list of the packet patterns that have been added to the
	//	adapter.
	//
	SINGLE_LIST_ENTRY			PatternList;

	//
	//	The driver's power management capabilities.
	//
	NDIS_PNP_CAPABILITIES		PMCapabilities;

	//
	// DeviceCapabilites as received from bus driver
	//
	DEVICE_CAPABILITIES			DeviceCaps;

	//
	//	Contains the wake-up events that are enabled for the miniport.
	//
	ULONG						WakeUpEnable;

	//
	//	The current device state that the adapter is in.
	//
	DEVICE_POWER_STATE			CurrentDeviceState;

	//
	//	The following IRP is created in response to a cable disconnect
	//	from the device.  We keep a pointer around in case we need to cancel
	//	it.
	//
	PIRP						pIrpWaitWake;

	SYSTEM_POWER_STATE			WaitWakeSystemState;

	//
	//	The following is a pointer to a dynamically allocated array
	//	of GUID structs. This is used to map GUIDs to OIDs
	//	for custom GUIDs provided by the miniport.
	//

	LARGE_INTEGER				VcIndex;				//	Index used to identify a VC.
	KSPIN_LOCK					VcCountLock;			//	Lock used to protect VC instance count.
	LIST_ENTRY					WmiEnabledVcs;			//	List of WMI enabled VCs
	PNDIS_GUID					pNdisGuidMap;			// This is a list of all the GUIDs
														//  and OIDs supported including any
														//	customg GUIDs.
	PNDIS_GUID					pCustomGuidMap;			// This is a pointer into
														//	the pGuidToOidMap to the
														//	first custom GUID.
	USHORT						VcCount;				//	Number of VC's that have instance names.

	USHORT						cNdisGuidMap;			// This is the number of std. GUIDs
	USHORT						cCustomGuidMap;			// This is the number of custom GUIDs

	//
	// These two are used temporarily while allocating the map registers.
	//
	USHORT						CurrentMapRegister;
	PKEVENT						AllocationEvent;

	USHORT						PhysicalMapRegistersNeeded;
	USHORT                      SGMapRegistersNeeded;
	ULONG						MaximumPhysicalMapping;

	//
	// This timer is used for media disconnect timouts.
	//
	NDIS_TIMER					MediaDisconnectTimer;

	//
	// The timeout value for media disconnect timer to fire
	// default is 20 seconds
	//
	USHORT						MediaDisconnectTimeOut;

	//
	// Used for WMI support
	//
	USHORT						InstanceNumber;

	//
	// this event will be set at the end of adapter initialization
	//
	NDIS_EVENT					OpenReadyEvent;

	//
	// current PnP state of the device, ex. started, stopped, query_removed, etc.
	//
	NDIS_PNP_DEVICE_STATE		PnPDeviceState;
	
	//
	// previous device state. to be used when we get a cancel_remove or a cancel_stop
	//
	NDIS_PNP_DEVICE_STATE		OldPnPDeviceState;
	
	//
	// Handlers to Write/Read Bus data
	//
    PGET_SET_DEVICE_DATA 		SetBusData;
    PGET_SET_DEVICE_DATA 		GetBusData;

	POID_LIST					OidList;

	KDPC						DeferredDpc;

	//
	// Some NDIS gathered stats
	//
	NDIS_STATS					NdisStats;

	//
	// Valid during Packet Indication
	//
	PNDIS_PACKET				IndicatedPacket[MAXIMUM_PROCESSORS];

	//
	// this event is for protecting against returning from REMOVE IRP
	// too early and while we still have pending workitems
	//
	PKEVENT						RemoveReadyEvent;

	//
	// this event gets signaled when all opens on the miniport are closed
	//
	PKEVENT						AllOpensClosedEvent;

	//
	// this event gets signaled when all requests on the miniport are gone
	//
	PKEVENT						AllRequestsCompletedEvent;

	//
	// Init time for the miniport in milliseconds
	//
	ULONG						InitTimeMs;

	NDIS_MINIPORT_WORK_ITEM		WorkItemBuffer[NUMBER_OF_SINGLE_WORK_ITEMS];
	PNDIS_MINIPORT_TIMER		TimerQueue;
	
	//
	// flags to fail certain NDIS APIs to make sure the driver does the right things
	//
	ULONG						DriverVerifyFlags;

	//
	// used to queue miniport on global miniport queue
	//
	PNDIS_MINIPORT_BLOCK		NextGlobalMiniport;
	
	//
	// InternalResetCount:	The # of times NDIS decided a miniport was hung
	// MiniportResetCount	The # of times miniport decided it was hung
	//
	USHORT						InternalResetCount;
	USHORT						MiniportResetCount;

	USHORT						MediaSenseConnectCount;
	USHORT						MediaSenseDisconnectCount;

	PNDIS_PACKET	*			xPackets;

	//
	// track the user mode requests
	//
	ULONG						UserModeOpenReferences;

#if	LOCK_DBG
	ULONG						LockDbg;
	ULONG						LockDbgX;
	PVOID						LockThread;
#endif

#if !(NDIS_NT)
	PNDIS_PACKET				LoopbackHead;
	PNDIS_PACKET				LoopbackTail;
#endif

#endif // NDIS_WRAPPER defined
};

//
//	Routines for intermediate miniport drivers.
//
typedef
VOID
(*W_MINIPORT_CALLBACK)(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PVOID					CallbackContext
	);

//
// These are now obsolete. Use Deserialized driver model for optimal performance.
//
EXPORT
NDIS_STATUS
NdisIMQueueMiniportCallback(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	W_MINIPORT_CALLBACK		CallbackRoutine,
	IN	PVOID					CallbackContext
	);

EXPORT
BOOLEAN
NdisIMSwitchToMiniport(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	OUT	PNDIS_HANDLE			SwitchHandle
	);

EXPORT
VOID
NdisIMRevertBack(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				SwitchHandle
	);

EXPORT
NDIS_STATUS
NdisIMRegisterLayeredMiniport(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
	IN	UINT					CharacteristicsLength,
	OUT PNDIS_HANDLE			DriverHandle
	);

EXPORT
VOID
NdisIMDeregisterLayeredMiniport(
	IN	NDIS_HANDLE			DriverHandle
	);

EXPORT
VOID
NdisIMAssociateMiniport(
	IN	NDIS_HANDLE			DriverHandle,
	IN	NDIS_HANDLE			ProtocolHandle
	);

EXPORT
NDIS_STATUS
NdisMRegisterDevice(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PNDIS_STRING			DeviceName,
	IN	PNDIS_STRING			SymbolicName,
	IN	PDRIVER_DISPATCH		MajorFunctions[],
	OUT	PDEVICE_OBJECT		*	pDeviceObject,
	OUT	NDIS_HANDLE			*	NdisDeviceHandle
	);

EXPORT
NDIS_STATUS
NdisMDeregisterDevice(
	IN	NDIS_HANDLE				NdisDeviceHandle
	);

EXPORT
VOID
NdisMRegisterUnloadHandler(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PDRIVER_UNLOAD			UnloadHandler
	);

//
// Operating System Requests
//
typedef UCHAR	NDIS_DMA_SIZE;

#define	NDIS_DMA_24BITS				((NDIS_DMA_SIZE)0)
#define	NDIS_DMA_32BITS				((NDIS_DMA_SIZE)1)
#define	NDIS_DMA_64BITS				((NDIS_DMA_SIZE)2)

EXPORT
NDIS_STATUS
NdisMAllocateMapRegisters(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					DmaChannel,
	IN	NDIS_DMA_SIZE			DmaSize,
	IN	ULONG					PhysicalMapRegistersNeeded,
	IN	ULONG					MaximumPhysicalMapping
	);

EXPORT
VOID
NdisMFreeMapRegisters(
	IN	NDIS_HANDLE				MiniportAdapterHandle
	);

EXPORT
NDIS_STATUS
NdisMInitializeScatterGatherDma(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	BOOLEAN					Dma64BitAddresses,
	IN	ULONG					MaximumPhysicalMapping
	);

EXPORT
NDIS_STATUS
NdisMRegisterIoPortRange(
	OUT PVOID *					PortOffset,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					InitialPort,
	IN	UINT					NumberOfPorts
	);

EXPORT
VOID
NdisMDeregisterIoPortRange(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					InitialPort,
	IN	UINT					NumberOfPorts,
	IN	PVOID					PortOffset
	);

EXPORT
NDIS_STATUS
NdisMMapIoSpace(
	OUT PVOID *					VirtualAddress,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress,
	IN	UINT					Length
	);

EXPORT
VOID
NdisMUnmapIoSpace(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PVOID					VirtualAddress,
	IN	UINT					Length
	);

EXPORT
NDIS_STATUS
NdisMRegisterInterrupt(
	OUT	PNDIS_MINIPORT_INTERRUPT Interrupt,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					InterruptVector,
	IN	UINT					InterruptLevel,
	IN	BOOLEAN					RequestIsr,
	IN	BOOLEAN					SharedInterrupt,
	IN	NDIS_INTERRUPT_MODE		InterruptMode
	);

EXPORT
VOID
NdisMDeregisterInterrupt(
	IN	PNDIS_MINIPORT_INTERRUPT Interrupt
	);

EXPORT
BOOLEAN
NdisMSynchronizeWithInterrupt(
	IN	PNDIS_MINIPORT_INTERRUPT Interrupt,
	IN	PVOID					SynchronizeFunction,
	IN	PVOID					SynchronizeContext
	);


EXPORT
VOID
NdisMQueryAdapterResources(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	OUT PNDIS_RESOURCE_LIST		ResourceList,
	IN	OUT PUINT				BufferSize
	);

//
// Timers
//
// VOID
// NdisMSetTimer(
//	IN	PNDIS_MINIPORT_TIMER	Timer,
//	IN	UINT					MillisecondsToDelay
//	);
#define	NdisMSetTimer(_Timer, _Delay)	NdisSetTimer((PNDIS_TIMER)_Timer, _Delay)

VOID
NdisMSetPeriodicTimer(
	IN	PNDIS_MINIPORT_TIMER	 Timer,
	IN	UINT					 MillisecondPeriod
	);

EXPORT
VOID
NdisMInitializeTimer(
	IN	OUT PNDIS_MINIPORT_TIMER Timer,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_TIMER_FUNCTION	TimerFunction,
	IN	PVOID					FunctionContext
	);

EXPORT
VOID
NdisMCancelTimer(
	IN	PNDIS_MINIPORT_TIMER	Timer,
	OUT PBOOLEAN				TimerCancelled
	);

EXPORT
VOID
NdisMSleep(
	IN	ULONG					MicrosecondsToSleep
	);

//
// Physical Mapping
//
EXPORT
VOID
NdisMStartBufferPhysicalMapping(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					PhysicalMapRegister,
	IN	BOOLEAN					WriteToDevice,
	OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
	OUT PUINT					ArraySize
	);

EXPORT
VOID
NdisMCompleteBufferPhysicalMapping(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					PhysicalMapRegister
	);


//
// Shared memory
//
EXPORT
VOID
NdisMAllocateSharedMemory(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	ULONG					Length,
	IN	BOOLEAN					Cached,
	OUT PVOID *					VirtualAddress,
	OUT PNDIS_PHYSICAL_ADDRESS	PhysicalAddress
	);

EXPORT
NDIS_STATUS
NdisMAllocateSharedMemoryAsync(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	ULONG					Length,
	IN	BOOLEAN					Cached,
	IN	PVOID					Context
	);

/*++
VOID
NdisMUpdateSharedMemory(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	ULONG					Length,
	IN	PVOID					VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	)
--*/
#define NdisMUpdateSharedMemory(_H, _L, _V, _P) NdisUpdateSharedMemory(_H, _L, _V, _P)


EXPORT
VOID
NdisMFreeSharedMemory(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	ULONG					Length,
	IN	BOOLEAN					Cached,
	IN	PVOID					VirtualAddress,
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	);


//
// DMA operations.
//
EXPORT
NDIS_STATUS
NdisMRegisterDmaChannel(
	OUT PNDIS_HANDLE			MiniportDmaHandle,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					DmaChannel,
	IN	BOOLEAN					Dma32BitAddresses,
	IN	PNDIS_DMA_DESCRIPTION	DmaDescription,
	IN	ULONG					MaximumLength
	);


EXPORT
VOID
NdisMDeregisterDmaChannel(
	IN	NDIS_HANDLE				MiniportDmaHandle
	);

/*++
VOID
NdisMSetupDmaTransfer(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				MiniportDmaHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					Offset,
	IN	ULONG					Length,
	IN	BOOLEAN					WriteToDevice
	)
--*/
#define NdisMSetupDmaTransfer(_S, _H, _B, _O, _L, _M_) \
		NdisSetupDmaTransfer(_S, _H, _B, _O, _L, _M_)

/*++
VOID
NdisMCompleteDmaTransfer(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				MiniportDmaHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					Offset,
	IN	ULONG					Length,
	IN	BOOLEAN					WriteToDevice
	)
--*/
#define NdisMCompleteDmaTransfer(_S, _H, _B, _O, _L, _M_) \
		NdisCompleteDmaTransfer(_S, _H, _B, _O, _L, _M_)

EXPORT
ULONG
NdisMReadDmaCounter(
	IN	NDIS_HANDLE				MiniportDmaHandle
	);


//
// Requests Used by Miniport Drivers
//
#define NdisMInitializeWrapper(_a,_b,_c,_d) NdisInitializeWrapper((_a),(_b),(_c),(_d))

EXPORT
NDIS_STATUS
NdisMRegisterMiniport(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
	IN	UINT					CharacteristicsLength
	);

// EXPORT
// NDIS_STATUS
// NdisIMInitializeDeviceInstance(
// 	IN	NDIS_HANDLE				DriverHandle,
// 	IN	PNDIS_STRING			DriverInstance
// 	);
#define	NdisIMInitializeDeviceInstance(_H_, _I_)	\
								NdisIMInitializeDeviceInstanceEx(_H_, _I_, NULL)

EXPORT
NDIS_STATUS
NdisIMInitializeDeviceInstanceEx(
	IN	NDIS_HANDLE				DriverHandle,
	IN	PNDIS_STRING			DriverInstance,
	IN	NDIS_HANDLE				DeviceContext	OPTIONAL
	);

EXPORT
NDIS_STATUS
NdisIMCancelInitializeDeviceInstance(
	IN	NDIS_HANDLE				DriverHandle,
	IN	PNDIS_STRING			DeviceInstance
	);

EXPORT
NDIS_HANDLE
NdisIMGetDeviceContext(
	IN	NDIS_HANDLE				MiniportAdapterHandle
	);

EXPORT
NDIS_HANDLE
NdisIMGetBindingContext(
	IN	NDIS_HANDLE				NdisBindingHandle
	);

EXPORT
NDIS_STATUS
NdisIMDeInitializeDeviceInstance(
	IN	NDIS_HANDLE				NdisMiniportHandle
	);

EXPORT
VOID
NdisIMCopySendPerPacketInfo(
	IN PNDIS_PACKET DstPacket,
	IN PNDIS_PACKET SrcPacket
	);

EXPORT
VOID
NdisIMCopySendCompletePerPacketInfo(
	IN PNDIS_PACKET DstPacket, 
	PNDIS_PACKET SrcPacket
	);             

// EXPORT
// VOID
// NdisMSetAttributes(
// 	IN	NDIS_HANDLE				MiniportAdapterHandle,
// 	IN	NDIS_HANDLE				MiniportAdapterContext,
// 	IN	BOOLEAN					BusMaster,
// 	IN	NDIS_INTERFACE_TYPE		AdapterType
// 	);
#define	NdisMSetAttributes(_H_, _C_, _M_, _T_)										\
						NdisMSetAttributesEx(_H_,									\
											 _C_,									\
											 0,										\
											 (_M_) ? NDIS_ATTRIBUTE_BUS_MASTER : 0,	\
											 _T_)									\


EXPORT
VOID
NdisMSetAttributesEx(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	UINT					CheckForHangTimeInSeconds OPTIONAL,
	IN	ULONG					AttributeFlags,
	IN	NDIS_INTERFACE_TYPE		AdapterType	OPTIONAL
	);

#define	NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT		0x00000001
#define NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT		0x00000002
#define NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS		0x00000004
#define NDIS_ATTRIBUTE_BUS_MASTER					0x00000008
#define NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER			0x00000010
#define NDIS_ATTRIBUTE_DESERIALIZE					0x00000020
#define	NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND			0x00000040

EXPORT
NDIS_STATUS
NdisMSetMiniportSecondary(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	NDIS_HANDLE				PrimaryMiniportHandle
	);

EXPORT
NDIS_STATUS
NdisMPromoteMiniport(
	IN	NDIS_HANDLE				MiniportHandle
	);

EXPORT
NDIS_STATUS
NdisMRemoveMiniport(
	IN	NDIS_HANDLE				MiniportHandle
	);

#define	NdisMSendComplete(_M, _P, _S)	(*((PNDIS_MINIPORT_BLOCK)(_M))->SendCompleteHandler)(_M, _P, _S)

#define	NdisMSendResourcesAvailable(_M)	(*((PNDIS_MINIPORT_BLOCK)(_M))->SendResourcesHandler)(_M)

#define	NdisMResetComplete(_M, _S, _A)	(*((PNDIS_MINIPORT_BLOCK)(_M))->ResetCompleteHandler)(_M, _S, _A)

#define	NdisMTransferDataComplete(_M, _P, _S, _B)	\
										(*((PNDIS_MINIPORT_BLOCK)(_M))->TDCompleteHandler)(_M, _P, _S, _B)

/*++

VOID
NdisMWanSendComplete(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PVOID					Packet,
	IN	NDIS_STATUS				Status
	);

--*/

#define	NdisMWanSendComplete(_M_, _P_, _S_)												\
				(*((PNDIS_MINIPORT_BLOCK)(_M_))->WanSendCompleteHandler)(_M_, _P_, _S_)

#define	NdisMQueryInformationComplete(_M, _S)	\
										(*((PNDIS_MINIPORT_BLOCK)(_M))->QueryCompleteHandler)(_M, _S)

#define	NdisMSetInformationComplete(_M, _S)	\
										(*((PNDIS_MINIPORT_BLOCK)(_M))->SetCompleteHandler)(_M, _S)

/*++

VOID
NdisMIndicateReceivePacket(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PPNDIS_PACKET			ReceivedPackets,
	IN	UINT					NumberOfPackets
	);

--*/
#define NdisMIndicateReceivePacket(_H, _P, _N)									\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->PacketIndicateHandler)(						\
						_H,														\
						_P,														\
						_N);													\
}

/*++

VOID
NdisMWanIndicateReceive(
	OUT PNDIS_STATUS			Status,
	IN NDIS_HANDLE				MiniportAdapterHandle,
	IN NDIS_HANDLE				NdisLinkContext,
	IN PUCHAR					Packet,
	IN ULONG					PacketSize
	);

--*/

#define	NdisMWanIndicateReceive(_S_, _M_, _C_, _P_, _Z_)						\
				(*((PNDIS_MINIPORT_BLOCK)(_M_))->WanRcvHandler)(_S_, _M_, _C_, _P_, _Z_)

/*++

VOID
NdisMWanIndicateReceiveComplete(
	IN NDIS_HANDLE				MiniportAdapterHandle,
	IN NDIS_HANDLE				NdisLinkContext
	);

--*/

#define	NdisMWanIndicateReceiveComplete(_M_, _C_)									\
				(*((PNDIS_MINIPORT_BLOCK)(_M_))->WanRcvCompleteHandler)(_M_, _C_)

/*++

VOID
NdisMEthIndicateReceive(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	)

--*/
#define NdisMEthIndicateReceive( _H, _C, _B, _SZ, _L, _LSZ, _PSZ)				\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->EthRxIndicateHandler)(						\
		((PNDIS_MINIPORT_BLOCK)(_H))->EthDB,									\
		_C,																		\
		_B,																		\
		_B,																		\
		_SZ,																	\
		_L,																		\
		_LSZ,																	\
		_PSZ																	\
		);																		\
}

/*++

VOID
NdisMTrIndicateReceive(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	)

--*/
#define NdisMTrIndicateReceive( _H, _C, _B, _SZ, _L, _LSZ, _PSZ)				\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->TrRxIndicateHandler)(						\
		((PNDIS_MINIPORT_BLOCK)(_H))->TrDB,										\
		_C,																		\
		_B,																		\
		_SZ,																	\
		_L,																		\
		_LSZ,																	\
		_PSZ																	\
		);																		\
}

/*++

VOID
NdisMFddiIndicateReceive(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	)

--*/

#define NdisMFddiIndicateReceive( _H, _C, _B, _SZ, _L, _LSZ, _PSZ)				\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->FddiRxIndicateHandler)(						\
			((PNDIS_MINIPORT_BLOCK)(_H))->FddiDB,								\
			_C,																	\
			(PUCHAR)_B + 1,														\
			((((PUCHAR)_B)[0] & 0x40) ? FDDI_LENGTH_OF_LONG_ADDRESS 			\
							: FDDI_LENGTH_OF_SHORT_ADDRESS),					\
			_B,																	\
			_SZ,																\
			_L,																	\
			_LSZ,																\
			_PSZ																\
	);																			\
}

/*++

VOID
NdisMArcIndicateReceive(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	PUCHAR					pRawHeader,		// Pointer to Arcnet frame header
	IN	PUCHAR					pData,			// Pointer to data portion of Arcnet frame
	IN	UINT					Length			// Data Length
	)

--*/
#define NdisMArcIndicateReceive( _H, _HD, _D, _SZ)								\
{																				\
	ArcFilterDprIndicateReceive(((PNDIS_MINIPORT_BLOCK)(_H))->ArcDB,			\
								_HD,											\
								_D,												\
								_SZ												\
								);												\
}


/*++

VOID
NdisMEthIndicateReceiveComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define NdisMEthIndicateReceiveComplete( _H )									\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->EthRxCompleteHandler)(						\
										((PNDIS_MINIPORT_BLOCK)_H)->EthDB);		\
}

/*++

VOID
NdisMTrIndicateReceiveComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define NdisMTrIndicateReceiveComplete( _H )									\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->TrRxCompleteHandler)(						\
										((PNDIS_MINIPORT_BLOCK)_H)->TrDB);		\
}

/*++

VOID
NdisMFddiIndicateReceiveComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define NdisMFddiIndicateReceiveComplete( _H )									\
{																				\
	(*((PNDIS_MINIPORT_BLOCK)(_H))->FddiRxCompleteHandler)(						\
										((PNDIS_MINIPORT_BLOCK)_H)->FddiDB);	\
}

/*++

VOID
NdisMArcIndicateReceiveComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define NdisMArcIndicateReceiveComplete( _H )									\
{																				\
	if (((PNDIS_MINIPORT_BLOCK)_H)->EthDB)										\
	{																			\
		NdisMEthIndicateReceiveComplete(_H);									\
	}																			\
																				\
	ArcFilterDprIndicateReceiveComplete(((PNDIS_MINIPORT_BLOCK)_H)->ArcDB);		\
}

/*++

EXPORT
VOID
NdisMIndicateStatus(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);
--*/

#define	NdisMIndicateStatus(_M, _G, _SB, _BS)	(*((PNDIS_MINIPORT_BLOCK)(_M))->StatusHandler)(_M, _G, _SB, _BS)

/*++

EXPORT
VOID
NdisMIndicateStatusComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define	NdisMIndicateStatusComplete(_M)	(*((PNDIS_MINIPORT_BLOCK)(_M))->StatusCompleteHandler)(_M)

EXPORT
VOID
NdisMRegisterAdapterShutdownHandler(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	PVOID					ShutdownContext,
	IN	ADAPTER_SHUTDOWN_HANDLER ShutdownHandler
	);

EXPORT
VOID
NdisMDeregisterAdapterShutdownHandler(
	IN	NDIS_HANDLE				MiniportHandle
	);

EXPORT
NDIS_STATUS
NdisMPciAssignResources(
	IN	NDIS_HANDLE				MiniportHandle,
	IN	ULONG					SlotNumber,
	IN	PNDIS_RESOURCE_LIST *	AssignedResources
	);

//
// Logging support for miniports
//

EXPORT
NDIS_STATUS
NdisMCreateLog(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	UINT					Size,
	OUT	PNDIS_HANDLE			LogHandle
	);

EXPORT
VOID
NdisMCloseLog(
	IN	NDIS_HANDLE				LogHandle
	);

EXPORT
NDIS_STATUS
NdisMWriteLogData(
	IN	NDIS_HANDLE				LogHandle,
	IN	PVOID					LogBuffer,
	IN	UINT					LogBufferSize
	);

EXPORT
VOID
NdisMFlushLog(
	IN	NDIS_HANDLE				LogHandle
	);

EXPORT
VOID
NdisMGetDeviceProperty(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN OUT PDEVICE_OBJECT *		PhysicalDeviceObject		OPTIONAL,
	IN OUT PDEVICE_OBJECT *		FunctionalDeviceObject		OPTIONAL,
	IN OUT PDEVICE_OBJECT *		NextDeviceObject			OPTIONAL,
	IN OUT PCM_RESOURCE_LIST *	AllocatedResources			OPTIONAL,
	IN OUT PCM_RESOURCE_LIST *	AllocatedResourcesTranslated OPTIONAL
	);

//
//	Get a pointer to the adapter's localized instance name.
//
EXPORT
NDIS_STATUS
NdisMQueryAdapterInstanceName(
	OUT	PNDIS_STRING			pAdapterInstanceName,
	IN	NDIS_HANDLE				MiniportHandle
	);

//
// NDIS 5.0 extensions for miniports
//

EXPORT
VOID
NdisMCoIndicateReceivePacket(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

EXPORT
VOID
NdisMCoIndicateStatus(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				NdisVcHandle	OPTIONAL,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer	OPTIONAL,
	IN	ULONG					StatusBufferSize
	);

EXPORT
VOID
NdisMCoReceiveComplete(
	IN	NDIS_HANDLE				MiniportAdapterHandle
	);

EXPORT
VOID
NdisMCoSendComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PNDIS_PACKET			Packet
	);

EXPORT
VOID
NdisMCoActivateVcComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

EXPORT
VOID
NdisMCoDeactivateVcComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle
	);

EXPORT
VOID
NdisMCoRequestComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PNDIS_REQUEST			Request
	);

EXPORT
NDIS_STATUS
NdisMCmRegisterAddressFamily(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	PCO_ADDRESS_FAMILY		AddressFamily,
	IN	PNDIS_CALL_MANAGER_CHARACTERISTICS CmCharacteristics,
	IN	UINT					SizeOfCmCharacteristics
	);

EXPORT
NDIS_STATUS
NdisMCmCreateVc(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				NdisAfHandle,
	IN	NDIS_HANDLE				MiniportVcContext,
	OUT	PNDIS_HANDLE			NdisVcHandle
	);

EXPORT
NDIS_STATUS
NdisMCmDeleteVc(
	IN	NDIS_HANDLE				NdisVcHandle
	);


EXPORT
NDIS_STATUS
NdisMCmActivateVc(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

EXPORT
NDIS_STATUS
NdisMCmDeactivateVc(
	IN	NDIS_HANDLE				NdisVcHandle
	);


EXPORT
NDIS_STATUS
NdisMCmRequest(
	IN	NDIS_HANDLE				NdisAfHandle,
	IN	NDIS_HANDLE				NdisVcHandle	OPTIONAL,
	IN	NDIS_HANDLE				NdisPartyHandle OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	);

// EXPORT
// VOID
// NdisMCmRequestComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisAfHandle,
// 	IN	NDIS_HANDLE				NdisVcHandle	OPTIONAL,
// 	IN	NDIS_HANDLE				NdisPartyHandle	OPTIONAL,
//	IN	PNDIS_REQUEST			NdisRequest
//	);
#define	NdisMCmRequestComplete(_S_, _AH_, _VH_, _PH_, _R_) \
										NdisCoRequestComplete(_S_, _AH_, _VH_, _PH_, _R_)

// EXPORT
// VOID
// NdisMCmOpenAddressFamilyComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisAfHandle,
// 	IN	NDIS_HANDLE				CallMgrAfContext
// 	);

#define	NdisMCmOpenAddressFamilyComplete(_S_, _H_, _C_)	\
										NdisCmOpenAddressFamilyComplete(_S_, _H_, _C_)


// EXPORT
// VOID
// NdisMCmCloseAddressFamilyComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisAfHandle
// 	);

#define	NdisMCmCloseAddressFamilyComplete(_S_, _H_)		\
										NdisCmCloseAddressFamilyComplete(_S_, _H_)



// EXPORT
// VOID
// NdisMCmRegisterSapComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisSapHandle,
// 	IN	NDIS_HANDLE				CallMgrSapContext
// 	);

#define	NdisMCmRegisterSapComplete(_S_, _H_, _C_)		\
										NdisCmRegisterSapComplete(_S_, _H_, _C_)


// EXPORT
// VOID
// NdisMCmDeregisterSapComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisSapHandle
// 	);

#define	NdisMCmDeregisterSapComplete(_S_, _H_)			\
										NdisCmDeregisterSapComplete(_S_, _H_)


// EXPORT
// VOID
// NdisMCmMakeCallComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisVcHandle,
// 	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
// 	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
// 	IN	PCO_CALL_PARAMETERS		CallParameters
// 	);

#define	NdisMCmMakeCallComplete(_S_, _VH_, _PH_, _CC_, _CP_)	\
										NdisCmMakeCallComplete(_S_, _VH_, _PH_, _CC_, _CP_)


// EXPORT
// VOID
// NdisMCmCloseCallComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisVcHandle,
// 	IN	NDIS_HANDLE				NdisPartyHandle	OPTIONAL
// 	);

#define	NdisMCmCloseCallComplete(_S_, _VH_, _PH_)		\
										NdisCmCloseCallComplete(_S_, _VH_, _PH_)


// EXPORT
// VOID
// NdisMCmAddPartyComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisPartyHandle,
// 	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
// 	IN	PCO_CALL_PARAMETERS		CallParameters
// 	);

#define	NdisMCmAddPartyComplete(_S_, _H_, _C_, _P_)		\
										NdisCmAddPartyComplete(_S_, _H_, _C_, _P_)


// EXPORT
// VOID
// NdisMCmDropPartyComplete(
// 	IN	NDIS_STATUS				Status,
// 	IN	NDIS_HANDLE				NdisPartyHandle
// 	);

#define	NdisMCmDropPartyComplete(_S_, _H_)				\
										NdisCmDropPartyComplete(_S_, _H_)


// EXPORT
// NDIS_STATUS
// NdisMCmDispatchIncomingCall(
// 	IN	NDIS_HANDLE				NdisSapHandle,
// 	IN	NDIS_HANDLE				NdisVcHandle,
// 	IN	PCO_CALL_PARAMETERS		CallParameters
// 	);

#define	NdisMCmDispatchIncomingCall(_SH_, _VH_, _CP_)	\
										NdisCmDispatchIncomingCall(_SH_, _VH_, _CP_)


// EXPORT
// VOID
// NdisMCmDispatchCallConnected(
// 	IN	NDIS_HANDLE				NdisVcHandle
// 	);

#define	NdisMCmDispatchCallConnected(_H_)				\
										NdisCmDispatchCallConnected(_H_)


// EXPORT
// NdisMCmModifyCallQoSComplete(
//	IN	NDIS_STATUS				Status,
//	IN	NDIS_HANDLE				NdisVcHandle,
//	IN	PCO_CALL_PARAMETERS		CallParameters
// 	);

#define	NdisMCmModifyCallQoSComplete(_S_, _H_, _P_)		\
										NdisCmModifyCallQoSComplete(_S_, _H_, _P_)


// EXPORT
// VOID
// VOID
// NdisMCmDispatchIncomingCallQoSChange(
// 	IN	NDIS_HANDLE				NdisVcHandle,
// 	IN	PCO_CALL_PARAMETERS		CallParameters
// 	);

#define	NdisMCmDispatchIncomingCallQoSChange(_H_, _P_)	\
										NdisCmDispatchIncomingCallQoSChange(_H_, _P_)


// EXPORT
// VOID
// NdisMCmDispatchIncomingCloseCall(
//   IN  NDIS_STATUS			 CloseStatus,
//   IN  NDIS_HANDLE			 NdisVcHandle,
//   IN  PVOID					 Buffer			OPTIONAL,
//   IN  UINT					 Size
//   );

#define	NdisMCmDispatchIncomingCloseCall(_S_, _H_, _B_, _Z_)	\
										NdisCmDispatchIncomingCloseCall(_S_, _H_, _B_, _Z_)


//	EXPORT
//	VOID
//	NdisMCmDispatchIncomingDropParty(
//		IN	NDIS_STATUS			DropStatus,
//		IN	NDIS_HANDLE			NdisPartyHandle,
//		IN	PVOID				Buffer		OPTIONAL,
//		IN	UINT				Size
//		);
#define	NdisMCmDispatchIncomingDropParty(_S_, _H_, _B_, _Z_)	\
										NdisCmDispatchIncomingDropParty(_S_, _H_, _B_, _Z_)




