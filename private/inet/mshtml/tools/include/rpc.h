/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    rpc.h

Abstract:

    Master include file for RPC applications.

--*/

#ifndef RPC_NO_WINDOWS_H
#include <windows.h>
#endif // RPC_NO_WINDOWS_H

#ifndef __RPC_H__
#define __RPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define __RPC_WIN32__
#define __RPC_NT__

#ifndef __MIDL_USER_DEFINED
#define midl_user_allocate MIDL_user_allocate
#define midl_user_free     MIDL_user_free
#define __MIDL_USER_DEFINED
#endif

typedef void * I_RPC_HANDLE;
typedef long RPC_STATUS;

#define RPC_UNICODE_SUPPORTED
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

#ifdef IN
#undef IN
#undef OUT
#undef OPTIONAL
#endif /* IN */

#include <rpcdce.h>
#include <rpcnsi.h>
#include <rpcnterr.h>


#include <excpt.h>
#include <winerror.h>

#define RpcTryExcept \
    __try \
        {

// trystmts

#define RpcExcept(expr) \
        } \
    __except (expr) \
        {

// exceptstmts

#define RpcEndExcept \
        }

#define RpcTryFinally \
    __try \
        {

// trystmts

#define RpcFinally \
        } \
    __finally \
        {

// finallystmts

#define RpcEndFinally \
        }

#define RpcExceptionCode() GetExceptionCode()
#define RpcAbnormalTermination() AbnormalTermination()

RPC_STATUS RPC_ENTRY
RpcImpersonateClient (
    IN RPC_BINDING_HANDLE BindingHandle OPTIONAL
    );

RPC_STATUS RPC_ENTRY
RpcRevertToSelfEx (
    IN RPC_BINDING_HANDLE BindingHandle OPTIONAL
    );

RPC_STATUS RPC_ENTRY
RpcRevertToSelf (
    );

long RPC_ENTRY
I_RpcMapWin32Status (
    IN RPC_STATUS Status
    );

#ifdef __cplusplus
}
#endif

#endif // __RPC_H__

