#ifndef __INCLUDE_RPCRT4_RPC_H
#define __INCLUDE_RPCRT4_RPC_H

typedef void* RPC_BINDING_HANDLE;
typedef long RPC_STATUS;

typedef ULONG RPC_PROTOCOL_ID;
typedef ULONG RPC_PROTSEQ_ID;

typedef struct 
{
   RPC_PROTSEQ_ID ProtseqId;
   ULONG Len;
   sockaddr_t sa;
} *PRPC_ADDR;

typedef struct _RPC_PROTOCOL_VERSION
{
   ULONG MajorVersion;
   ULONG MinorVersion;
} RPC_PROTOCOL_VERSION, *PRPC_PROTOCOL_VERSION;

typedef struct _RPC_BINDING_REP
{
   LIST_ENTRY ListEntry;
   RPC_PROTOCOL_ID ProtocolId;
   LONG ReferenceCount;
   UUID ObjectId;
   PRPC_ADDR RpcAddr;
   BOOLEAN IsServer;
   BOOLEAN AddrIsDynamic;
   PVOID AuthInfo;
   ULONG ExtendedBindFlag;
   ULONG BoundServerInstance;
   ULONG AddrHasEndpoint;
   LONG CallsInProgress;
   PVOID NsSpecific;
   PRPC_PROTOCOL_VERSION ProtocolVersion;
} RPC_BINDING_REP, *PRPC_BINDING_REP;



#endif /* __INCLUDE_RPCRT4_RPC_H */
