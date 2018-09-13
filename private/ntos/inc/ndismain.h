//
// Indicate that we're building for NT. NDIS_NT is always used for
// miniport builds.
//

#define NDIS_NT 1

#if defined(NDIS_DOS)
#undef NDIS_DOS
#endif


//
// Define status codes and event log codes.
//

#include <ntstatus.h>
#include <netevent.h>

//
// Define a couple of extra types.
//

#if !defined(_WINDEF_)		// these are defined in windows.h too
typedef signed int INT, *PINT;
typedef unsigned int UINT, *PUINT;
#endif

typedef UNICODE_STRING NDIS_STRING, *PNDIS_STRING;


//
// Portability extentions
//

#define NDIS_INIT_FUNCTION(_F)		alloc_text(INIT,_F)
#define NDIS_PAGABLE_FUNCTION(_F)	alloc_text(PAGE,_F)
#define NDIS_PAGEABLE_FUNCTION(_F)	alloc_text(PAGE,_F)

//
// This file contains the definition of an NDIS_OID as
// well as #defines for all the current OID values.
//

//
// Define NDIS_STATUS and NDIS_HANDLE here
//
typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

typedef int NDIS_STATUS, *PNDIS_STATUS; // note default size

#include <ntddndis.h>

//
// Ndis defines for configuration manager data structures
//
typedef CM_MCA_POS_DATA NDIS_MCA_POS_DATA, *PNDIS_MCA_POS_DATA;
typedef CM_EISA_SLOT_INFORMATION NDIS_EISA_SLOT_INFORMATION, *PNDIS_EISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION NDIS_EISA_FUNCTION_INFORMATION, *PNDIS_EISA_FUNCTION_INFORMATION;

//
// Define an exported function.
//
#if defined(NDIS_WRAPPER)
#define EXPORT
#else
#define EXPORT DECLSPEC_IMPORT
#endif

//
// Memory manipulation functions.
//
#define NdisMoveMemory(Destination, Source, Length)	RtlCopyMemory(Destination, Source, Length)
#define NdisZeroMemory(Destination, Length)			RtlZeroMemory(Destination, Length)
#define	NdisEqualMemory(Source1, Source2, Length)	RtlEqualMemory(Source1, Source2, Length)
#define NdisFillMemory(Destination, Length, Fill)	RtlFillMemory(Destination, Length, Fill)
#define NdisRetrieveUlong(Destination, Source)		RtlRetrieveUlong(Destination, Source)
#define NdisStoreUlong(Destination, Value)			RtlStoreUlong(Destination, Value)

#define NDIS_STRING_CONST(x)	{sizeof(L##x)-2, sizeof(L##x), L##x}

//
// On a RISC machine, I/O mapped memory can't be accessed with
// the Rtl routines.
//
#if defined(_M_IX86)

#define NdisMoveMappedMemory(Destination,Source,Length) RtlCopyMemory(Destination,Source,Length)
#define NdisZeroMappedMemory(Destination,Length)		RtlZeroMemory(Destination,Length)

#elif defined(_M_IA64)

#define NdisMoveMappedMemory(Destination,Source,Length)						\
{																			\
	PUCHAR _Src = (Source);													\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		*_Dest++ = *_Src++;													\
	}																		\
}

#define NdisZeroMappedMemory(Destination,Length)							\
{																			\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		*_Dest++ = 0;														\
	}																		\
}

#elif defined(_ALPHA_)

#define NdisMoveMappedMemory(Destination,Source,Length) 					\
{																			\
	PUCHAR _Src = (Source);													\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		NdisReadRegisterUchar(_Src, _Dest);									\
		_Src++;																\
		_Dest++;															\
	}																		\
}

#define NdisZeroMappedMemory(Destination,Length)							\
{																			\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		NdisWriteRegisterUchar(_Dest,0);									\
		_Dest++;															\
	}																		\
}

#endif


//
// On Mips and Intel systems, these are the same. On Alpha, they are different.
//

#if defined(_ALPHA_)

#define NdisMoveToMappedMemory(Destination,Source,Length)					\
							WRITE_REGISTER_BUFFER_UCHAR(Destination,Source,Length)
#define NdisMoveFromMappedMemory(Destination,Source,Length)					\
							READ_REGISTER_BUFFER_UCHAR(Source,Destination,Length)
#else

#define NdisMoveToMappedMemory(Destination,Source,Length)					\
							NdisMoveMappedMemory(Destination,Source,Length)
#define NdisMoveFromMappedMemory(Destination,Source,Length)					\
							NdisMoveMappedMemory(Destination,Source,Length)
#endif


//
// definition of the basic spin lock structure
//

typedef struct _NDIS_SPIN_LOCK
{
	KSPIN_LOCK	SpinLock;
	KIRQL		OldIrql;
} NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;


//
// definition of the ndis event structure
//
typedef struct _NDIS_EVENT
{
	KEVENT		Event;	
} NDIS_EVENT, *PNDIS_EVENT;

typedef	VOID	(*NDIS_PROC)(struct _NDIS_WORK_ITEM *, PVOID);

//
// Definition of an ndis work-item
//
typedef struct _NDIS_WORK_ITEM
{
	PVOID			Context;
	NDIS_PROC		Routine;
	UCHAR			WrapperReserved[8*sizeof(PVOID)];
} NDIS_WORK_ITEM, *PNDIS_WORK_ITEM;

#define NdisInterruptLatched			Latched
#define NdisInterruptLevelSensitive		LevelSensitive
typedef KINTERRUPT_MODE NDIS_INTERRUPT_MODE, *PNDIS_INTERRUPT_MODE;

//
// Configuration definitions
//

//
// Possible data types
//

typedef enum _NDIS_PARAMETER_TYPE
{
	NdisParameterInteger,
	NdisParameterHexInteger,
	NdisParameterString,
	NdisParameterMultiString,
	NdisParameterBinary
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef	struct
{
	USHORT			Length;
	PVOID			Buffer;
} BINARY_DATA;

//
// To store configuration information
//
typedef struct _NDIS_CONFIGURATION_PARAMETER
{
	NDIS_PARAMETER_TYPE ParameterType;
	union
	{
		ULONG			IntegerData;
		NDIS_STRING		StringData;
		BINARY_DATA		BinaryData;
	} ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;


//
// Definitions for the "ProcessorType" keyword
//
typedef enum _NDIS_PROCESSOR_TYPE
{
	NdisProcessorX86,
	NdisProcessorMips,
	NdisProcessorAlpha,
	NdisProcessorPpc
} NDIS_PROCESSOR_TYPE, *PNDIS_PROCESSOR_TYPE;

//
// Definitions for the "Environment" keyword
//
typedef enum _NDIS_ENVIRONMENT_TYPE
{
	NdisEnvironmentWindows,
	NdisEnvironmentWindowsNt
} NDIS_ENVIRONMENT_TYPE, *PNDIS_ENVIRONMENT_TYPE;


//
// Possible Hardware Architecture. Define these to
// match the HAL INTERFACE_TYPE enum.
//
typedef enum _NDIS_INTERFACE_TYPE
{
	NdisInterfaceInternal = Internal,
	NdisInterfaceIsa = Isa,
	NdisInterfaceEisa = Eisa,
	NdisInterfaceMca = MicroChannel,
	NdisInterfaceTurboChannel = TurboChannel,
	NdisInterfacePci = PCIBus,
	NdisInterfacePcMcia = PCMCIABus
} NDIS_INTERFACE_TYPE, *PNDIS_INTERFACE_TYPE;

//
// Definition for shutdown handler
//

typedef
VOID
(*ADAPTER_SHUTDOWN_HANDLER) (
	IN	PVOID ShutdownContext
	);

//
// Stuff for PCI configuring
//

typedef CM_PARTIAL_RESOURCE_LIST NDIS_RESOURCE_LIST, *PNDIS_RESOURCE_LIST;


//
// The structure passed up on a WAN_LINE_UP indication
//

typedef struct _NDIS_WAN_LINE_UP
{
	IN ULONG				LinkSpeed;			// 100 bps units
	IN ULONG				MaximumTotalSize;	// suggested max for send packets
	IN NDIS_WAN_QUALITY		Quality;
	IN USHORT				SendWindow;			// suggested by the MAC
	IN UCHAR				RemoteAddress[6];
	IN OUT UCHAR			LocalAddress[6];
	IN ULONG				ProtocolBufferLength;	// Length of protocol info buffer
	IN PUCHAR				ProtocolBuffer;		// Information used by protocol
	IN USHORT				ProtocolType;		// Protocol ID
	IN OUT NDIS_STRING		DeviceName;
} NDIS_WAN_LINE_UP, *PNDIS_WAN_LINE_UP;

//
// The structure passed up on a WAN_LINE_DOWN indication
//

typedef struct _NDIS_WAN_LINE_DOWN
{
	IN UCHAR	RemoteAddress[6];
	IN UCHAR	LocalAddress[6];
} NDIS_WAN_LINE_DOWN, *PNDIS_WAN_LINE_DOWN;

//
// The structure passed up on a WAN_FRAGMENT indication
//

typedef struct _NDIS_WAN_FRAGMENT
{
	IN UCHAR	RemoteAddress[6];
	IN UCHAR	LocalAddress[6];
} NDIS_WAN_FRAGMENT, *PNDIS_WAN_FRAGMENT;

//
// The structure passed up on a WAN_GET_STATS indication
//

typedef struct _NDIS_WAN_GET_STATS
{
	IN  UCHAR	LocalAddress[6];
	OUT ULONG	BytesSent;
	OUT ULONG	BytesRcvd;
	OUT ULONG	FramesSent;
	OUT ULONG	FramesRcvd;
	OUT ULONG	CRCErrors;						// Serial-like info only
	OUT ULONG	TimeoutErrors;					// Serial-like info only
	OUT ULONG	AlignmentErrors;				// Serial-like info only
	OUT ULONG	SerialOverrunErrors;			// Serial-like info only
	OUT ULONG	FramingErrors;					// Serial-like info only
	OUT ULONG	BufferOverrunErrors;			// Serial-like info only
	OUT ULONG	BytesTransmittedUncompressed;	// Compression info only
	OUT ULONG	BytesReceivedUncompressed;		// Compression info only
	OUT ULONG	BytesTransmittedCompressed;	 	// Compression info only
	OUT ULONG	BytesReceivedCompressed;		// Compression info only
} NDIS_WAN_GET_STATS, *PNDIS_WAN_GET_STATS;


//
// DMA Channel information
//
typedef struct _NDIS_DMA_DESCRIPTION
{
	BOOLEAN		DemandMode;
	BOOLEAN		AutoInitialize;
	BOOLEAN		DmaChannelSpecified;
	DMA_WIDTH	DmaWidth;
	DMA_SPEED	DmaSpeed;
	ULONG		DmaPort;
	ULONG		DmaChannel;
} NDIS_DMA_DESCRIPTION, *PNDIS_DMA_DESCRIPTION;

//
// Internal structure representing an NDIS DMA channel
//
typedef struct _NDIS_DMA_BLOCK
{
	PVOID				MapRegisterBase;
	KEVENT				AllocationEvent;
	PADAPTER_OBJECT		SystemAdapterObject;
	BOOLEAN				InProgress;
} NDIS_DMA_BLOCK, *PNDIS_DMA_BLOCK;



#if defined(NDIS_WRAPPER)
//
// definitions for PnP state
//

typedef enum _NDIS_PNP_DEVICE_STATE
{
	NdisPnPDeviceAdded,
	NdisPnPDeviceStarted,
	NdisPnPDeviceQueryStopped,
	NdisPnPDeviceStopped,
	NdisPnPDeviceQueryRemoved,
	NdisPnPDeviceRemoved,
	NdisPnPDeviceSurpriseRemoved
} NDIS_PNP_DEVICE_STATE;

//
// flags to use in PnPCapabilities Flag
//
#define	NDIS_DEVICE_NOT_STOPPABLE					0x00000001		// the debvice is not stoppable i.e. ISA
#define	NDIS_DEVICE_NOT_REMOVEABLE					0x00000002		// the device can not be safely removed
#define	NDIS_DEVICE_NOT_SUSPENDABLE					0x00000004		// the device can not be safely suspended
#define NDIS_DEVICE_DISABLE_PM						0x00000008		// disable all PM features
#define NDIS_DEVICE_DISABLE_WAKE_UP					0x00000010		// disable device waking up the system
#define NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT		0x00000020		// disable device waking up the -system- due to a cable re-connect
#ifdef NDIS_WOL_QFE
#define NDIS_DEVICE_RESERVED						0x00000040		// should not be used
#define NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET	0x00000080		// disable device waking up the -system- due to a magic packet
#define NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH	0x00000100		// disable device waking up the -system- due to a pattern match
#endif

#endif // NDIS_WRAPPER defined



//
// Ndis Buffer is actually an Mdl
//
typedef MDL NDIS_BUFFER, *PNDIS_BUFFER;

struct _NDIS_PACKET;
typedef	NDIS_HANDLE	PNDIS_PACKET_POOL;

//
//
// wrapper-specific part of a packet
//

typedef struct _NDIS_PACKET_PRIVATE
{
	UINT				PhysicalCount;	// number of physical pages in packet.
	UINT				TotalLength;	// Total amount of data in the packet.
	PNDIS_BUFFER		Head;			// first buffer in the chain
	PNDIS_BUFFER		Tail;			// last buffer in the chain

	// if Head is NULL the chain is empty; Tail doesn't have to be NULL also

	PNDIS_PACKET_POOL	Pool;			// so we know where to free it back to
	UINT				Count;
	ULONG				Flags;			
	BOOLEAN				ValidCounts;
	UCHAR				NdisPacketFlags;	// See fPACKET_xxx bits below
	USHORT				NdisPacketOobOffset;
} NDIS_PACKET_PRIVATE, * PNDIS_PACKET_PRIVATE;

//
// The bits define the bits in the NDIS flags
//
#define	NDIS_FLAGS_PROTOCOL_ID_MASK				0x0000000F	// The low 4 bits are defined for protocol-id
															// The values are defined in ntddndis.h
#define	NDIS_FLAGS_MULTICAST_PACKET				0x00000010
#define	NDIS_FLAGS_BROADCAST_PACKET				0x00000020
#define	NDIS_FLAGS_DIRECTED_PACKET				0x00000040
#define	NDIS_FLAGS_DONT_LOOPBACK				0x00000080
#define	NDIS_FLAGS_IS_LOOPBACK_PACKET			0x00000100	// Valid on receive path only
#define	NDIS_FLAGS_LOOPBACK_ONLY				0x00000200
#define	NDIS_FLAGS_SKIP_LOOPBACK				0x00000400	// Internal use only

//
// Low-bits in the NdisPacketFlags are reserved by NDIS Wrapper for internal use
//
#define fPACKET_WRAPPER_RESERVED				0x3F
#define fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO	0x40
#define	fPACKET_ALLOCATED_BY_NDIS				0x80

//
// Definition for layout of the media-specific data. More than one class of media-specific
// information can be tagged onto a packet.
//
typedef enum _NDIS_CLASS_ID
{
	NdisClass802_3Priority,
	NdisClassWirelessWanMbxMailbox,
	NdisClassIrdaPacketInfo,
	NdisClassAtmAALInfo

} NDIS_CLASS_ID;

typedef struct _MEDIA_SPECIFIC_INFORMATION
{
	UINT			NextEntryOffset;
	NDIS_CLASS_ID	ClassId;
	UINT			Size;
	UCHAR			ClassInformation[1];

} MEDIA_SPECIFIC_INFORMATION, *PMEDIA_SPECIFIC_INFORMATION;

typedef struct _NDIS_PACKET_OOB_DATA
{
	union
	{
		ULONGLONG	TimeToSend;
		ULONGLONG	TimeSent;
	};
	ULONGLONG		TimeReceived;
	UINT			HeaderSize;
	UINT			SizeMediaSpecificInfo;
	PVOID			MediaSpecificInformation;

	NDIS_STATUS		Status;
} NDIS_PACKET_OOB_DATA, *PNDIS_PACKET_OOB_DATA;

#define	NDIS_GET_PACKET_PROTOCOL_TYPE(_Packet_)	((_Packet_)->Private.Flags & NDIS_PROTOCOL_ID_MASK)
	
#define NDIS_OOB_DATA_FROM_PACKET(_p)									\
						(PNDIS_PACKET_OOB_DATA)((PUCHAR)(_p) +			\
						(_p)->Private.NdisPacketOobOffset)
#define NDIS_GET_PACKET_HEADER_SIZE(_Packet)							\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->HeaderSize
#define NDIS_GET_PACKET_STATUS(_Packet)									\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->Status
#define	NDIS_GET_PACKET_TIME_TO_SEND(_Packet)							\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeToSend
#define	NDIS_GET_PACKET_TIME_SENT(_Packet)								\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeSent
#define NDIS_GET_PACKET_TIME_RECEIVED(_Packet)							\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeReceived
#define NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(_Packet,					\
											_pMediaSpecificInfo,		\
											_pSizeMediaSpecificInfo)	\
{																		\
	if (!((_Packet)->Private.NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS) ||\
		!((_Packet)->Private.NdisPacketFlags & fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO))\
	{																	\
		*(_pMediaSpecificInfo) = NULL;									\
		*(_pSizeMediaSpecificInfo) = 0;									\
	}																	\
	else																\
	{																	\
		*(_pMediaSpecificInfo) =((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +\
					(_Packet)->Private.NdisPacketOobOffset))->MediaSpecificInformation;\
		*(_pSizeMediaSpecificInfo) = ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +\
					(_Packet)->Private.NdisPacketOobOffset))->SizeMediaSpecificInfo;\
	}																	\
}

#define NDIS_SET_PACKET_HEADER_SIZE(_Packet, _HdrSize)					\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->HeaderSize = (_HdrSize)
#define NDIS_SET_PACKET_STATUS(_Packet, _Status)						\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->Status = (_Status)
#define	NDIS_SET_PACKET_TIME_TO_SEND(_Packet, _TimeToSend)				\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeToSend = (_TimeToSend)
#define	NDIS_SET_PACKET_TIME_SENT(_Packet, _TimeSent)					\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeSent = (_TimeSent)
#define NDIS_SET_PACKET_TIME_RECEIVED(_Packet, _TimeReceived)			\
						((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +	\
						(_Packet)->Private.NdisPacketOobOffset))->TimeReceived = (_TimeReceived)
#define NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(_Packet,					\
											_MediaSpecificInfo,			\
											_SizeMediaSpecificInfo)		\
{																		\
	if ((_Packet)->Private.NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS)	\
	{																	\
		(_Packet)->Private.NdisPacketFlags |= fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO;\
		((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +					\
										  (_Packet)->Private.NdisPacketOobOffset))->MediaSpecificInformation = (_MediaSpecificInfo);\
		((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) +					\
										  (_Packet)->Private.NdisPacketOobOffset))->SizeMediaSpecificInfo = (_SizeMediaSpecificInfo);\
	}																	\
}

//
// packet definition
//
typedef struct _NDIS_PACKET
{
	NDIS_PACKET_PRIVATE	Private;

	union
	{
		struct					// For Connection-less miniports
		{
			UCHAR	MiniportReserved[2*sizeof(PVOID)];
			UCHAR	WrapperReserved[2*sizeof(PVOID)];
		};

		struct
		{
			//
			// For de-serialized miniports. And by implication conn-oriented miniports.
			// This is for the send-path only. Packets indicated will use WrapperReserved
			// instead of WrapperReservedEx
			//
			UCHAR	MiniportReservedEx[3*sizeof(PVOID)];
			UCHAR	WrapperReservedEx[sizeof(PVOID)];
		};

		struct
		{
			UCHAR	MacReserved[4*sizeof(PVOID)];
		};
	};

	ULONG_PTR		Reserved[2];			// For compatibility with Win95
	UCHAR			ProtocolReserved[1];

} NDIS_PACKET, *PNDIS_PACKET, **PPNDIS_PACKET;

//
//	NDIS per-packet information.
//
typedef enum _NDIS_PER_PACKET_INFO
{
	TcpIpChecksumPacketInfo,
	IpSecPacketInfo,
	TcpLargeSendPacketInfo,
	ClassificationHandlePacketInfo,
	HeaderIndexInfo,				// Internal NDIS use only
	ScatterGatherListPacketInfo,
	Ieee8021pPriority,
	OriginalPacketInfo,
	NdisInternalExtension1,			// Internal NDIS use only
	NdisInternalExtension2,			// Internal NDIS use only
#if	PKT_DBG
	NdisInternalPktDebug,			// Internal NDIS use only
#endif
	MaxPerPacketInfo
} NDIS_PER_PACKET_INFO, *PNDIS_PER_PACKET_INFO;

typedef struct _NDIS_PACKET_EXTENSION
{
 	PVOID		NdisPacketInfo[MaxPerPacketInfo];
} NDIS_PACKET_EXTENSION, *PNDIS_PACKET_EXTENSION;

#define	NDIS_PACKET_EXTENSION_FROM_PACKET(_P)		((PNDIS_PACKET_EXTENSION)((PUCHAR)(_P) + (_P)->Private.NdisPacketOobOffset + sizeof(NDIS_PACKET_OOB_DATA)))
#define	NDIS_PER_PACKET_INFO_FROM_PACKET(_P, _Id)	((PNDIS_PACKET_EXTENSION)((PUCHAR)(_P) + (_P)->Private.NdisPacketOobOffset + sizeof(NDIS_PACKET_OOB_DATA)))->NdisPacketInfo[(_Id)]
#define	NDIS_GET_ORIGINAL_PACKET(_P)				NDIS_PER_PACKET_INFO_FROM_PACKET(_P, OriginalPacketInfo)
#define	NDIS_SET_ORIGINAL_PACKET(_P, _OP)			NDIS_PER_PACKET_INFO_FROM_PACKET(_P, OriginalPacketInfo) = _OP

//
//	Per-packet information for TcpIpChecksumPacketInfo.
//
typedef struct _NDIS_TCP_IP_CHECKSUM_PACKET_INFO
{
	union
	{
		struct
		{	
			ULONG	NdisPacketChecksumV4:1;
			ULONG	NdisPacketChecksumV6:1;
			ULONG	NdisPacketTcpChecksum:1;
			ULONG	NdisPacketUdpChecksum:1;
			ULONG	NdisPacketIpChecksum:1;
		} Transmit;

		struct
		{
			ULONG	NdisPacketTcpChecksumFailed:1;
			ULONG	NdisPacketUdpChecksumFailed:1;
			ULONG	NdisPacketIpChecksumFailed:1;
			ULONG	NdisPacketTcpChecksumSucceeded:1;
			ULONG	NdisPacketUdpChecksumSucceeded:1;
			ULONG	NdisPacketIpChecksumSucceeded:1;
			ULONG	NdisPacketLoopback:1;
		} Receive;

		ULONG	Value;
	};
} NDIS_TCP_IP_CHECKSUM_PACKET_INFO, *PNDIS_TCP_IP_CHECKSUM_PACKET_INFO;



#define MAX_HASHES			4
#define TRUNCATED_HASH_LEN	12

#define CRYPTO_SUCCESS						0
#define CRYPTO_GENERIC_ERROR				1
#define CRYPTO_TRANSPORT_AH_AUTH_FAILED		2
#define CRYPTO_TRANSPORT_ESP_AUTH_FAILED	3
#define CRYPTO_TUNNEL_AH_AUTH_FAILED		4
#define CRYPTO_TUNNEL_ESP_AUTH_FAILED		5
#define CRYPTO_INVALID_PACKET_SYNTAX		6
#define CRYPTO_INVALID_PROTOCOL				7

typedef struct _NDIS_IPSEC_PACKET_INFO
{
	union
	{
		struct
		{
			NDIS_HANDLE	OffloadHandle; 	
			NDIS_HANDLE	NextOffloadHandle;

		} Transmit;

		struct
		{
			ULONG	SA_DELETE_REQ:1;		
			ULONG	CRYPTO_DONE:1;
			ULONG	NEXT_CRYPTO_DONE:1;
			ULONG	CryptoStatus;
		} Receive;
	};
} NDIS_IPSEC_PACKET_INFO, *PNDIS_IPSEC_PACKET_INFO;


///
//	NDIS Task Off-Load data structures.
///

#define NDIS_TASK_OFFLOAD_VERSION 1

//
//	The following defines are used in the Task field above to define
//	the type of task offloading necessary.
//
typedef enum _NDIS_TASK
{
	TcpIpChecksumNdisTask,
	IpSecNdisTask,
	TcpLargeSendNdisTask,
	MaxNdisTask
} NDIS_TASK, *PNDIS_TASK;

typedef enum _NDIS_ENCAPSULATION
{
	UNSPECIFIED_Encapsulation,
	NULL_Encapsulation,
	IEEE_802_3_Encapsulation,
	IEEE_802_5_Encapsulation,
	LLC_SNAP_ROUTED_Encapsulation,
	LLC_SNAP_BRIDGED_Encapsulation

} NDIS_ENCAPSULATION;

//
// Encapsulation header format
//
typedef struct _NDIS_ENCAPSULATION_FORMAT
{
	NDIS_ENCAPSULATION	Encapsulation;				// Encapsulation type
	struct
	{
		ULONG	FixedHeaderSize:1;
		ULONG	Reserved:31;
	} Flags;

	ULONG	 EncapsulationHeaderSize;				// Encapsulation header size

} NDIS_ENCAPSULATION_FORMAT,*PNDIS_ENCAPSULATION_FORMAT;


//
// OFFLOAD header structure for OID_TCP_TASK_OFFLOAD
//
typedef struct _NDIS_TASK_OFFLOAD_HEADER
{
	ULONG		Version;							// set to NDIS_TASK_OFFLOAD_VERSION
	ULONG		Size;								// Size of this structure
	ULONG		Reserved;							// Reserved for future use
	ULONG		OffsetFirstTask;					// Offset to the first
	NDIS_ENCAPSULATION_FORMAT  EncapsulationFormat; // Encapsulation information.
													// NDIS_TASK_OFFLOAD structure(s)

} NDIS_TASK_OFFLOAD_HEADER, *PNDIS_TASK_OFFLOAD_HEADER;


//
//	Task offload Structure, which follows the above header in ndis query
//
typedef struct _NDIS_TASK_OFFLOAD
{
	ULONG		Version;							// NDIS_TASK_OFFLOAD_VERSION
	ULONG		Size;								//	Size of this structure. Used for version checking.
	NDIS_TASK	Task;								//	Task.
	ULONG		OffsetNextTask;						//	Offset to the next NDIS_TASK_OFFLOAD
	ULONG		TaskBufferLength;					//	Length of the task offload information.
	UCHAR		TaskBuffer[1];						//	The task offload information.
} NDIS_TASK_OFFLOAD, *PNDIS_TASK_OFFLOAD;

//
//	Offload structure for NDIS_TASK_TCP_IP_CHECKSUM
//
typedef struct _NDIS_TASK_TCP_IP_CHECKSUM
{
	struct
	{
		ULONG		IpOptionsSupported:1;
		ULONG		TcpOptionsSupported:1;
		ULONG		TcpChecksum:1;
		ULONG		UdpChecksum:1;
		ULONG		IpChecksum:1;
	} V4Transmit;

	struct
	{	
		ULONG		IpOptionsSupported:1;
		ULONG		TcpOptionsSupported:1;
		ULONG		TcpChecksum:1;
		ULONG		UdpChecksum:1;
		ULONG		IpChecksum:1;
	} V4Receive;


	struct
	{
		ULONG		IpOptionsSupported:1;
		ULONG		TcpOptionsSupported:1;
		ULONG		TcpChecksum:1;
		ULONG		UdpChecksum:1;

	} V6Transmit;

	struct
	{	
		ULONG		IpOptionsSupported:1;
		ULONG		TcpOptionsSupported:1;
		ULONG		TcpChecksum:1;
		ULONG		UdpChecksum:1;
		
	} V6Receive;


} NDIS_TASK_TCP_IP_CHECKSUM, *PNDIS_TASK_TCP_IP_CHECKSUM;

//
//	Off-load structure for NDIS_TASK_TCP_LARGE_SEND
//
typedef struct _NDIS_TASK_TCP_LARGE_SEND
{
	ULONG	  Version;
	ULONG	  MaxOffLoadSize;
	ULONG		 MinSegmentCount;
	BOOLEAN	  TcpOptions;
	BOOLEAN	  IpOptions;

} NDIS_TASK_TCP_LARGE_SEND, *PNDIS_TASK_TCP_LARGE_SEND;


typedef struct _NDIS_TASK_IPSEC
{
	struct
	{
		ULONG	AH_ESP_COMBINED;
		ULONG	TRANSPORT_TUNNEL_COMBINED;
		ULONG	V4_OPTIONS;
		ULONG	RESERVED;
	} Supported;

	struct
	{
		ULONG	MD5:1;
		ULONG	SHA_1:1;
		ULONG	Transport:1;
		ULONG	Tunnel:1;
		ULONG	Send:1;
		ULONG	Receive:1;
	} V4AH;

	struct
	{
		ULONG	DES:1;
		ULONG	RESERVED:1;
		ULONG	TRIPLE_DES:1;
		ULONG	NULL_ESP:1;
		ULONG	Transport:1;
		ULONG	Tunnel:1;
		ULONG	Send:1;
		ULONG	Receive:1;
	} V4ESP;

} NDIS_TASK_IPSEC, *PNDIS_TASK_IPSEC;

typedef	UINT	IEEE8021PPRIORITY;

//
// WAN Packet. This is used by WAN miniports only. This is the legacy model.
// Co-Ndis is the preferred model for WAN miniports
//
typedef struct _NDIS_WAN_PACKET
{
	LIST_ENTRY			WanPacketQueue;
	PUCHAR				CurrentBuffer;
	ULONG				CurrentLength;
	PUCHAR				StartBuffer;
	PUCHAR				EndBuffer;
	PVOID				ProtocolReserved1;
	PVOID				ProtocolReserved2;
	PVOID				ProtocolReserved3;
	PVOID				ProtocolReserved4;
	PVOID				MacReserved1;
	PVOID				MacReserved2;
	PVOID				MacReserved3;
	PVOID				MacReserved4;
} NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;

//
// Routines to get/set packet flags
//

/*++

UINT
NdisGetPacketFlags(
	IN	PNDIS_PACKET	Packet
	);

--*/

#define NdisGetPacketFlags(_Packet) 		(_Packet)->Private.Flags

/*++

VOID
NdisSetPacketFlags(
	IN	PNDIS_PACKET Packet,
	IN	UINT Flags
	);

--*/

#define NdisSetPacketFlags(_Packet, _Flags)		(_Packet)->Private.Flags |= (_Flags)
#define NdisClearPacketFlags(_Packet, _Flags)	(_Packet)->Private.Flags &= ~(_Flags)

//
// Request types used by NdisRequest; constants are added for
// all entry points in the MAC, for those that want to create
// their own internal requests.
//

typedef enum _NDIS_REQUEST_TYPE
{
	NdisRequestQueryInformation,
	NdisRequestSetInformation,
	NdisRequestQueryStatistics,
	NdisRequestOpen,
	NdisRequestClose,
	NdisRequestSend,
	NdisRequestTransferData,
	NdisRequestReset,
	NdisRequestGeneric1,
	NdisRequestGeneric2,
	NdisRequestGeneric3,
	NdisRequestGeneric4
} NDIS_REQUEST_TYPE, *PNDIS_REQUEST_TYPE;


//
// Structure of requests sent via NdisRequest
//

typedef struct _NDIS_REQUEST
{
	UCHAR				MacReserved[4*sizeof(PVOID)];
	NDIS_REQUEST_TYPE	RequestType;
	union _DATA
	{
		struct _QUERY_INFORMATION
		{
			NDIS_OID	Oid;
			PVOID		InformationBuffer;
			UINT		InformationBufferLength;
			UINT		BytesWritten;
			UINT		BytesNeeded;
		} QUERY_INFORMATION;

		struct _SET_INFORMATION
		{
			NDIS_OID	Oid;
			PVOID		InformationBuffer;
			UINT		InformationBufferLength;
			UINT		BytesRead;
			UINT		BytesNeeded;
		} SET_INFORMATION;

	} DATA;
#if (defined(NDIS50) || defined(NDIS50_MINIPORT))
	UCHAR				NdisReserved[9*sizeof(PVOID)];
	union
	{
		UCHAR			CallMgrReserved[2*sizeof(PVOID)];
		UCHAR			ProtocolReserved[2*sizeof(PVOID)];
	};
	UCHAR				MiniportReserved[2*sizeof(PVOID)];
#endif
} NDIS_REQUEST, *PNDIS_REQUEST;


//
// NDIS Address Family definitions.
//
typedef ULONG			NDIS_AF, *PNDIS_AF;
#define CO_ADDRESS_FAMILY_Q2931				((NDIS_AF)0x1)	// ATM
#define CO_ADDRESS_FAMILY_PSCHED			((NDIS_AF)0x2)	// Packet scheduler
#define CO_ADDRESS_FAMILY_L2TP				((NDIS_AF)0x3)
#define CO_ADDRESS_FAMILY_IRDA				((NDIS_AF)0x4)
#define CO_ADDRESS_FAMILY_1394				((NDIS_AF)0x5)
#define CO_ADDRESS_FAMILY_PPP               ((NDIS_AF)0x6)
#define CO_ADDRESS_FAMILY_TAPI				((NDIS_AF)0x800)
#define CO_ADDRESS_FAMILY_TAPI_PROXY		((NDIS_AF)0x801)

//
// The following is OR'ed with the base AF to denote proxy support
//
#define CO_ADDRESS_FAMILY_PROXY				0x80000000


//
//	Address family structure registered/opened via
//		NdisCmRegisterAddressFamily
//		NdisClOpenAddressFamily
//
typedef struct
{
	NDIS_AF						AddressFamily;	// one of the CO_ADDRESS_FAMILY_xxx values above
	ULONG						MajorVersion;	// the major version of call manager
	ULONG						MinorVersion;	// the minor version of call manager
} CO_ADDRESS_FAMILY, *PCO_ADDRESS_FAMILY;

//
// Definition for a SAP
//
typedef struct
{
	ULONG						SapType;
	ULONG						SapLength;
	UCHAR						Sap[1];
} CO_SAP, *PCO_SAP;

//
// Definitions for physical address.
//

typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;
typedef struct _NDIS_PHYSICAL_ADDRESS_UNIT
{
	NDIS_PHYSICAL_ADDRESS		PhysicalAddress;
	UINT						Length;
} NDIS_PHYSICAL_ADDRESS_UNIT, *PNDIS_PHYSICAL_ADDRESS_UNIT;


/*++

ULONG
NdisGetPhysicalAddressHigh(
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress
	);

--*/

#define NdisGetPhysicalAddressHigh(_PhysicalAddress)			\
		((_PhysicalAddress).HighPart)

/*++

VOID
NdisSetPhysicalAddressHigh(
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress,
	IN	ULONG					Value
	);

--*/

#define NdisSetPhysicalAddressHigh(_PhysicalAddress, _Value)	\
	 ((_PhysicalAddress).HighPart) = (_Value)


/*++

ULONG
NdisGetPhysicalAddressLow(
	IN	NDIS_PHYSICAL_ADDRESS PhysicalAddress
	);

--*/

#define NdisGetPhysicalAddressLow(_PhysicalAddress)				\
	((_PhysicalAddress).LowPart)


/*++

VOID
NdisSetPhysicalAddressLow(
	IN	NDIS_PHYSICAL_ADDRESS	PhysicalAddress,
	IN	ULONG					Value
	);

--*/

#define NdisSetPhysicalAddressLow(_PhysicalAddress, _Value)		\
	((_PhysicalAddress).LowPart) = (_Value)

//
// Macro to initialize an NDIS_PHYSICAL_ADDRESS constant
//

#define NDIS_PHYSICAL_ADDRESS_CONST(_Low, _High)				\
	{ (ULONG)(_Low), (LONG)(_High) }


//
// block used for references...
//
typedef struct _REFERENCE
{
	KSPIN_LOCK					SpinLock;
	USHORT						ReferenceCount;
	BOOLEAN						Closing;
} REFERENCE, * PREFERENCE;


//
// This holds a map register entry.
//

typedef struct _MAP_REGISTER_ENTRY
{
	PVOID						MapRegister;
	BOOLEAN						WriteToDevice;
} MAP_REGISTER_ENTRY, * PMAP_REGISTER_ENTRY;

//
// Types of Memory (not mutually exclusive)
//

#define NDIS_MEMORY_CONTIGUOUS		0x00000001
#define NDIS_MEMORY_NONCACHED		0x00000002

//
// Open options
//
#define NDIS_OPEN_RECEIVE_NOT_REENTRANT	0x00000001

//
// NDIS_STATUS values
//

#define NDIS_STATUS_SUCCESS						((NDIS_STATUS)STATUS_SUCCESS)
#define NDIS_STATUS_PENDING						((NDIS_STATUS) STATUS_PENDING)
#define NDIS_STATUS_NOT_RECOGNIZED				((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED					((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED				((NDIS_STATUS)0x00010003L)
#define NDIS_STATUS_CALL_ACTIVE					((NDIS_STATUS)0x00010007L)

#define NDIS_STATUS_ONLINE						((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START					((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END					((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS					((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED						((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP					((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN				((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT				((NDIS_STATUS)0x4001000AL)
#define	NDIS_STATUS_MEDIA_CONNECT				((NDIS_STATUS)0x4001000BL)
#define	NDIS_STATUS_MEDIA_DISCONNECT			((NDIS_STATUS)0x4001000CL)
#define NDIS_STATUS_HARDWARE_LINE_UP			((NDIS_STATUS)0x4001000DL)
#define NDIS_STATUS_HARDWARE_LINE_DOWN			((NDIS_STATUS)0x4001000EL)
#define NDIS_STATUS_INTERFACE_UP				((NDIS_STATUS)0x4001000FL)
#define NDIS_STATUS_INTERFACE_DOWN				((NDIS_STATUS)0x40010010L)
#define NDIS_STATUS_MEDIA_BUSY					((NDIS_STATUS)0x40010011L)
#define	NDIS_STATUS_MEDIA_SPECIFIC_INDICATION	((NDIS_STATUS)0x40010012L)
#define	NDIS_STATUS_WW_INDICATION				NDIS_STATUS_MEDIA_SPECIFIC_INDICATION
#define NDIS_STATUS_LINK_SPEED_CHANGE			((NDIS_STATUS)0x40010013L)
#define NDIS_STATUS_WAN_GET_STATS				((NDIS_STATUS)0x40010014L)
#define NDIS_STATUS_WAN_CO_FRAGMENT             ((NDIS_STATUS)0x40010015L)
#define NDIS_STATUS_WAN_CO_LINKPARAMS           ((NDIS_STATUS)0x40010016L)

#define NDIS_STATUS_NOT_RESETTABLE				((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS					((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS					((NDIS_STATUS)0x80010004L)
#define NDIS_STATUS_BUFFER_OVERFLOW				((NDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define NDIS_STATUS_FAILURE						((NDIS_STATUS) STATUS_UNSUCCESSFUL)
#define NDIS_STATUS_RESOURCES					((NDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define NDIS_STATUS_CLOSING						((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION					((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS			((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND			((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED					((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED				((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL				((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS			((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND			((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED				((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS			((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING			((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED				((NDIS_STATUS)STATUS_NOT_SUPPORTED)
#define NDIS_STATUS_INVALID_PACKET				((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL				((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY			((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN			((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING				((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH				((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA				((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT			((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID					((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED				((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA			((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE		((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND				((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE			((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED				((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT			((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE					((NDIS_STATUS)0xC001001FL)

#define NDIS_STATUS_INVALID_SAP					((NDIS_STATUS)0xC0010020L)
#define NDIS_STATUS_SAP_IN_USE					((NDIS_STATUS)0xC0010021L)
#define NDIS_STATUS_INVALID_ADDRESS				((NDIS_STATUS)0xC0010022L)
#define NDIS_STATUS_VC_NOT_ACTIVATED			((NDIS_STATUS)0xC0010023L)
#define NDIS_STATUS_DEST_OUT_OF_ORDER			((NDIS_STATUS)0xC0010024L)	// cause 27
#define NDIS_STATUS_VC_NOT_AVAILABLE			((NDIS_STATUS)0xC0010025L)	// cause 35,45
#define NDIS_STATUS_CELLRATE_NOT_AVAILABLE		((NDIS_STATUS)0xC0010026L)	// cause 37
#define NDIS_STATUS_INCOMPATABLE_QOS			((NDIS_STATUS)0xC0010027L)	// cause 49
#define NDIS_STATUS_AAL_PARAMS_UNSUPPORTED		((NDIS_STATUS)0xC0010028L)	// cause 93
#define NDIS_STATUS_NO_ROUTE_TO_DESTINATION		((NDIS_STATUS)0xC0010029L)	// cause 3

#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR		((NDIS_STATUS)0xC0011000L)
#define	NDIS_STATUS_INVALID_DEVICE_REQUEST		((NDIS_STATUS)STATUS_INVALID_DEVICE_REQUEST)
#define	NDIS_STATUS_NETWORK_UNREACHABLE			((NDIS_STATUS)STATUS_NETWORK_UNREACHABLE)


//
// used in error logging
//

#define NDIS_ERROR_CODE ULONG

#define NDIS_ERROR_CODE_RESOURCE_CONFLICT			EVENT_NDIS_RESOURCE_CONFLICT
#define NDIS_ERROR_CODE_OUT_OF_RESOURCES			EVENT_NDIS_OUT_OF_RESOURCE
#define NDIS_ERROR_CODE_HARDWARE_FAILURE			EVENT_NDIS_HARDWARE_FAILURE
#define NDIS_ERROR_CODE_ADAPTER_NOT_FOUND			EVENT_NDIS_ADAPTER_NOT_FOUND
#define NDIS_ERROR_CODE_INTERRUPT_CONNECT			EVENT_NDIS_INTERRUPT_CONNECT
#define NDIS_ERROR_CODE_DRIVER_FAILURE				EVENT_NDIS_DRIVER_FAILURE
#define NDIS_ERROR_CODE_BAD_VERSION					EVENT_NDIS_BAD_VERSION
#define NDIS_ERROR_CODE_TIMEOUT						EVENT_NDIS_TIMEOUT
#define NDIS_ERROR_CODE_NETWORK_ADDRESS				EVENT_NDIS_NETWORK_ADDRESS
#define NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION	EVENT_NDIS_UNSUPPORTED_CONFIGURATION
#define NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER	EVENT_NDIS_INVALID_VALUE_FROM_ADAPTER
#define NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER	EVENT_NDIS_MISSING_CONFIGURATION_PARAMETER
#define NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS			EVENT_NDIS_BAD_IO_BASE_ADDRESS
#define NDIS_ERROR_CODE_RECEIVE_SPACE_SMALL			EVENT_NDIS_RECEIVE_SPACE_SMALL
#define NDIS_ERROR_CODE_ADAPTER_DISABLED			EVENT_NDIS_ADAPTER_DISABLED

#if BINARY_COMPATIBLE

#if	USE_KLOCKS

#define	DISPATCH_LEVEL		2

#define NdisAllocateSpinLock(_SpinLock)	KeInitializeSpinLock(&(_SpinLock)->SpinLock)

#define NdisFreeSpinLock(_SpinLock)

#define NdisAcquireSpinLock(_SpinLock)	KeAcquireSpinLock(&(_SpinLock)->SpinLock, &(_SpinLock)->OldIrql)

#define NdisReleaseSpinLock(_SpinLock)	KeReleaseSpinLock(&(_SpinLock)->SpinLock,(_SpinLock)->OldIrql)

#define NdisDprAcquireSpinLock(_SpinLock)						\
{																\
	KeAcquireSpinLockAtDpcLevel(&(_SpinLock)->SpinLock);		\
	(_SpinLock)->OldIrql = DISPATCH_LEVEL;						\
}

#define NdisDprReleaseSpinLock(_SpinLock) KeReleaseSpinLockFromDpcLevel(&(_SpinLock)->SpinLock)

#else

//
// Ndis Spin Locks
//
EXPORT
VOID
NdisAllocateSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
VOID
NdisFreeSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
VOID
NdisAcquireSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
VOID
NdisReleaseSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
VOID
NdisDprAcquireSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
VOID
NdisDprReleaseSpinLock(
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

#endif


EXPORT
VOID
NdisGetCurrentSystemTime(
	PLARGE_INTEGER				pSystemTime
	);

//
// Interlocked support functions
//

EXPORT
ULONG
NdisInterlockedIncrement(
	IN	PLONG					Addend
	);

EXPORT
ULONG
NdisInterlockedDecrement(
	IN	PLONG					Addend
	);

EXPORT
VOID
NdisInterlockedAddUlong(
	IN	PULONG					Addend,
	IN	ULONG					Increment,
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
PLIST_ENTRY
NdisInterlockedInsertHeadList(
	IN	PLIST_ENTRY				ListHead,
	IN	PLIST_ENTRY				ListEntry,
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


EXPORT
PLIST_ENTRY
NdisInterlockedInsertTailList(
	IN	PLIST_ENTRY				ListHead,
	IN	PLIST_ENTRY				ListEntry,
	IN	PNDIS_SPIN_LOCK			SpinLock
	);


EXPORT
PLIST_ENTRY
NdisInterlockedRemoveHeadList(
	IN	PLIST_ENTRY				ListHead,
	IN	PNDIS_SPIN_LOCK			SpinLock
	);

EXPORT
LARGE_INTEGER
NdisInterlockedAddLargeInteger(
	IN	PLARGE_INTEGER			Addend,
	IN	ULONG					Increment,
	IN	PKSPIN_LOCK				Lock
	);

#else // BINARY_COMPATIBLE

#define NdisAllocateSpinLock(_SpinLock)	KeInitializeSpinLock(&(_SpinLock)->SpinLock)

#define NdisFreeSpinLock(_SpinLock)

#define NdisAcquireSpinLock(_SpinLock)	KeAcquireSpinLock(&(_SpinLock)->SpinLock, &(_SpinLock)->OldIrql)

#define NdisReleaseSpinLock(_SpinLock)	KeReleaseSpinLock(&(_SpinLock)->SpinLock,(_SpinLock)->OldIrql)

#define NdisDprAcquireSpinLock(_SpinLock)									\
{																			\
	KeAcquireSpinLockAtDpcLevel(&(_SpinLock)->SpinLock);					\
	(_SpinLock)->OldIrql = DISPATCH_LEVEL;									\
}

#define NdisDprReleaseSpinLock(_SpinLock) KeReleaseSpinLockFromDpcLevel(&(_SpinLock)->SpinLock)

#define	NdisGetCurrentSystemTime(_pSystemTime)								\
	{																		\
		KeQuerySystemTime(_pSystemTime);									\
	}

//
// Interlocked support functions
//

#define	NdisInterlockedIncrement(Addend)	InterlockedIncrement(Addend)

#define	NdisInterlockedDecrement(Addend)	InterlockedDecrement(Addend)

#define NdisInterlockedAddUlong(_Addend, _Increment, _SpinLock) \
	ExInterlockedAddUlong(_Addend, _Increment, &(_SpinLock)->SpinLock)

#define NdisInterlockedInsertHeadList(_ListHead, _ListEntry, _SpinLock) \
	ExInterlockedInsertHeadList(_ListHead, _ListEntry, &(_SpinLock)->SpinLock)

#define NdisInterlockedInsertTailList(_ListHead, _ListEntry, _SpinLock) \
	ExInterlockedInsertTailList(_ListHead, _ListEntry, &(_SpinLock)->SpinLock)

#define NdisInterlockedRemoveHeadList(_ListHead, _SpinLock) \
	ExInterlockedRemoveHeadList(_ListHead, &(_SpinLock)->SpinLock)

#define	NdisInterlockedPushEntryList(ListHead, ListEntry, Lock) \
	ExInterlockedPushEntryList(ListHead, ListEntry, &(Lock)->SpinLock)

#define	NdisInterlockedPopEntryList(ListHead, Lock) \
	ExInterlockedPopEntryList(ListHead, &(Lock)->SpinLock)

#endif // BINARY_COMPATIBLE

#ifndef	MAXIMUM_PROCESSORS
#define	MAXIMUM_PROCESSORS	32
#endif

typedef union _NDIS_RW_LOCK_REFCOUNT
{
	UINT 						RefCount;
	UCHAR 						cacheLine[16];	// One refCount per cache line
} NDIS_RW_LOCK_REFCOUNT;

typedef struct _NDIS_RW_LOCK
{
	union
	{
		struct
		{
			KSPIN_LOCK			SpinLock;
			PVOID				Context;
		};
		UCHAR					Reserved[16];
	};

	NDIS_RW_LOCK_REFCOUNT		RefCount[MAXIMUM_PROCESSORS];
} NDIS_RW_LOCK, *PNDIS_RW_LOCK;

typedef struct _LOCK_STATE
{
	USHORT						LockState;
	KIRQL						OldIrql;
} LOCK_STATE, *PLOCK_STATE;


EXPORT
VOID
NdisInitializeReadWriteLock(
	IN	PNDIS_RW_LOCK			Lock
	);


EXPORT
VOID
NdisAcquireReadWriteLock(
	IN	PNDIS_RW_LOCK			Lock,
	IN	BOOLEAN					fWrite,			// TRUE	-> Write, FALSE -> Read
	IN	PLOCK_STATE				LockState
	);


EXPORT
VOID
NdisReleaseReadWriteLock(
	IN	PNDIS_RW_LOCK			Lock,
	IN	PLOCK_STATE				LockState
	);


#define	NdisInterlockedAddLargeStatistic(_Addend, _Increment)	\
	ExInterlockedAddLargeStatistic((PLARGE_INTEGER)_Addend, _Increment)

//
// S-List support
//

#define	NdisInterlockedPushEntrySList(SListHead, SListEntry, Lock) \
	ExInterlockedPushEntrySList(SListHead, SListEntry, &(Lock)->SpinLock)

#define	NdisInterlockedPopEntrySList(SListHead, Lock) \
	ExInterlockedPopEntrySList(SListHead, &(Lock)->SpinLock)

#define	NdisInterlockedFlushSList(SListHead) ExInterlockedFlushSList(SListHead)

#define	NdisInitializeSListHead(SListHead)	ExInitializeSListHead(SListHead)

#define	NdisQueryDepthSList(SListHead)	ExQueryDepthSList(SListHead)

EXPORT
VOID
NdisGetCurrentProcessorCpuUsage(
	OUT	PULONG					pCpuUsage
	);

EXPORT
VOID
NdisGetCurrentProcessorCounts(
	OUT	PULONG					pIdleCount,
	OUT	PULONG					pKernelAndUser,
	OUT	PULONG					pIndex
	);

EXPORT
VOID
NdisGetSystemUpTime(
	OUT	PULONG					pSystemUpTime
	);

//
// List manipulation
//

/*++

VOID
NdisInitializeListHead(
	IN	PLIST_ENTRY ListHead
	);

--*/
#define NdisInitializeListHead(_ListHead) InitializeListHead(_ListHead)


//
// Configuration Requests
//

EXPORT
VOID
NdisOpenConfiguration(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			ConfigurationHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext
	);

EXPORT
VOID
NdisOpenConfigurationKeyByName(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ConfigurationHandle,
	IN	PNDIS_STRING			SubKeyName,
	OUT PNDIS_HANDLE			SubKeyHandle
	);

EXPORT
VOID
NdisOpenConfigurationKeyByIndex(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ConfigurationHandle,
	IN	ULONG					Index,
	OUT	PNDIS_STRING			KeyName,
	OUT PNDIS_HANDLE			KeyHandle
	);

EXPORT
VOID
NdisReadConfiguration(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_CONFIGURATION_PARAMETER *ParameterValue,
	IN	NDIS_HANDLE				ConfigurationHandle,
	IN	PNDIS_STRING			Keyword,
	IN	NDIS_PARAMETER_TYPE		ParameterType
	);

EXPORT
VOID
NdisWriteConfiguration(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ConfigurationHandle,
	IN	PNDIS_STRING			Keyword,
	IN	PNDIS_CONFIGURATION_PARAMETER ParameterValue
	);

EXPORT
VOID
NdisCloseConfiguration(
	IN	NDIS_HANDLE				ConfigurationHandle
	);

EXPORT
VOID
NdisReadNetworkAddress(
	OUT PNDIS_STATUS			Status,
	OUT PVOID *					NetworkAddress,
	OUT PUINT					NetworkAddressLength,
	IN	NDIS_HANDLE				ConfigurationHandle
	);

EXPORT
VOID
NdisReadEisaSlotInformation(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	OUT PUINT					SlotNumber,
	OUT PNDIS_EISA_FUNCTION_INFORMATION EisaData
	);

EXPORT
VOID
NdisReadEisaSlotInformationEx(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	OUT PUINT					SlotNumber,
	OUT PNDIS_EISA_FUNCTION_INFORMATION *EisaData,
	OUT PUINT					NumberOfFunctions
	);

EXPORT
VOID
NdisReadMcaPosInformation(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	PUINT					ChannelNumber,
	OUT PNDIS_MCA_POS_DATA		McaData
	);

EXPORT
ULONG
NdisReadPciSlotInformation(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

EXPORT
ULONG
NdisWritePciSlotInformation(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

EXPORT
NDIS_STATUS
NdisPciAssignResources(
	IN	NDIS_HANDLE				NdisMacHandle,
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SlotNumber,
	OUT PNDIS_RESOURCE_LIST *	AssignedResources
	);

EXPORT
ULONG
NdisReadPcmciaAttributeMemory(
	IN NDIS_HANDLE				NdisAdapterHandle,
	IN ULONG					Offset,
	IN PVOID					Buffer,
	IN ULONG					Length
	);
	
EXPORT
ULONG
NdisWritePcmciaAttributeMemory(
	IN NDIS_HANDLE				NdisAdapterHandle,
	IN ULONG					Offset,
	IN PVOID					Buffer,
	IN ULONG					Length
	);

//
// Buffer Pool
//

EXPORT
VOID
NdisAllocateBufferPool(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			PoolHandle,
	IN	UINT					NumberOfDescriptors
	);

EXPORT
VOID
NdisFreeBufferPool(
	IN	NDIS_HANDLE				PoolHandle
	);

EXPORT
VOID
NdisAllocateBuffer(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_BUFFER *			Buffer,
	IN	NDIS_HANDLE				PoolHandle,
	IN	PVOID					VirtualAddress,
	IN	UINT					Length
	);

EXPORT
VOID
NdisCopyBuffer(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_BUFFER *			Buffer,
	IN	NDIS_HANDLE				PoolHandle,
	IN	PVOID					MemoryDescriptor,
	IN	UINT					Offset,
	IN	UINT					Length
	);


//
//	VOID
//	NdisCopyLookaheadData(
//		IN	PVOID					Destination,
//		IN	PVOID					Source,
//		IN	ULONG					Length,
//		IN	ULONG					ReceiveFlags
//		);
//

#ifdef _M_IX86
#define NdisCopyLookaheadData(_Destination, _Source, _Length, _MacOptions)	\
		RtlCopyMemory(_Destination, _Source, _Length)
#else
#define NdisCopyLookaheadData(_Destination, _Source, _Length, _MacOptions)	\
	{																		\
		if ((_MacOptions) & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA)			\
		{																	\
			RtlCopyMemory(_Destination, _Source, _Length);					\
		}																	\
		else																\
		{																	\
			PUCHAR _Src = (PUCHAR)(_Source);								\
			PUCHAR _Dest = (PUCHAR)(_Destination);							\
			PUCHAR _End = _Dest + (_Length);								\
			while (_Dest < _End)											\
			{																\
				*_Dest++ = *_Src++;											\
			}																\
		}																	\
	}
#endif

//
// Packet Pool
//
EXPORT
VOID
NdisAllocatePacketPool(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			PoolHandle,
	IN	UINT					NumberOfDescriptors,
	IN	UINT					ProtocolReservedLength
	);

EXPORT
VOID
NdisAllocatePacketPoolEx(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			PoolHandle,
	IN	UINT					NumberOfDescriptors,
	IN	UINT					NumberOfOverflowDescriptors,
	IN	UINT					ProtocolReservedLength
	);

EXPORT
VOID
NdisSetPacketPoolProtocolId(
	IN	NDIS_HANDLE				PacketPoolHandle,
	IN	UINT					ProtocolId
	);

EXPORT
UINT
NdisPacketPoolUsage(
	IN	NDIS_HANDLE				PoolHandle
	);

EXPORT
VOID
NdisFreePacketPool(
	IN	NDIS_HANDLE				PoolHandle
	);

EXPORT
VOID
NdisFreePacket(
	IN	PNDIS_PACKET			Packet
	);

EXPORT
VOID
NdisDprFreePacket(
	IN	PNDIS_PACKET			Packet
	);

EXPORT
VOID
NdisDprFreePacketNonInterlocked(
	IN	PNDIS_PACKET			Packet
	);


EXPORT
VOID
NdisAllocatePacket(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_PACKET *			Packet,
	IN	NDIS_HANDLE				PoolHandle
	);

EXPORT
VOID
NdisDprAllocatePacket(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_PACKET *			Packet,
	IN	NDIS_HANDLE				PoolHandle
	);

EXPORT
VOID
NdisDprAllocatePacketNonInterlocked(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_PACKET *			Packet,
	IN	NDIS_HANDLE				PoolHandle
	);

// VOID
// NdisReinitializePacket(
//	IN OUT PNDIS_PACKET			Packet
//	);
#define NdisReinitializePacket(Packet)										\
{																			\
	(Packet)->Private.Head = (PNDIS_BUFFER)NULL;							\
	(Packet)->Private.ValidCounts = FALSE;									\
}


#if BINARY_COMPATIBLE

EXPORT
VOID
NdisFreeBuffer(
	IN	PNDIS_BUFFER			Buffer
	);

EXPORT
VOID
NdisQueryBuffer(
	IN	PNDIS_BUFFER			Buffer,
	OUT PVOID *					VirtualAddress OPTIONAL,
	OUT PUINT					Length
	);

EXPORT
VOID
NdisQueryBufferSafe(
	IN	PNDIS_BUFFER			Buffer,
	OUT PVOID *					VirtualAddress OPTIONAL,
	OUT PUINT					Length,
	IN	UINT					Priority
	);

EXPORT
VOID
NdisQueryBufferOffset(
	IN	PNDIS_BUFFER			Buffer,
	OUT PUINT					Offset,
	OUT PUINT					Length
	);

//
// This is a combination of NdisQueryPacket and NdisQueryBuffer and
// optimized for protocols to get the first Buffer, its VA and its size.
//
VOID
NdisGetFirstBufferFromPacket(
	IN	PNDIS_PACKET			Packet,
	OUT PNDIS_BUFFER *			FirstBuffer,
	OUT PVOID *					FirstBufferVA,
	OUT PUINT					FirstBufferLength,
	OUT	PUINT					TotalBufferLength
	);

//
// This is used to determine how many physical pieces
// an NDIS_BUFFER will take up when mapped.
//

EXPORT
ULONG
NDIS_BUFFER_TO_SPAN_PAGES(
	IN	PNDIS_BUFFER				Buffer
	);

EXPORT
VOID
NdisGetBufferPhysicalArraySize(
	IN	PNDIS_BUFFER				Buffer,
	OUT PUINT						ArraySize
	);

#else // BINARY_COMPATIBLE

#define NdisFreeBuffer(Buffer)	IoFreeMdl(Buffer)

#define NdisQueryBuffer(_Buffer, _VirtualAddress, _Length)					\
{																			\
	if (ARGUMENT_PRESENT(_VirtualAddress))									\
	{																		\
		*(PVOID *)(_VirtualAddress) = MmGetSystemAddressForMdl(_Buffer);	\
	}																		\
	*(_Length) = MmGetMdlByteCount(_Buffer);								\
}

#define NdisQueryBufferSafe(_Buffer, _VirtualAddress, _Length, _Priority)	\
{																			\
	PVOID	_VA;															\
																			\
	_VA = MmGetSystemAddressForMdlSafe(_Buffer, _Priority);					\
	if (ARGUMENT_PRESENT(_VirtualAddress))									\
	{																		\
		*(PVOID *)(_VirtualAddress) = _VA;									\
	}																		\
	*(_Length) = (_VA != NULL) ? MmGetMdlByteCount(_Buffer) : 0;			\
}

#define NdisQueryBufferOffset(_Buffer, _Offset, _Length)					\
{																			\
	*(_Offset) = MmGetMdlByteOffset(_Buffer);								\
	*(_Length) = MmGetMdlByteCount(_Buffer);								\
}


#define	NdisGetFirstBufferFromPacket(_Packet,								\
									 _FirstBuffer,							\
									 _FirstBufferVA,						\
									 _FirstBufferLength,					\
									 _TotalBufferLength)					\
	{																		\
		PNDIS_BUFFER	_pBuf;												\
																			\
		_pBuf = (_Packet)->Private.Head;									\
		*(_FirstBuffer) = _pBuf;											\
		*(_FirstBufferVA) =	MmGetMdlVirtualAddress(_pBuf);					\
		*(_FirstBufferLength) =												\
		*(_TotalBufferLength) = MmGetMdlByteCount(_pBuf);					\
		for (_pBuf = _pBuf->Next;											\
			 _pBuf != NULL;													\
			 _pBuf = _pBuf->Next)											\
		{																		\
			*(_TotalBufferLength) += MmGetMdlByteCount(_pBuf);				\
		}																	\
	}

#define NDIS_BUFFER_TO_SPAN_PAGES(_Buffer)									\
	(MmGetMdlByteCount(_Buffer)==0 ?										\
				1 :															\
				(COMPUTE_PAGES_SPANNED(										\
						MmGetMdlVirtualAddress(_Buffer),					\
						MmGetMdlByteCount(_Buffer))))

#define NdisGetBufferPhysicalArraySize(Buffer, ArraySize)					\
	(*(ArraySize) = NDIS_BUFFER_TO_SPAN_PAGES(Buffer))

#endif // BINARY_COMPATIBLE


/*++

NDIS_BUFFER_LINKAGE(
	IN	PNDIS_BUFFER			Buffer
	);

--*/

#define NDIS_BUFFER_LINKAGE(Buffer)	((Buffer)->Next)


/*++

VOID
NdisRecalculatePacketCounts(
	IN OUT PNDIS_PACKET			Packet
	);

--*/

#define NdisRecalculatePacketCounts(Packet)									\
{																			\
	{																		\
		PNDIS_BUFFER TmpBuffer = (Packet)->Private.Head; 					\
		if (TmpBuffer)														\
		{																	\
			while (TmpBuffer->Next)											\
			{																\
				TmpBuffer = TmpBuffer->Next;								\
			}																\
			(Packet)->Private.Tail = TmpBuffer; 							\
		}																	\
		(Packet)->Private.ValidCounts = FALSE;								\
	}																		\
}


/*++

VOID
NdisChainBufferAtFront(
	IN OUT PNDIS_PACKET			Packet,
	IN OUT PNDIS_BUFFER			Buffer
	);

--*/

#define NdisChainBufferAtFront(Packet, Buffer)								\
{																			\
	PNDIS_BUFFER TmpBuffer = (Buffer);										\
																			\
	for (;;)																\
	{																		\
		if (TmpBuffer->Next == (PNDIS_BUFFER)NULL)							\
			break;															\
		TmpBuffer = TmpBuffer->Next;										\
	}																		\
	if ((Packet)->Private.Head == NULL)										\
	{																		\
		(Packet)->Private.Tail = TmpBuffer;									\
	}																		\
	TmpBuffer->Next = (Packet)->Private.Head;								\
	(Packet)->Private.Head = (Buffer);										\
	(Packet)->Private.ValidCounts = FALSE;									\
}

/*++

VOID
NdisChainBufferAtBack(
	IN OUT PNDIS_PACKET			Packet,
	IN OUT PNDIS_BUFFER			Buffer
	);

--*/

#define NdisChainBufferAtBack(Packet, Buffer)								\
{																			\
	PNDIS_BUFFER TmpBuffer = (Buffer);										\
																			\
	for (;;)																\
	{																		\
		if (TmpBuffer->Next == NULL)										\
			break;															\
		TmpBuffer = TmpBuffer->Next;										\
	}																		\
	if ((Packet)->Private.Head != NULL)										\
	{																		\
		(Packet)->Private.Tail->Next = (Buffer);							\
	}																		\
	else																	\
	{																		\
		(Packet)->Private.Head = (Buffer);									\
	}																		\
	(Packet)->Private.Tail = TmpBuffer;										\
	TmpBuffer->Next = NULL;													\
	(Packet)->Private.ValidCounts = FALSE;									\
}

EXPORT
VOID
NdisUnchainBufferAtFront(
	IN OUT PNDIS_PACKET			Packet,
	OUT PNDIS_BUFFER *			Buffer
	);

EXPORT
VOID
NdisUnchainBufferAtBack(
	IN OUT PNDIS_PACKET			Packet,
	OUT PNDIS_BUFFER *			Buffer
	);


/*++

VOID
NdisQueryPacket(
	IN	PNDIS_PACKET			_Packet,
	OUT PUINT					_PhysicalBufferCount OPTIONAL,
	OUT PUINT					_BufferCount OPTIONAL,
	OUT PNDIS_BUFFER *			_FirstBuffer OPTIONAL,
	OUT PUINT					_TotalPacketLength OPTIONAL
	);

--*/

#define NdisQueryPacket(_Packet,											\
						_PhysicalBufferCount,								\
						_BufferCount,										\
						_FirstBuffer,										\
						_TotalPacketLength)									\
{																			\
	if ((_FirstBuffer) != NULL)												\
	{																		\
		PNDIS_BUFFER * __FirstBuffer = (_FirstBuffer);						\
		*(__FirstBuffer) = (_Packet)->Private.Head;							\
	}																		\
	if ((_TotalPacketLength) || (_BufferCount) || (_PhysicalBufferCount))	\
	{																		\
		if (!(_Packet)->Private.ValidCounts)								\
		{																	\
			PNDIS_BUFFER TmpBuffer = (_Packet)->Private.Head;				\
			UINT PTotalLength = 0, PPhysicalCount = 0, PAddedCount = 0;		\
			UINT PacketLength, Offset;										\
																			\
			while (TmpBuffer != (PNDIS_BUFFER)NULL)							\
			{																\
				NdisQueryBufferOffset(TmpBuffer, &Offset, &PacketLength);	\
				PTotalLength += PacketLength;								\
				PPhysicalCount += (UINT)NDIS_BUFFER_TO_SPAN_PAGES(TmpBuffer);\
				++PAddedCount;												\
				TmpBuffer = TmpBuffer->Next;								\
			}																\
			(_Packet)->Private.Count = PAddedCount;							\
			(_Packet)->Private.TotalLength = PTotalLength;					\
			(_Packet)->Private.PhysicalCount = PPhysicalCount;				\
			(_Packet)->Private.ValidCounts = TRUE;							\
		}																	\
																			\
		if (_PhysicalBufferCount)											\
		{																	\
			PUINT __PhysicalBufferCount = (_PhysicalBufferCount);			\
			*(__PhysicalBufferCount) = (_Packet)->Private.PhysicalCount;	\
		}																	\
		if (_BufferCount)													\
		{																	\
			PUINT __BufferCount = (_BufferCount);							\
			*(__BufferCount) = (_Packet)->Private.Count;					\
		}																	\
		if (_TotalPacketLength)												\
		{																	\
			PUINT __TotalPacketLength = (_TotalPacketLength);				\
			*(__TotalPacketLength) = (_Packet)->Private.TotalLength;		\
		}																	\
	}																		\
}


/*++

VOID
NdisGetNextBuffer(
	IN	PNDIS_BUFFER			CurrentBuffer,
	OUT PNDIS_BUFFER *			NextBuffer
	);

--*/

#define NdisGetNextBuffer(CurrentBuffer, NextBuffer)						\
{																			\
	*(NextBuffer) = (CurrentBuffer)->Next;									\
}

#if BINARY_COMPATIBLE

VOID
NdisAdjustBufferLength(
	IN	PNDIS_BUFFER			Buffer,
	IN	UINT					Length
	);

#else // BINARY_COMPATIBLE

#if NDIS_NT
#define NdisAdjustBufferLength(Buffer, Length)	(((Buffer)->ByteCount) = (Length))
#else
#define NdisAdjustBufferLength(Buffer, Length)	(((Buffer)->Length) = (Length))
#endif

#endif // BINARY_COMPATIBLE

EXPORT
VOID
NdisCopyFromPacketToPacket(
	IN	PNDIS_PACKET			Destination,
	IN	UINT					DestinationOffset,
	IN	UINT					BytesToCopy,
	IN	PNDIS_PACKET			Source,
	IN	UINT					SourceOffset,
	OUT PUINT					BytesCopied
	);


EXPORT
NDIS_STATUS
NdisAllocateMemory(
	OUT PVOID *					VirtualAddress,
	IN	UINT					Length,
	IN	UINT					MemoryFlags,
	IN	NDIS_PHYSICAL_ADDRESS	HighestAcceptableAddress
	);

EXPORT
NDIS_STATUS
NdisAllocateMemoryWithTag(
	OUT PVOID *					VirtualAddress,
	IN	UINT					Length,
	IN	ULONG					Tag
	);

EXPORT
VOID
NdisFreeMemory(
	IN	PVOID					VirtualAddress,
	IN	UINT					Length,
	IN	UINT					MemoryFlags
	);


/*++
VOID
NdisStallExecution(
	IN	UINT					MicrosecondsToStall
	)
--*/

#define NdisStallExecution(MicroSecondsToStall)		KeStallExecutionProcessor(MicroSecondsToStall)


EXPORT
VOID
NdisInitializeEvent(
	IN	PNDIS_EVENT				Event
);

EXPORT
VOID
NdisSetEvent(
	IN	PNDIS_EVENT				Event
);

EXPORT
VOID
NdisResetEvent(
	IN	PNDIS_EVENT				Event
);

EXPORT
BOOLEAN
NdisWaitEvent(
	IN	PNDIS_EVENT				Event,
	IN	UINT					msToWait
);

/*++
VOID
NdisInitializeWorkItem(
	IN	PNDIS_WORK_ITEM			WorkItem,
	IN	NDIS_PROC				Routine,
	IN	PVOID					Context
	);
--*/

#define NdisInitializeWorkItem(_WI_, _R_, _C_)	\
	{											\
		(_WI_)->Context = _C_;					\
		(_WI_)->Routine = _R_;					\
	}

EXPORT
NDIS_STATUS
NdisScheduleWorkItem(
	IN	PNDIS_WORK_ITEM			WorkItem
	);

EXPORT
NDIS_STATUS
NdisQueryMapRegisterCount(
	IN	NDIS_INTERFACE_TYPE		BusType,
	OUT	PUINT					MapRegisterCount
);

//
// Simple I/O support
//

EXPORT
VOID
NdisOpenFile(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			FileHandle,
	OUT PUINT					FileLength,
	IN	PNDIS_STRING			FileName,
	IN	NDIS_PHYSICAL_ADDRESS	HighestAcceptableAddress
	);

EXPORT
VOID
NdisCloseFile(
	IN	NDIS_HANDLE				FileHandle
	);

EXPORT
VOID
NdisMapFile(
	OUT PNDIS_STATUS			Status,
	OUT PVOID *					MappedBuffer,
	IN	NDIS_HANDLE				FileHandle
	);

EXPORT
VOID
NdisUnmapFile(
	IN	NDIS_HANDLE				FileHandle
	);


//
// Portability extensions
//

/*++
VOID
NdisFlushBuffer(
	IN	PNDIS_BUFFER			Buffer,
	IN	BOOLEAN					WriteToDevice
	)
--*/

#define NdisFlushBuffer(Buffer,WriteToDevice)								\
		KeFlushIoBuffers((Buffer),!(WriteToDevice), TRUE)

/*++
ULONG
NdisGetCacheFillSize(
	)
--*/
#define NdisGetCacheFillSize()	HalGetDmaAlignmentRequirement()

//
// This macro is used to convert a port number as the caller
// thinks of it, to a port number as it should be passed to
// READ/WRITE_PORT.
//

#define NDIS_PORT_TO_PORT(Handle,Port)	(((PNDIS_ADAPTER_BLOCK)(Handle))->PortOffset + (Port))


//
// Write Port
//

/*++
VOID
NdisWritePortUchar(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	UCHAR					Data
	)
--*/
#define NdisWritePortUchar(Handle,Port,Data)								\
		WRITE_PORT_UCHAR((PUCHAR)(NDIS_PORT_TO_PORT(Handle,Port)),(UCHAR)(Data))

/*++
VOID
NdisWritePortUshort(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	USHORT					Data
	)
--*/
#define NdisWritePortUshort(Handle,Port,Data)								\
		WRITE_PORT_USHORT((PUSHORT)(NDIS_PORT_TO_PORT(Handle,Port)),(USHORT)(Data))


/*++
VOID
NdisWritePortUlong(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	ULONG					Data
	)
--*/
#define NdisWritePortUlong(Handle,Port,Data)								\
		WRITE_PORT_ULONG((PULONG)(NDIS_PORT_TO_PORT(Handle,Port)),(ULONG)(Data))


//
// Write Port Buffers
//

/*++
VOID
NdisWritePortBufferUchar(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisWritePortBufferUchar(Handle,Port,Buffer,Length)					\
		NdisRawWritePortBufferUchar(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))

/*++
VOID
NdisWritePortBufferUshort(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	PUSHORT					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisWritePortBufferUshort(Handle,Port,Buffer,Length)				\
		NdisRawWritePortBufferUshort(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))


/*++
VOID
NdisWritePortBufferUlong(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	IN	PULONG					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisWritePortBufferUlong(Handle,Port,Buffer,Length)					\
		NdisRawWritePortBufferUlong(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))


//
// Read Ports
//

/*++
VOID
NdisReadPortUchar(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PUCHAR					Data
	)
--*/
#define NdisReadPortUchar(Handle,Port, Data)								\
		NdisRawReadPortUchar(NDIS_PORT_TO_PORT((Handle),(Port)),(Data))

/*++
VOID
NdisReadPortUshort(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PUSHORT					Data
	)
--*/
#define NdisReadPortUshort(Handle,Port,Data)								\
		NdisRawReadPortUshort(NDIS_PORT_TO_PORT((Handle),(Port)),(Data))


/*++
VOID
NdisReadPortUlong(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PULONG					Data
	)
--*/
#define NdisReadPortUlong(Handle,Port,Data)									\
		NdisRawReadPortUlong(NDIS_PORT_TO_PORT((Handle),(Port)),(Data))

//
// Read Buffer Ports
//

/*++
VOID
NdisReadPortBufferUchar(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PUCHAR					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisReadPortBufferUchar(Handle,Port,Buffer,Length)					\
		NdisRawReadPortBufferUchar(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))

/*++
VOID
NdisReadPortBufferUshort(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PUSHORT					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisReadPortBufferUshort(Handle,Port,Buffer,Length) 				\
		NdisRawReadPortBufferUshort(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))

/*++
VOID
NdisReadPortBufferUlong(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	ULONG					Port,
	OUT PULONG					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisReadPortBufferUlong(Handle,Port,Buffer) 						\
		NdisRawReadPortBufferUlong(NDIS_PORT_TO_PORT((Handle),(Port)),(Buffer),(Length))

//
// Raw Routines
//

//
// Write Port Raw
//

/*++
VOID
NdisRawWritePortUchar(
	IN	ULONG_PTR				Port,
	IN	UCHAR					Data
	)
--*/
#define NdisRawWritePortUchar(Port,Data) 									\
		WRITE_PORT_UCHAR((PUCHAR)(Port),(UCHAR)(Data))

/*++
VOID
NdisRawWritePortUshort(
	IN	ULONG_PTR				Port,
	IN	USHORT					Data
	)
--*/
#define NdisRawWritePortUshort(Port,Data)									\
		WRITE_PORT_USHORT((PUSHORT)(Port),(USHORT)(Data))

/*++
VOID
NdisRawWritePortUlong(
	IN	ULONG_PTR				Port,
	IN	ULONG					Data
	)
--*/
#define NdisRawWritePortUlong(Port,Data) 									\
		WRITE_PORT_ULONG((PULONG)(Port),(ULONG)(Data))


//
// Raw Write Port Buffers
//

/*++
VOID
NdisRawWritePortBufferUchar(
	IN	ULONG_PTR				Port,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisRawWritePortBufferUchar(Port,Buffer,Length) \
		WRITE_PORT_BUFFER_UCHAR((PUCHAR)(Port),(PUCHAR)(Buffer),(Length))

/*++
VOID
NdisRawWritePortBufferUshort(
	IN	ULONG_PTR				Port,
	IN	PUSHORT					Buffer,
	IN	ULONG					Length
	)
--*/
#if defined(_M_IX86)
#define NdisRawWritePortBufferUshort(Port,Buffer,Length)					\
		WRITE_PORT_BUFFER_USHORT((PUSHORT)(Port),(PUSHORT)(Buffer),(Length))
#else
#define NdisRawWritePortBufferUshort(Port,Buffer,Length)					\
{																			\
		ULONG_PTR _Port = (ULONG_PTR)(Port);								\
		PUSHORT _Current = (Buffer);										\
		PUSHORT _End = _Current + (Length);									\
		for ( ; _Current < _End; ++_Current)								\
		{																	\
			WRITE_PORT_USHORT((PUSHORT)_Port,*(UNALIGNED USHORT *)_Current);\
		}																	\
}
#endif


/*++
VOID
NdisRawWritePortBufferUlong(
	IN	ULONG_PTR				Port,
	IN	PULONG					Buffer,
	IN	ULONG					Length
	)
--*/
#if defined(_M_IX86)
#define NdisRawWritePortBufferUlong(Port,Buffer,Length) 					\
		WRITE_PORT_BUFFER_ULONG((PULONG)(Port),(PULONG)(Buffer),(Length))
#else
#define NdisRawWritePortBufferUlong(Port,Buffer,Length)						\
{																			\
		ULONG_PTR _Port = (ULONG_PTR)(Port);								\
		PULONG _Current = (Buffer);											\
		PULONG _End = _Current + (Length);									\
		for ( ; _Current < _End; ++_Current)								\
		{																	\
			WRITE_PORT_ULONG((PULONG)_Port,*(UNALIGNED ULONG *)_Current);	\
		}																	\
}
#endif


//
// Raw Read Ports
//

/*++
VOID
NdisRawReadPortUchar(
	IN	ULONG_PTR				Port,
	OUT PUCHAR					Data
	)
--*/
#define NdisRawReadPortUchar(Port, Data) \
		*(Data) = READ_PORT_UCHAR((PUCHAR)(Port))

/*++
VOID
NdisRawReadPortUshort(
	IN	ULONG_PTR				Port,
	OUT PUSHORT					Data
	)
--*/
#define NdisRawReadPortUshort(Port,Data) \
		*(Data) = READ_PORT_USHORT((PUSHORT)(Port))

/*++
VOID
NdisRawReadPortUlong(
	IN	ULONG_PTR				Port,
	OUT PULONG					Data
	)
--*/
#define NdisRawReadPortUlong(Port,Data) \
		*(Data) = READ_PORT_ULONG((PULONG)(Port))


//
// Raw Read Buffer Ports
//

/*++
VOID
NdisRawReadPortBufferUchar(
	IN	ULONG_PTR				Port,
	OUT PUCHAR					Buffer,
	IN	ULONG					Length
	)
--*/
#define NdisRawReadPortBufferUchar(Port,Buffer,Length)						\
		READ_PORT_BUFFER_UCHAR((PUCHAR)(Port),(PUCHAR)(Buffer),(Length))


/*++
VOID
NdisRawReadPortBufferUshort(
	IN	ULONG_PTR				Port,
	OUT PUSHORT					Buffer,
	IN	ULONG					Length
	)
--*/
#if defined(_M_IX86)
#define NdisRawReadPortBufferUshort(Port,Buffer,Length) 					\
		READ_PORT_BUFFER_USHORT((PUSHORT)(Port),(PUSHORT)(Buffer),(Length))
#else
#define NdisRawReadPortBufferUshort(Port,Buffer,Length)						\
{																			\
		ULONG_PTR _Port = (ULONG_PTR)(Port);								\
		PUSHORT _Current = (Buffer);										\
		PUSHORT _End = _Current + (Length);									\
		for ( ; _Current < _End; ++_Current)								\
		{ 																	\
			*(UNALIGNED USHORT *)_Current = READ_PORT_USHORT((PUSHORT)_Port); \
		}																	\
}
#endif


/*++
VOID
NdisRawReadPortBufferUlong(
	IN	ULONG_PTR				Port,
	OUT PULONG					Buffer,
	IN	ULONG					Length
	)
--*/
#if defined(_M_IX86)
#define NdisRawReadPortBufferUlong(Port,Buffer,Length)						\
		READ_PORT_BUFFER_ULONG((PULONG)(Port),(PULONG)(Buffer),(Length))
#else
#define NdisRawReadPortBufferUlong(Port,Buffer,Length)						\
{																			\
		ULONG_PTR _Port = (ULONG_PTR)(Port);								\
		PULONG _Current = (Buffer);											\
		PULONG _End = _Current + (Length);									\
		for ( ; _Current < _End; ++_Current)								\
		{																	\
			*(UNALIGNED ULONG *)_Current = READ_PORT_ULONG((PULONG)_Port);	\
		}																	\
}
#endif


//
// Write Registers
//

/*++
VOID
NdisWriteRegisterUchar(
	IN	PUCHAR					Register,
	IN	UCHAR					Data
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUchar(Register,Data)								\
		WRITE_REGISTER_UCHAR((Register),(Data))
#else
#define NdisWriteRegisterUchar(Register,Data)								\
	{																		\
		WRITE_REGISTER_UCHAR((Register),(Data));							\
		READ_REGISTER_UCHAR(Register);										\
	}
#endif

/*++
VOID
NdisWriteRegisterUshort(
	IN	PUCHAR					Register,
	IN	USHORT					Data
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUshort(Register,Data)								\
		WRITE_REGISTER_USHORT((Register),(Data))
#else
#define NdisWriteRegisterUshort(Register,Data)								\
	{																		\
		WRITE_REGISTER_USHORT((Register),(Data));							\
		READ_REGISTER_USHORT(Register);										\
	}
#endif

/*++
VOID
NdisWriteRegisterUlong(
	IN	PUCHAR					Register,
	IN	ULONG					Data
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUlong(Register,Data)	WRITE_REGISTER_ULONG((Register),(Data))
#else
#define NdisWriteRegisterUlong(Register,Data)								\
	{																		\
		WRITE_REGISTER_ULONG((Register),(Data));							\
		READ_REGISTER_ULONG(Register);										\
	}
#endif

/*++
VOID
NdisWriteRegisterUcharWithStall(
	IN	PUCHAR					Register,
	IN	UCHAR					Data,
	IN	UINT					StallTimeInMicroSeconds
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUcharWithStall(Register, Data, StallTime)			\
		WRITE_REGISTER_UCHAR((Register),(Data))
#else
#define NdisWriteRegisterUcharWithStall(Register, Data, StallTime)					\
	{																		\
		WRITE_REGISTER_UCHAR((Register),(Data));							\
		NdisStallExecution(StallTime);										\
		READ_REGISTER_UCHAR(Register);										\
	}
#endif

/*++
VOID
NdisWriteRegisterUshortWithStall(
	IN	PUCHAR					Register,
	IN	USHORT					Data,
	IN	UINT					StallTimeInMicroSeconds
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUshortWithStall(Register, Data, StallTime)			\
		WRITE_REGISTER_USHORT((Register),(Data))
#else
#define NdisWriteRegisterUshortWithStall(Register, Data, StallTime)			\
	{																		\
		WRITE_REGISTER_USHORT((Register),(Data));							\
		NdisStallExecution(StallTime);										\
		READ_REGISTER_USHORT(Register);										\
	}
#endif

/*++
VOID
NdisWriteRegisterUlongWithStall(
	IN	PUCHAR					Register,
	IN	ULONG					Data,
	IN	UINT					StallTimeInMicroSeconds
	)
--*/

#if defined(_M_IX86)
#define NdisWriteRegisterUlongWithStall(Register,Data, StallTime)			\
		WRITE_REGISTER_ULONG((Register),(Data))
#else
#define NdisWriteRegisterUlongWithStall(Register,Data, StallTime)			\
	{																		\
		WRITE_REGISTER_ULONG((Register),(Data));							\
		NdisStallExecution(StallTime);										\
		READ_REGISTER_ULONG(Register);										\
	}
#endif

/*++
VOID
NdisReadRegisterUchar(
	IN	PUCHAR					Register,
	OUT PUCHAR					Data
	)
--*/
#if defined(_M_IX86)
#define NdisReadRegisterUchar(Register,Data)	*((PUCHAR)(Data)) = *(Register)
#else
#define NdisReadRegisterUchar(Register,Data)	*(Data) = READ_REGISTER_UCHAR((PUCHAR)(Register))
#endif

/*++
VOID
NdisReadRegisterUshort(
	IN	PUSHORT					Register,
	OUT PUSHORT					Data
	)
--*/
#if defined(_M_IX86)
#define NdisReadRegisterUshort(Register,Data)	*((PUSHORT)(Data)) = *(Register)
#else
#define NdisReadRegisterUshort(Register,Data)	*(Data) = READ_REGISTER_USHORT((PUSHORT)(Register))
#endif

/*++
VOID
NdisReadRegisterUlong(
	IN	PULONG					Register,
	OUT PULONG					Data
	)
--*/
#if defined(_M_IX86)
#define NdisReadRegisterUlong(Register,Data)	*((PULONG)(Data)) = *(Register)
#else
#define NdisReadRegisterUlong(Register,Data)	*(Data) = READ_REGISTER_ULONG((PULONG)(Register))
#endif

#define NdisEqualAnsiString(_String1,_String2, _CaseInsensitive)			\
			RtlEqualAnsiString(_String1, _String2, _CaseInsensitive)

#define NdisEqualString(_String1, _String2, _CaseInsensitive)				\
			RtlEqualUnicodeString(_String1, _String2, _CaseInsensitive)

#define NdisEqualUnicodeString(_String1, _String2, _CaseInsensitive)		\
			RtlEqualUnicodeString(_String1, _String2, _CaseInsensitive)

EXPORT
VOID
NdisWriteErrorLogEntry(
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	NDIS_ERROR_CODE			ErrorCode,
	IN	ULONG					NumberOfErrorValues,
	...
	);

EXPORT
VOID
NdisInitializeString(
	OUT	PNDIS_STRING	Destination,
	IN	PUCHAR			Source
	);

#define NdisFreeString(String) NdisFreeMemory((String).Buffer, (String).MaximumLength, 0)

#define NdisPrintString(String) DbgPrint("%ls",(String).Buffer)


#if !defined(_ALPHA_)
/*++

VOID
NdisCreateLookaheadBufferFromSharedMemory(
	IN	PVOID					pSharedMemory,
	IN	UINT					LookaheadLength,
	OUT PVOID *					pLookaheadBuffer
	);

--*/

#define NdisCreateLookaheadBufferFromSharedMemory(_S, _L, _B)	((*(_B)) = (_S))

/*++

VOID
NdisDestroyLookaheadBufferFromSharedMemory(
	IN	PVOID					pLookaheadBuffer
	);

--*/

#define NdisDestroyLookaheadBufferFromSharedMemory(_B)

#else // Alpha

EXPORT
VOID
NdisCreateLookaheadBufferFromSharedMemory(
	IN	PVOID					pSharedMemory,
	IN	UINT					LookaheadLength,
	OUT PVOID *					pLookaheadBuffer
	);

EXPORT
VOID
NdisDestroyLookaheadBufferFromSharedMemory(
	IN	PVOID 					pLookaheadBuffer
	);

#endif


//
// The following declarations are shared between ndismac.h and ndismini.h. They
// are meant to be for internal use only. They should not be used directly by
// miniport drivers.
//

//
// declare these first since they point to each other
//

typedef struct _NDIS_WRAPPER_HANDLE	NDIS_WRAPPER_HANDLE, *PNDIS_WRAPPER_HANDLE;
typedef struct _NDIS_MAC_BLOCK		NDIS_MAC_BLOCK, *PNDIS_MAC_BLOCK;
typedef struct _NDIS_ADAPTER_BLOCK	NDIS_ADAPTER_BLOCK, *PNDIS_ADAPTER_BLOCK;
typedef struct _NDIS_PROTOCOL_BLOCK NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;
typedef struct _NDIS_OPEN_BLOCK		NDIS_OPEN_BLOCK, *PNDIS_OPEN_BLOCK;

//
// Timers.
//

typedef
VOID
(*PNDIS_TIMER_FUNCTION) (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);

typedef struct _NDIS_TIMER
{
	KTIMER		Timer;
	KDPC		Dpc;
} NDIS_TIMER, *PNDIS_TIMER;

EXPORT
VOID
NdisSetTimer(
	IN	PNDIS_TIMER				Timer,
	IN	UINT					MillisecondsToDelay
	);

//
// DMA operations.
//

EXPORT
VOID
NdisAllocateDmaChannel(
	OUT PNDIS_STATUS			Status,
	OUT PNDIS_HANDLE			NdisDmaHandle,
	IN	NDIS_HANDLE				NdisAdapterHandle,
	IN	PNDIS_DMA_DESCRIPTION	DmaDescription,
	IN	ULONG					MaximumLength
	);

EXPORT
VOID
NdisFreeDmaChannel(
	IN	NDIS_HANDLE				NdisDmaHandle
	);

EXPORT
VOID
NdisSetupDmaTransfer(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisDmaHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					Offset,
	IN	ULONG					Length,
	IN	BOOLEAN					WriteToDevice
	);

EXPORT
VOID
NdisCompleteDmaTransfer(
	OUT PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisDmaHandle,
	IN	PNDIS_BUFFER			Buffer,
	IN	ULONG					Offset,
	IN	ULONG					Length,
	IN	BOOLEAN					WriteToDevice
	);

/*++
ULONG
NdisReadDmaCounter(
	IN	NDIS_HANDLE				NdisDmaHandle
	)
--*/

#define NdisReadDmaCounter(_NdisDmaHandle) \
	HalReadDmaCounter(((PNDIS_DMA_BLOCK)(_NdisDmaHandle))->SystemAdapterObject)

//
// Wrapper initialization and termination.
//

EXPORT
VOID
NdisInitializeWrapper(
	OUT PNDIS_HANDLE			NdisWrapperHandle,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);

EXPORT
VOID
NdisTerminateWrapper(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PVOID					SystemSpecific
	);

//
// Shared memory
//

#define	NdisUpdateSharedMemory(_H, _L, _V, _P)

//
// System processor count
//

EXPORT
CCHAR
NdisSystemProcessorCount(
	VOID
	);

EXPORT
VOID
NdisImmediateReadPortUchar(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PUCHAR					Data
	);

EXPORT
VOID
NdisImmediateReadPortUshort(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PUSHORT Data
	);

EXPORT
VOID
NdisImmediateReadPortUlong(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	OUT PULONG Data
	);

EXPORT
VOID
NdisImmediateWritePortUchar(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	UCHAR					Data
	);

EXPORT
VOID
NdisImmediateWritePortUshort(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	USHORT					Data
	);

EXPORT
VOID
NdisImmediateWritePortUlong(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					Port,
	IN	ULONG					Data
	);

EXPORT
VOID
NdisImmediateReadSharedMemory(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SharedMemoryAddress,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	);

EXPORT
VOID
NdisImmediateWriteSharedMemory(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SharedMemoryAddress,
	IN	PUCHAR					Buffer,
	IN	ULONG					Length
	);

EXPORT
ULONG
NdisImmediateReadPciSlotInformation(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

EXPORT
ULONG
NdisImmediateWritePciSlotInformation(
	IN	NDIS_HANDLE				WrapperConfigurationContext,
	IN	ULONG					SlotNumber,
	IN	ULONG					Offset,
	IN	PVOID					Buffer,
	IN	ULONG					Length
	);

//
// Ansi/Unicode support routines
//

#if BINARY_COMPATIBLE

EXPORT
VOID
NdisInitAnsiString(
	IN OUT	PANSI_STRING		DestinationString,
	IN		PCSTR				SourceString
	);

EXPORT
VOID
NdisInitUnicodeString(
	IN OUT	PUNICODE_STRING		DestinationString,
	IN		PCWSTR				SourceString
	);

EXPORT
NDIS_STATUS
NdisAnsiStringToUnicodeString(
	IN OUT	PUNICODE_STRING		DestinationString,
	IN		PANSI_STRING		SourceString
	);

EXPORT
NDIS_STATUS
NdisUnicodeStringToAnsiString(
	IN OUT	PANSI_STRING		DestinationString,
	IN		PUNICODE_STRING		SourceString
	);

EXPORT
NDIS_STATUS
NdisUpcaseUnicodeString(
	OUT	PUNICODE_STRING			DestinationString,
	IN	PUNICODE_STRING			SourceString
	);

#else // BINARY_COMPATIBLE

#define	NdisInitAnsiString(_as, s)				RtlInitString(_as, s)
#define	NdisInitUnicodeString(_us, s)			RtlInitUnicodeString(_us, s)
#define	NdisAnsiStringToUnicodeString(_us, _as)	RtlAnsiStringToUnicodeString(_us, _as, FALSE)
#define	NdisUnicodeStringToAnsiString(_as, _us)	RtlUnicodeStringToAnsiString(_as, _us, FALSE)
#define	NdisUpcaseUnicodeString(_d, _s)			RtlUpcaseUnicodeString(_d, _s, FALSE)

#endif // BINARY_COMPATIBLE

//
// Non-paged lookaside list support routines
//

#define	NdisInitializeNPagedLookasideList(_L, _AR, _FR, _Fl, _S, _T, _D) \
				ExInitializeNPagedLookasideList(_L, _AR, _FR, _Fl, _S, _T, _D)

#define	NdisDeleteNPagedLookasideList(_L)			ExDeleteNPagedLookasideList(_L)
#define	NdisAllocateFromNPagedLookasideList(_L)		ExAllocateFromNPagedLookasideList(_L)
#define	NdisFreeToNPagedLookasideList(_L, _E)		ExFreeToNPagedLookasideList(_L, _E)


#if defined(NDIS_WRAPPER)
typedef struct _OID_LIST	OID_LIST, *POID_LIST;
#endif // NDIS_WRAPPER defined





