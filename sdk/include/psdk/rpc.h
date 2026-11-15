
#if !defined( RPC_NO_WINDOWS_H ) && !defined( MAC ) && !defined( _MAC )
#ifndef _INC_WINDOWS
#include <windows.h>
#endif /* _INC_WINDOWS */
#endif

#ifndef __RPC_H__
#define __RPC_H__

#if _MSC_VER > 1000
#pragma once
#endif


#ifdef __cplusplus
extern "C" {
#endif

#if defined( MAC ) || defined( _MAC ) || defined(__powerpc__) && !defined(__REACTOS__)
    #define __RPC_MAC__
    #define __RPC_WIN32__
    #include <pshpack2.h>

#else
    #if defined(_M_IA64) || defined(_M_AMD64) || defined(_WIN64)
        #define __RPC_WIN64__
    #else
        #define __RPC_WIN32__
    #endif
#endif

#include <basetsd.h>

#if defined(__RPC_WIN64__)
    #include <pshpack8.h>
#endif

#ifndef __MIDL_USER_DEFINED
    #define midl_user_allocate MIDL_user_allocate
    #define midl_user_free MIDL_user_free
    #define __MIDL_USER_DEFINED
#endif


typedef void * I_RPC_HANDLE;
#ifndef __ROS_LONG64__
typedef long RPC_STATUS;
#else
typedef int RPC_STATUS;
#endif
#define __RPC_FAR

#if defined(__RPC_WIN32__) || defined(__RPC_WIN64__)
    #define RPC_UNICODE_SUPPORTED
#endif


#if !defined(__RPC_MAC__)
    #define __RPC_API  __stdcall
    #define __RPC_USER __stdcall
    #define __RPC_STUB __stdcall
    #define  RPC_ENTRY __stdcall
#else
    #define __RPC_API
    #define __RPC_USER
    #define __RPC_STUB
    #define RPC_ENTRY
#endif


#ifndef __GNUC__
    #if !defined(DECLSPEC_IMPORT)
            #define DECLSPEC_IMPORT
    #endif

    #if !defined(_RPCRT4_)
        #define RPCRTAPI DECLSPEC_IMPORT
    #else
        #define RPCRTAPI
    #endif

    #if !defined(_RPCNS4_)
        #define RPCNSAPI DECLSPEC_IMPORT
    #else
        #define RPCNSAPI
    #endif
#else
    #define RPCRTAPI
    #define RPCNSAPI
#endif

#ifdef __RPC_MAC__
    #include <setjmp.h>
    #define RPCXCWORD (sizeof(jmp_buf)/sizeof(int))

    #pragma warning(push)
    #pragma warning( disable: 4005 )
    #include <rpcdce.h>
    #include <rpcnsi.h>
    #include <rpcerr.h>
    #include <rpcmac.h>
    #pragma warning(pop)

    typedef void  (RPC_ENTRY *MACYIELDCALLBACK)(short *) ;
    RPC_STATUS RPC_ENTRY
    RpcMacSetYieldInfo(MACYIELDCALLBACK pfnCallback) ;

    #if !defined(UNALIGNED)
        #define UNALIGNED
    #endif

    #include <poppack.h>
#else
    #include <rpcdce.h>
    /* #include <rpcnsi.h> */
    #include <rpcnterr.h>
    #include <excpt.h>
    #include <winerror.h>

    #define RpcTryExcept _SEH2_TRY
    #define RpcExcept(expr) _SEH2_EXCEPT((expr))
    #define RpcEndExcept _SEH2_END;
    #define RpcTryFinally _SEH2_TRY
    #define RpcFinally _SEH2_FINALLY
    #define RpcEndFinally _SEH2_END;
    #define RpcExceptionCode() _SEH2_GetExceptionCode()
    #define RpcAbnormalTermination() (_SEH2_GetExceptionCode() != 0)

    /* Copyright 2026 Ryan Wilson <ryan@verosnt.com>
     * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
     * Clang amd64 has no SEH plugin, so RpcTryExcept does not catch.
     * An uncaught RPC exception kills the caller, including winlogon. */
#if defined(__clang__) && (defined(_M_AMD64) || defined(__x86_64__)) && !defined(_NTDDK_)
#include <setjmp.h>
typedef struct _RPC_SEH_FRAME
{
    jmp_buf Jmp;
    volatile RPC_STATUS Code;
    struct _RPC_SEH_FRAME *Previous;
} RPC_SEH_FRAME;
void RPC_ENTRY I_RpcSehPushFrame(RPC_SEH_FRAME *Frame);
void RPC_ENTRY I_RpcSehPopFrame(RPC_SEH_FRAME *Frame);
#undef RpcTryExcept
#undef RpcExcept
#undef RpcEndExcept
#undef RpcExceptionCode
#define RpcTryExcept \
    do { \
        RPC_SEH_FRAME _rpc_frame __attribute__((aligned(16))); \
        _rpc_frame.Code = RPC_S_OK; \
        I_RpcSehPushFrame(&_rpc_frame); \
        if (setjmp(_rpc_frame.Jmp) == 0) {
#define RpcExcept(expr) \
            I_RpcSehPopFrame(&_rpc_frame); \
        } else if (expr) {
#define RpcEndExcept \
        } \
    } while (0);
#define RpcExceptionCode() (_rpc_frame.Code)
#endif
#endif

#if defined(__RPC_WIN64__)
    #include <poppack.h>
#endif

#ifndef RPC_NO_WINDOWS_H
#include <rpcasync.h>
#endif

#ifdef __cplusplus
}
#endif

#endif


