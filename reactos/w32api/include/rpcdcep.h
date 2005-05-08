#ifndef _RPCDCEP_H
#define _RPCDCEP_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define RPC_NCA_FLAGS_DEFAULT 0
#define RPC_NCA_FLAGS_IDEMPOTENT 1
#define RPC_NCA_FLAGS_BROADCAST 2
#define RPC_NCA_FLAGS_MAYBE 4
#define RPCFLG_ASYNCHRONOUS 0x40000000
#define RPCFLG_INPUT_SYNCHRONOUS 0x20000000
#define RPC_FLAGS_VALID_BIT 0x8000
#define TRANSPORT_TYPE_CN 1
#define TRANSPORT_TYPE_DG 2
#define TRANSPORT_TYPE_LPC 4
#define TRANSPORT_TYPE_WMSG 8

typedef struct _RPC_VERSION {
	unsigned short MajorVersion;
	unsigned short MinorVersion;
} RPC_VERSION;
typedef struct _RPC_SYNTAX_IDENTIFIER {
	GUID SyntaxGUID;
	RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, *PRPC_SYNTAX_IDENTIFIER;
typedef struct _RPC_MESSAGE {
	HANDLE Handle;
	unsigned long DataRepresentation;
	void *Buffer;
	unsigned int BufferLength;
	unsigned int ProcNum;
	PRPC_SYNTAX_IDENTIFIER TransferSyntax;
	void *RpcInterfaceInformation;
	void *ReservedForRuntime;
	void *ManagerEpv;
	void *ImportContext;
	unsigned long RpcFlags;
} RPC_MESSAGE,*PRPC_MESSAGE;
typedef long __stdcall RPC_FORWARD_FUNCTION(GUID*,RPC_VERSION*,GUID*,unsigned char*,void**);
typedef void(__stdcall *RPC_DISPATCH_FUNCTION) ( PRPC_MESSAGE Message);
typedef struct {
	unsigned int DispatchTableCount;
	RPC_DISPATCH_FUNCTION *DispatchTable;
	int Reserved;
} RPC_DISPATCH_TABLE,*PRPC_DISPATCH_TABLE;
typedef struct _RPC_PROTSEQ_ENDPOINT {
	unsigned char *RpcProtocolSequence;
	unsigned char *Endpoint;
} RPC_PROTSEQ_ENDPOINT,*PRPC_PROTSEQ_ENDPOINT;
typedef struct _RPC_SERVER_INTERFACE {
	unsigned int Length;
	RPC_SYNTAX_IDENTIFIER InterfaceId;
	RPC_SYNTAX_IDENTIFIER TransferSyntax;
	PRPC_DISPATCH_TABLE DispatchTable;
	unsigned int RpcProtseqEndpointCount;
	PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
	void *DefaultManagerEpv;
	void const *InterpreterInfo;
} RPC_SERVER_INTERFACE,*PRPC_SERVER_INTERFACE;
typedef struct _RPC_CLIENT_INTERFACE {
	unsigned int Length;
	RPC_SYNTAX_IDENTIFIER InterfaceId;
	RPC_SYNTAX_IDENTIFIER TransferSyntax;
	PRPC_DISPATCH_TABLE DispatchTable;
	unsigned int RpcProtseqEndpointCount;
	PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
	unsigned long Reserved;
	void const *InterpreterInfo;
} RPC_CLIENT_INTERFACE,*PRPC_CLIENT_INTERFACE;
typedef void *I_RPC_MUTEX;
typedef struct _RPC_TRANSFER_SYNTAX {
	GUID Uuid;
	unsigned short VersMajor;
	unsigned short VersMinor;
} RPC_TRANSFER_SYNTAX;
typedef long(__stdcall *RPC_BLOCKING_FUNCTION)(void*,void*);

long __stdcall I_RpcGetBuffer(RPC_MESSAGE*);
long __stdcall I_RpcSendReceive(RPC_MESSAGE*);
long __stdcall I_RpcFreeBuffer(RPC_MESSAGE*);
void __stdcall I_RpcRequestMutex(I_RPC_MUTEX*);
void __stdcall I_RpcClearMutex(I_RPC_MUTEX);
void __stdcall I_RpcDeleteMutex(I_RPC_MUTEX);
DECLARE_STDCALL_P(void *) I_RpcAllocate(unsigned int);
void __stdcall I_RpcFree(void*);
void __stdcall I_RpcPauseExecution(unsigned long);
typedef void(__stdcall *PRPC_RUNDOWN) (void*);
long __stdcall I_RpcMonitorAssociation(HANDLE,PRPC_RUNDOWN,void*);
long __stdcall I_RpcStopMonitorAssociation(HANDLE);
HANDLE __stdcall I_RpcGetCurrentCallHandle(void);
long __stdcall I_RpcGetAssociationContext(void**);
long __stdcall I_RpcSetAssociationContext(void*);
#ifdef __RPC_NT__
long __stdcall I_RpcNsBindingSetEntryName(HANDLE,unsigned long,unsigned short*);
long __stdcall I_RpcBindingInqDynamicEndpoint(HANDLE, unsigned short**);
#else
long __stdcall I_RpcNsBindingSetEntryName(HANDLE,unsigned long,unsigned char*);
long __stdcall I_RpcBindingInqDynamicEndpoint(HANDLE,unsigned char**);
#endif
long __stdcall I_RpcBindingInqTransportType(HANDLE,unsigned int*);
long __stdcall I_RpcIfInqTransferSyntaxes(HANDLE,RPC_TRANSFER_SYNTAX*,unsigned int,unsigned int*);
long __stdcall I_UuidCreate(GUID*);
long __stdcall I_RpcBindingCopy(HANDLE,HANDLE*);
long __stdcall I_RpcBindingIsClientLocal(HANDLE,unsigned int*);
void __stdcall I_RpcSsDontSerializeContext(void);
long __stdcall I_RpcServerRegisterForwardFunction(RPC_FORWARD_FUNCTION*);
long __stdcall I_RpcConnectionInqSockBuffSize(unsigned long*,unsigned long*);
long __stdcall I_RpcConnectionSetSockBuffSize(unsigned long,unsigned long);
long __stdcall I_RpcBindingSetAsync(HANDLE,RPC_BLOCKING_FUNCTION);
long __stdcall I_RpcAsyncSendReceive(RPC_MESSAGE*,void*);
long __stdcall I_RpcGetThreadWindowHandle(void**);
long __stdcall I_RpcServerThreadPauseListening(void);
long __stdcall I_RpcServerThreadContinueListening(void);
long __stdcall I_RpcServerUnregisterEndpointA(unsigned char*,unsigned char*);
long __stdcall I_RpcServerUnregisterEndpointW(unsigned short*,unsigned short*);
#ifdef UNICODE
#define I_RpcServerUnregisterEndpoint I_RpcServerUnregisterEndpointW
#else
#define I_RpcServerUnregisterEndpoint I_RpcServerUnregisterEndpointA
#endif
#ifdef __cplusplus
}
#endif
#endif
