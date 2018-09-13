
typedef struct _CO_CALL_PARAMETERS		CO_CALL_PARAMETERS, *PCO_CALL_PARAMETERS;
typedef struct _CO_MEDIA_PARAMETERS		CO_MEDIA_PARAMETERS, *PCO_MEDIA_PARAMETERS;

//
// CoNdis client only handler proto-types - used by clients of call managers
//
typedef
VOID
(*CL_OPEN_AF_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisAfHandle
	);

typedef
VOID
(*CL_CLOSE_AF_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext
	);

typedef
VOID
(*CL_REG_SAP_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	PCO_SAP					Sap,
	IN	NDIS_HANDLE				NdisSapHandle
	);

typedef
VOID
(*CL_DEREG_SAP_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext
	);

typedef
VOID
(*CL_MAKE_CALL_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CL_CLOSE_CALL_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				ProtocolPartyContext OPTIONAL
	);

typedef
VOID
(*CL_ADD_PARTY_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CL_DROP_PARTY_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext
	);

typedef
NDIS_STATUS
(*CL_INCOMING_CALL_HANDLER)(
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters
	);

typedef
VOID
(*CL_CALL_CONNECTED_HANDLER)(
	IN	NDIS_HANDLE				ProtocolVcContext
	);

typedef
VOID
(*CL_INCOMING_CLOSE_CALL_HANDLER)(
	IN	NDIS_STATUS				CloseStatus,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

typedef
VOID
(*CL_INCOMING_DROP_PARTY_HANDLER)(
	IN	NDIS_STATUS				DropStatus,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

typedef
VOID
(*CL_MODIFY_CALL_QOS_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CL_INCOMING_CALL_QOS_CHANGE_HANDLER)(
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef struct _NDIS_CLIENT_CHARACTERISTICS
{
	UCHAR							MajorVersion;
	UCHAR							MinorVersion;

	USHORT							Filler;
	UINT							Reserved;

	CO_CREATE_VC_HANDLER			ClCreateVcHandler;
	CO_DELETE_VC_HANDLER			ClDeleteVcHandler;
	CO_REQUEST_HANDLER				ClRequestHandler;
	CO_REQUEST_COMPLETE_HANDLER		ClRequestCompleteHandler;
	CL_OPEN_AF_COMPLETE_HANDLER		ClOpenAfCompleteHandler;
	CL_CLOSE_AF_COMPLETE_HANDLER	ClCloseAfCompleteHandler;
	CL_REG_SAP_COMPLETE_HANDLER		ClRegisterSapCompleteHandler;
	CL_DEREG_SAP_COMPLETE_HANDLER	ClDeregisterSapCompleteHandler;
	CL_MAKE_CALL_COMPLETE_HANDLER	ClMakeCallCompleteHandler;
	CL_MODIFY_CALL_QOS_COMPLETE_HANDLER	ClModifyCallQoSCompleteHandler;
	CL_CLOSE_CALL_COMPLETE_HANDLER	ClCloseCallCompleteHandler;
	CL_ADD_PARTY_COMPLETE_HANDLER	ClAddPartyCompleteHandler;
	CL_DROP_PARTY_COMPLETE_HANDLER	ClDropPartyCompleteHandler;
	CL_INCOMING_CALL_HANDLER		ClIncomingCallHandler;
	CL_INCOMING_CALL_QOS_CHANGE_HANDLER	ClIncomingCallQoSChangeHandler;
	CL_INCOMING_CLOSE_CALL_HANDLER	ClIncomingCloseCallHandler;
	CL_INCOMING_DROP_PARTY_HANDLER	ClIncomingDropPartyHandler;
	CL_CALL_CONNECTED_HANDLER		ClCallConnectedHandler;

} NDIS_CLIENT_CHARACTERISTICS, *PNDIS_CLIENT_CHARACTERISTICS;

//
// CoNdis call-manager only handler proto-types - used by call managers only
//
typedef
NDIS_STATUS
(*CM_OPEN_AF_HANDLER)(
	IN	NDIS_HANDLE				CallMgrBindingContext,
	IN	PCO_ADDRESS_FAMILY		AddressFamily,
	IN	NDIS_HANDLE				NdisAfHandle,
	OUT	PNDIS_HANDLE			CallMgrAfContext
	);

typedef
NDIS_STATUS
(*CM_CLOSE_AF_HANDLER)(
	IN	NDIS_HANDLE				CallMgrAfContext
	);

typedef
NDIS_STATUS
(*CM_REG_SAP_HANDLER)(
	IN	NDIS_HANDLE				CallMgrAfContext,
	IN	PCO_SAP					Sap,
	IN	NDIS_HANDLE				NdisSapHandle,
	OUT	PNDIS_HANDLE			CallMgrSapContext
	);

typedef
NDIS_STATUS
(*CM_DEREG_SAP_HANDLER)(
	IN	NDIS_HANDLE				CallMgrSapContext
	);

typedef
NDIS_STATUS
(*CM_MAKE_CALL_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	OUT	PNDIS_HANDLE			CallMgrPartyContext OPTIONAL
	);

typedef
NDIS_STATUS
(*CM_CLOSE_CALL_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
	IN	PVOID					CloseData			OPTIONAL,
	IN	UINT					Size				OPTIONAL
	);

typedef
NDIS_STATUS
(*CM_MODIFY_CALL_QOS_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CM_INCOMING_CALL_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CM_ACTIVATE_VC_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

typedef
VOID
(*CM_DEACTIVATE_VC_COMPLETE_HANDLER)(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				CallMgrVcContext
	);

typedef
NDIS_STATUS
(*CM_ADD_PARTY_HANDLER)(
	IN	NDIS_HANDLE				CallMgrVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	IN	NDIS_HANDLE				NdisPartyHandle,
	OUT	PNDIS_HANDLE			CallMgrPartyContext
	);

typedef
NDIS_STATUS
(*CM_DROP_PARTY_HANDLER)(
	IN	NDIS_HANDLE				CallMgrPartyContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

typedef struct _NDIS_CALL_MANAGER_CHARACTERISTICS
{
	UCHAR							MajorVersion;
	UCHAR							MinorVersion;
	USHORT							Filler;
	UINT							Reserved;

	CO_CREATE_VC_HANDLER			CmCreateVcHandler;
	CO_DELETE_VC_HANDLER			CmDeleteVcHandler;
	CM_OPEN_AF_HANDLER				CmOpenAfHandler;
	CM_CLOSE_AF_HANDLER				CmCloseAfHandler;
	CM_REG_SAP_HANDLER				CmRegisterSapHandler;
	CM_DEREG_SAP_HANDLER			CmDeregisterSapHandler;
	CM_MAKE_CALL_HANDLER			CmMakeCallHandler;
	CM_CLOSE_CALL_HANDLER			CmCloseCallHandler;
	CM_INCOMING_CALL_COMPLETE_HANDLER CmIncomingCallCompleteHandler;
	CM_ADD_PARTY_HANDLER			CmAddPartyHandler;
	CM_DROP_PARTY_HANDLER			CmDropPartyHandler;
	CM_ACTIVATE_VC_COMPLETE_HANDLER	CmActivateVcCompleteHandler;
	CM_DEACTIVATE_VC_COMPLETE_HANDLER CmDeactivateVcCompleteHandler;
	CM_MODIFY_CALL_QOS_HANDLER		CmModifyCallQoSHandler;
	CO_REQUEST_HANDLER				CmRequestHandler;
	CO_REQUEST_COMPLETE_HANDLER		CmRequestCompleteHandler;
	
} NDIS_CALL_MANAGER_CHARACTERISTICS, *PNDIS_CALL_MANAGER_CHARACTERISTICS;

//
// this send flag is used on ATM net cards to set ( turn on ) the CLP bit
// (Cell Loss Priority) bit
//
#define CO_SEND_FLAG_SET_DISCARD_ELIBILITY	0x00000001

//
// the Address structure used on NDIS_CO_ADD_ADDRESS or NDIS_CO_DELETE_ADDRESS
//
typedef struct _CO_ADDRESS
{
	ULONG						AddressSize;
	UCHAR						Address[1];
} CO_ADDRESS, *PCO_ADDRESS;

//
// the list of addresses returned from the CallMgr on a NDIS_CO_GET_ADDRESSES
//
typedef struct _CO_ADDRESS_LIST
{
	ULONG						NumberOfAddressesAvailable;
	ULONG						NumberOfAddresses;
	CO_ADDRESS					AddressList;
} CO_ADDRESS_LIST, *PCO_ADDRESS_LIST;

#ifndef	FAR
#define	FAR
#endif
#include <qos.h>

typedef struct _CO_SPECIFIC_PARAMETERS
{
	ULONG						ParamType;
	ULONG						Length;
	UCHAR						Parameters[1];
} CO_SPECIFIC_PARAMETERS, *PCO_SPECIFIC_PARAMETERS;

typedef struct _CO_CALL_MANAGER_PARAMETERS
{
	FLOWSPEC					Transmit;
	FLOWSPEC					Receive;
	CO_SPECIFIC_PARAMETERS		CallMgrSpecific;
} CO_CALL_MANAGER_PARAMETERS, *PCO_CALL_MANAGER_PARAMETERS;


//
// this is the generic portion of the media parameters, including the media
// specific component too.
//
typedef struct _CO_MEDIA_PARAMETERS
{
	ULONG						Flags;
	ULONG						ReceivePriority;
	ULONG						ReceiveSizeHint;
	CO_SPECIFIC_PARAMETERS		MediaSpecific;
} CO_MEDIA_PARAMETERS, *PCO_MEDIA_PARAMETERS;

//
// definitions for the flags in CO_MEDIA_PARAMETERS
//
#define RECEIVE_TIME_INDICATION	0x00000001
#define USE_TIME_STAMPS			0x00000002
#define TRANSMIT_VC				0x00000004
#define RECEIVE_VC				0x00000008
#define INDICATE_ERRED_PACKETS	0x00000010
#define INDICATE_END_OF_TX		0x00000020
#define RESERVE_RESOURCES_VC	0x00000040
#define	ROUND_DOWN_FLOW			0x00000080
#define	ROUND_UP_FLOW			0x00000100
//
// define a flag to set in the flags of an Ndis packet when the miniport
// indicates a receive with an error in it
//
#define ERRED_PACKET_INDICATION	0x00000001

//
// this is the structure passed during call-setup
//
typedef struct _CO_CALL_PARAMETERS
{
	ULONG						Flags;
	PCO_CALL_MANAGER_PARAMETERS CallMgrParameters;
	PCO_MEDIA_PARAMETERS		MediaParameters;
} CO_CALL_PARAMETERS, *PCO_CALL_PARAMETERS;

//
// Definitions for the Flags in CO_CALL_PARAMETERS
//
#define PERMANENT_VC			0x00000001
#define CALL_PARAMETERS_CHANGED 0x00000002
#define QUERY_CALL_PARAMETERS	0x00000004
#define BROADCAST_VC			0x00000008
#define MULTIPOINT_VC			0x00000010

//
// The format of the Request for adding/deleting a PVC
//
typedef struct _CO_PVC
{
	NDIS_HANDLE					NdisAfHandle;
	CO_SPECIFIC_PARAMETERS		PvcParameters;
} CO_PVC,*PCO_PVC;


typedef struct _ATM_ADDRESS		ATM_ADDRESS, *PATM_ADDRESS;

EXPORT
VOID
NdisConvertStringToAtmAddress(
	OUT	PNDIS_STATUS			Status,
	IN	PNDIS_STRING			String,
	OUT	PATM_ADDRESS			AtmAddress
	);

//
// NDIS 5.0 Extensions for protocols
//

EXPORT
NDIS_STATUS
NdisCoAssignInstanceName(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PNDIS_STRING			BaseInstanceName,
	OUT	PNDIS_STRING			VcInstanceName
	);

EXPORT
VOID
NdisCoSendPackets(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);


EXPORT
NDIS_STATUS
NdisCoCreateVc(
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	NDIS_HANDLE				NdisAfHandle		OPTIONAL,	// For CM signalling VCs
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN OUT PNDIS_HANDLE			NdisVcHandle
	);


EXPORT
NDIS_STATUS
NdisCoDeleteVc(
	IN	NDIS_HANDLE				NdisVcHandle
	);


EXPORT
NDIS_STATUS
NdisCoRequest(
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	NDIS_HANDLE				NdisAfHandle	OPTIONAL,
	IN	NDIS_HANDLE				NdisVcHandle	OPTIONAL,
	IN	NDIS_HANDLE				NdisPartyHandle OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	);


EXPORT
VOID
NdisCoRequestComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisAfHandle,
	IN	NDIS_HANDLE				NdisVcHandle	OPTIONAL,
	IN	NDIS_HANDLE				NdisPartyHandle	OPTIONAL,
	IN	PNDIS_REQUEST			NdisRequest
	);

#ifndef __NDISTAPI_VAR_STRING_DECLARED
#define __NDISTAPI_VAR_STRING_DECLARED

typedef struct _VAR_STRING
{
    ULONG   ulTotalSize;
    ULONG   ulNeededSize;
    ULONG   ulUsedSize;

    ULONG   ulStringFormat;
    ULONG   ulStringSize;
    ULONG   ulStringOffset;

} VAR_STRING, *PVAR_STRING;

#endif // __NDISTAPI_VAR_STRING_DECLARED


#ifndef __NDISTAPI_STRINGFORMATS_DEFINED
#define __NDISTAPI_STRINGFORMATS_DEFINED

#define STRINGFORMAT_ASCII                          0x00000001
#define STRINGFORMAT_DBCS                           0x00000002
#define STRINGFORMAT_UNICODE                        0x00000003
#define STRINGFORMAT_BINARY                         0x00000004

#endif // __NDISTAPI_STRINGFORMATS_DEFINED

EXPORT
NDIS_STATUS
NdisCoGetTapiCallId(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	OUT	PVAR_STRING			TapiCallId
	);

//
// Client Apis
//
EXPORT
NDIS_STATUS
NdisClOpenAddressFamily(
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	PCO_ADDRESS_FAMILY		AddressFamily,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	PNDIS_CLIENT_CHARACTERISTICS ClCharacteristics,
	IN	UINT					SizeOfClCharacteristics,
	OUT	PNDIS_HANDLE			NdisAfHandle
	);


EXPORT
NDIS_STATUS
NdisClCloseAddressFamily(
	IN	NDIS_HANDLE				NdisAfHandle
	);


EXPORT
NDIS_STATUS
NdisClRegisterSap(
	IN	NDIS_HANDLE				NdisAfHandle,
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	PCO_SAP					Sap,
	OUT	PNDIS_HANDLE			NdisSapHandle
	);


EXPORT
NDIS_STATUS
NdisClDeregisterSap(
	IN	NDIS_HANDLE				NdisSapHandle
	);


EXPORT
NDIS_STATUS
NdisClMakeCall(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	OUT	PNDIS_HANDLE			NdisPartyHandle			OPTIONAL
	);


EXPORT
NDIS_STATUS
NdisClCloseCall(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	NDIS_HANDLE				NdisPartyHandle			OPTIONAL,
	IN	PVOID					Buffer					OPTIONAL,
	IN	UINT					Size					OPTIONAL
	);


EXPORT
NDIS_STATUS
NdisClModifyCallQoS(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisClIncomingCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
NDIS_STATUS
NdisClAddParty(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters,
	OUT	PNDIS_HANDLE			NdisPartyHandle
	);


EXPORT
NDIS_STATUS
NdisClDropParty(
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	PVOID					Buffer		OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);


EXPORT
NDIS_STATUS
NdisClGetProtocolVcContextFromTapiCallId(
	IN	UNICODE_STRING			TapiCallId,
	OUT PNDIS_HANDLE			ProtocolVcContext
	);

//
// Call Manager Apis
//
EXPORT
NDIS_STATUS
NdisCmRegisterAddressFamily(
	IN	NDIS_HANDLE				NdisBindingHandle,
	IN	PCO_ADDRESS_FAMILY		AddressFamily,
	IN	PNDIS_CALL_MANAGER_CHARACTERISTICS CmCharacteristics,
	IN	UINT					SizeOfCmCharacteristics
	);


EXPORT
VOID
NdisCmOpenAddressFamilyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisAfHandle,
	IN	NDIS_HANDLE				CallMgrAfContext
	);


EXPORT
VOID
NdisCmCloseAddressFamilyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisAfHandle
	);


EXPORT
VOID
NdisCmRegisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisSapHandle,
	IN	NDIS_HANDLE				CallMgrSapContext
	);


EXPORT
VOID
NdisCmDeregisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisSapHandle
	);


EXPORT
NDIS_STATUS
NdisCmActivateVc(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN OUT PCO_CALL_PARAMETERS	CallParameters
	);


EXPORT
NDIS_STATUS
NdisCmDeactivateVc(
	IN	NDIS_HANDLE				NdisVcHandle
	);


EXPORT
VOID
NdisCmMakeCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisCmCloseCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	NDIS_HANDLE				NdisPartyHandle	OPTIONAL
	);


EXPORT
VOID
NdisCmAddPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	NDIS_HANDLE				CallMgrPartyContext	OPTIONAL,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisCmDropPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisPartyHandle
	);


EXPORT
NDIS_STATUS
NdisCmDispatchIncomingCall(
	IN	NDIS_HANDLE				NdisSapHandle,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisCmDispatchCallConnected(
	IN	NDIS_HANDLE				NdisVcHandle
	);


EXPORT
VOID
NdisCmModifyCallQoSComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisCmDispatchIncomingCallQoSChange(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


EXPORT
VOID
NdisCmDispatchIncomingCloseCall(
	IN	NDIS_STATUS				CloseStatus,
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PVOID					Buffer		OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);


EXPORT
VOID
NdisCmDispatchIncomingDropParty(
	IN	NDIS_STATUS				DropStatus,
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	PVOID					Buffer		OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

