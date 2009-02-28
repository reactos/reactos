/*
 * Copyright (C) 2000 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_RPCDCEP_H
#define __WINE_RPCDCEP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RPC_VERSION {
    unsigned short MajorVersion;
    unsigned short MinorVersion;
} RPC_VERSION;

typedef struct _RPC_SYNTAX_IDENTIFIER {
    GUID SyntaxGUID;
    RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, *PRPC_SYNTAX_IDENTIFIER;

typedef struct _RPC_MESSAGE
{
    RPC_BINDING_HANDLE Handle;
    ULONG DataRepresentation;
    void* Buffer;
    unsigned int BufferLength;
    unsigned int ProcNum;
    PRPC_SYNTAX_IDENTIFIER TransferSyntax;
    void* RpcInterfaceInformation;
    void* ReservedForRuntime;
    RPC_MGR_EPV* ManagerEpv;
    void* ImportContext;
    ULONG RpcFlags;
} RPC_MESSAGE, *PRPC_MESSAGE;

/* or'ed with ProcNum */
#define RPC_FLAGS_VALID_BIT         0x00008000

#define RPC_CONTEXT_HANDLE_DEFAULT_GUARD ((void *)0xfffff00d)

#define RPC_CONTEXT_HANDLE_DEFAULT_FLAGS    0x00000000
#define RPC_CONTEXT_HANDLE_FLAGS            0x30000000
#define RPC_CONTEXT_HANDLE_SERIALIZE        0x10000000
#define RPC_CONTEXT_HANDLE_DONT_SERIALIZE   0x20000000
#define RPC_TYPE_STRICT_CONTEXT_HANDLE      0x40000000

#define RPC_NCA_FLAGS_DEFAULT       0x00000000
#define RPC_NCA_FLAGS_IDEMPOTENT    0x00000001
#define RPC_NCA_FLAGS_BROADCAST     0x00000002
#define RPC_NCA_FLAGS_MAYBE         0x00000004

#define RPC_BUFFER_COMPLETE         0x00001000
#define RPC_BUFFER_PARTIAL          0x00002000
#define RPC_BUFFER_EXTRA            0x00004000
#define RPC_BUFFER_ASYNC            0x00008000
#define RPC_BUFFER_NONOTIFY         0x00010000

#define RPCFLG_MESSAGE              0x01000000
#define RPCFLG_HAS_MULTI_SYNTAXES   0x02000000
#define RPCFLG_HAS_CALLBACK         0x04000000
#define RPCFLG_AUTO_COMPLETE        0x08000000
#define RPCFLG_LOCAL_CALL           0x10000000
#define RPCFLG_INPUT_SYNCHRONOUS    0x20000000
#define RPCFLG_ASYNCHRONOUS         0x40000000
#define RPCFLG_NON_NDR              0x80000000

typedef void  (__RPC_STUB *RPC_DISPATCH_FUNCTION)(PRPC_MESSAGE Message);
typedef RPC_STATUS (RPC_ENTRY *RPC_FORWARD_FUNCTION)(UUID *InterfaceId, RPC_VERSION *InterfaceVersion, UUID *ObjectId, unsigned char *Rpcpro, void **ppDestEndpoint);

typedef struct
{
    unsigned int DispatchTableCount;
    RPC_DISPATCH_FUNCTION* DispatchTable;
    LONG_PTR Reserved;
} RPC_DISPATCH_TABLE, *PRPC_DISPATCH_TABLE;

typedef struct _RPC_PROTSEQ_ENDPOINT
{
    unsigned char* RpcProtocolSequence;
    unsigned char* Endpoint;
} RPC_PROTSEQ_ENDPOINT, *PRPC_PROTSEQ_ENDPOINT;

#define NT351_INTERFACE_SIZE 0x40
#define RPC_INTERFACE_HAS_PIPES 0x0001

typedef struct _RPC_SERVER_INTERFACE
{
    unsigned int Length;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    PRPC_DISPATCH_TABLE DispatchTable;
    unsigned int RpcProtseqEndpointCount;
    PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
    RPC_MGR_EPV* DefaultManagerEpv;
    void const* InterpreterInfo;
    unsigned int Flags;
} RPC_SERVER_INTERFACE, *PRPC_SERVER_INTERFACE;

typedef struct _RPC_CLIENT_INTERFACE
{
    unsigned int Length;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    PRPC_DISPATCH_TABLE DispatchTable;
    unsigned int RpcProtseqEndpointCount;
    PRPC_PROTSEQ_ENDPOINT RpcProtseqEndpoint;
    ULONG_PTR Reserved;
    void const* InterpreterInfo;
    unsigned int Flags;
} RPC_CLIENT_INTERFACE, *PRPC_CLIENT_INTERFACE;

#define TRANSPORT_TYPE_CN   0x01
#define TRANSPORT_TYPE_DG   0x02
#define TRANSPORT_TYPE_LPC  0x04
#define TRANSPORT_TYPE_WMSG 0x08

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcNegotiateTransferSyntax( RPC_MESSAGE* Message );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcGetBuffer( RPC_MESSAGE* Message );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcGetBufferWithObject( RPC_MESSAGE* Message, UUID* ObjectUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcSendReceive( RPC_MESSAGE* Message );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcFreeBuffer( RPC_MESSAGE* Message );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcSend( RPC_MESSAGE* Message );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcReceive( RPC_MESSAGE* Message );

RPCRTAPI void* RPC_ENTRY
  I_RpcAllocate( unsigned int Size );
RPCRTAPI void RPC_ENTRY
  I_RpcFree( void* Object );

RPCRTAPI RPC_BINDING_HANDLE RPC_ENTRY
  I_RpcGetCurrentCallHandle( void );

/*
 * The platform SDK headers don't define these functions at all if WINNT is defined
 * The MSVC6 headers define two different sets of functions :
 *  If WINNT and MSWMSG are defined, the NT versions are defined
 *  If WINNT is not defined, the windows 9x versions are defined.
 * Note that the prototypes for I_RpcBindingSetAsync are different for each case.
 *
 * Wine defaults to the WinNT case and only defines these function is MSWMSG is
 *  defined. Defining the NT functions by default causes MIDL generated proxies
 *  to not compile.
 */

#if 1  /* WINNT */
#ifdef MSWMSG

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStartListening( HWND hWnd );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStopListening( void );
/* WINNT */
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_GetThreadWindowHandle( HWND* hWnd );
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcAsyncSendReceive( RPC_MESSAGE* Message, void* Context, HWND hWnd );

typedef RPC_STATUS (*RPC_BLOCKING_FN)(void* hWnd, void* Context, HANDLE hSyncEvent);

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcBindingSetAsync( RPC_BINDING_HANDLE Binding, RPC_BLOCKING_FN BlockingFn );

RPCRTAPI UINT RPC_ENTRY
  I_RpcWindowProc( void* hWnd, UINT Message, UINT wParam, ULONG lParam );

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcSetWMsgEndpoint( WCHAR* Endpoint );

#endif

#else

/* WIN9x */
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStartListening( void* hWnd );

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStopListening( void );

typedef RPC_STATUS (*RPC_BLOCKING_FN)(void* hWnd, void* Context, void* hSyncEvent);

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcBindingSetAsync( RPC_BINDING_HANDLE Binding, RPC_BLOCKING_FN BlockingFn, ULONG ServerTid );

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcSetThreadParams( int fClientFree, void* Context, void* hWndClient );

RPCRTAPI UINT RPC_ENTRY
  I_RpcWindowProc( void* hWnd, unsigned int Message, unsigned int wParam, ULONG lParam );

#endif

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcBindingInqTransportType( RPC_BINDING_HANDLE Binding, unsigned int* Type );

RPCRTAPI LONG RPC_ENTRY I_RpcMapWin32Status(RPC_STATUS);

#ifdef __cplusplus
}
#endif

#endif /*__WINE_RPCDCEP_H */
