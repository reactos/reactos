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
    unsigned long DataRepresentation;
    void* Buffer;
    unsigned int BufferLength;
    unsigned int ProcNum;
    PRPC_SYNTAX_IDENTIFIER TransferSyntax;
    void* RpcInterfaceInformation;
    void* ReservedForRuntime;
    RPC_MGR_EPV* ManagerEpv;
    void* ImportContext;
    unsigned long RpcFlags;
} RPC_MESSAGE, *PRPC_MESSAGE;

#define RPC_NCA_FLAGS_DEFAULT       0x00000000
#define RPC_NCA_FLAGS_IDEMPOTENT    0x00000001
#define RPC_NCA_FLAGS_BROADCAST     0x00000002
#define RPC_NCA_FLAGS_MAYBE         0x00000004

typedef void  (__RPC_STUB *RPC_DISPATCH_FUNCTION)(PRPC_MESSAGE Message);

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
 *  defined. Defining the NT functions by default causes MIDL generated proxys
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

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcBindingInqTransportType( RPC_BINDING_HANDLE Binding, unsigned int* Type );

#endif

#else

/* WIN9x */
RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStartListening( void* hWnd );

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcServerStopListening( void );

typedef RPC_STATUS (*RPC_BLOCKING_FN)(void* hWnd, void* Context, void* hSyncEvent);

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcBindingSetAsync( RPC_BINDING_HANDLE Binding, RPC_BLOCKING_FN BlockingFn, unsigned long ServerTid );

RPCRTAPI RPC_STATUS RPC_ENTRY
  I_RpcSetThreadParams( int fClientFree, void* Context, void* hWndClient );

RPCRTAPI UINT RPC_ENTRY
  I_RpcWindowProc( void* hWnd, unsigned int Message, unsigned int wParam, unsigned long lParam );

#endif

#endif /*__WINE_RPCDCEP_H */
