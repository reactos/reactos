/*++

Copyright (c) 1991-1995 Microsoft Corporation

Module Name:

    rpcbase.h

Abstract:

    Basic definitions for OLE

--*/

#ifndef __RPCBASE_H__
#define __RPCBASE_H__

#if   (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define __RPC_FAR
#define __RPC_API  __stdcall
#define __RPC_USER __stdcall
#define __RPC_STUB __stdcall
#define RPC_ENTRY  __stdcall
#else
#define __RPC_FAR
#define __RPC_API
#define __RPC_USER
#define __RPC_STUB
#define RPC_ENTRY
#endif

typedef unsigned char byte;

#endif // __RPCBASE_H__
