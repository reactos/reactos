#ifndef _RPCNSIP_H
#define _RPCNSIP_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
	RPC_NS_HANDLE LookupContext;
	RPC_BINDING_HANDLE ProposedHandle;
	RPC_BINDING_VECTOR *Bindings;
} RPC_IMPORT_CONTEXT_P,*PRPC_IMPORT_CONTEXT_P;
RPC_STATUS RPC_ENTRY I_RpcNsGetBuffer(IN PRPC_MESSAGE);
RPC_STATUS RPC_ENTRY I_RpcNsSendReceive(IN PRPC_MESSAGE,OUT RPC_BINDING_HANDLE*);
void RPC_ENTRY I_RpcNsRaiseException(IN PRPC_MESSAGE,IN RPC_STATUS);
RPC_STATUS RPC_ENTRY I_RpcReBindBuffer(IN PRPC_MESSAGE);
RPC_STATUS RPC_ENTRY I_NsServerBindSearch(void);
RPC_STATUS RPC_ENTRY I_NsClientBindSearch(void);
void RPC_ENTRY I_NsClientBindDone(void);
#ifdef __cplusplus
}
#endif
#endif
