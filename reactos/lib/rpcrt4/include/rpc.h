/*
 * 
 */

#ifndef __LIB_RPCRT4_INCLUDE_RPC_H
#define __LIB_RPCRT4_INCLUDE_RPC_H

#define RPC_MGR_EPV     VOID
typedef RPC_BINDING_HANDLE handle_t

/*
 * RPC packet types
 */
#define RPC_REQUEST    (0x0)
#define RPC_PING       (0x1)
#define RPC_RESPONSE   (0x2)
#define RPC_FAULT      (0x3)
#define RPC_WORKING    (0x4)
#define RPC_NOCALL     (0x5)
#define RPC_REJECT     (0x6)
#define RPC_ACK        (0x7)
#define RPC_CL_CANCEL  (0x8)
#define RPC_FACK       (0x9)
#define RPC_CANCEL_ACK (0xa)
#define RPC_BIND       (0xb)
#define RPC_BINDACK    (0xc)
#define RPC_BINDNACK   (0xd)
#define RPC_ALTCONT    (0xe)
#define RPC_AUTH3      (0xf)
#define RPC_BINDCONT   (0x10)
#define RPC_SHUTDOWN   (0x10)
#define RPC_CO_CANCEL  (0x11)
#define RPC_ORPHANED   (0x12)

/*
 * Common RPC packet header
 */
typedef struct _RPC_HEADER_PACKET
{
   UCHAR MajorVersion;
   UCHAR MinorVersion;
   UCHAR PacketType;
   UCHAR Flags;
   ULONG DataRep;
   USHORT FragLen;
   USHORT AuthLen;
   ULONG CallId;
} RPC_HEADER_PACKET, *PRPC_HEADER_PACKET;

/*
 * Additional header for a RPC request packet
 */
typedef struct _RPC_REQUEST_PACKET
{
   ULONG AllocHint;
   USHORT ContextId;
   USHORT Opcode;
} RPC_REQUEST_PACKET, *PRPC_REQUEST_PACKET;

typedef struct _RPC_RESPONSE_PACKET
{
   ULONG AllocHint;
   USHORT ContextId;
   UCHAR CancelCount;
   UCHAR Reserved;
} RPC_RESPONSE_PACKET, *PRPC_RESPONSE_PACKET;

typedef struct _RPC_VERSION
{
   USHORT MajorVersion;
   USHORT MinorVersion;
} RPC_VERSION, *PRPC_VERSION;

typedef struct _RPC_SYNTAX_IDENTIFIER
{
   GUID SyntaxGuid;
   RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, *PRPC_SYNTAX_IDENTIFIER;

typedef struct _RPC_MESSAGE
{
   RPC_BINDING_HANDLE Handle;
   ULONG DataRepresentation;
   VOID* Buffer;
   USHORT BufferLength;
   USHORT ProcNum;
   PRPC_SYNTAX_IDENTIFIER TransferSyntax;
   VOID* RpcInterfaceInformation;
   VOID* ReservedForRuntime;
   RPC_MGR_EPV* ManagerEpv;
   VOID* ImportContext;
   ULONG RpcFlags;
} RPC_MESSAGE, *PRPC_MESSAGE;

typedef VOID (*RPC_DISPATCH_FUNCTION)(PRPC_MESSAGE Message);

typedef struct _RPC_DISPATCH_TABLE
{
   ULONG DispatchTableCount;
   RPC_DISPATCH_FUNCTION* DispatchTable;
   LONG Reserved;
} RPC_DISPATCH_TABLE, *PRPC_DISPATCH_TABLE;

typedef struct _RPC_SERVER_INTERFACE
{
   ULONG Length;
   RPC_SYNTAX_IDENTIFIER InterfaceId;
   RPC_SYNTAX_IDENTIFIER TransferSyntax;
   PRPC_DISPATCH_TABLE DispatchTable;
   ULONG RpcProtseqEndpointCount;
   PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
   RPC_MGR_EPV* DefaultManagerEpv;
   VOID CONST* InterpreterInfo;
   ULONG Flags;
} RPC_SERVER_INTERFACE, *PRPC_SERVER_INTERFACE;

#endif /* __LIB_RPCRT4_INCLUDE_RPC_H */
