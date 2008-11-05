
#if !defined( RPC_NO_WINDOWS_H ) && !defined( MAC ) && !defined( _MAC )
#if defined (_OLE32_)
#ifndef RC_INVOKED
#include <stdarg.h>
#endif
#include <windef.h>
#include <winbase.h>
#else
#include <windows.h>
#endif
#endif

#ifdef __GNUC__
    #ifndef _SEH_NO_NATIVE_NLG
        /* FIXME ReactOS SEH support, we need remove this when gcc support native seh */
        #include  <libs/pseh/pseh.h>
    #endif
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
typedef long RPC_STATUS;
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

    #if 0
        #define RpcTryExcept __try {
        #define RpcExcept(expr) } __except (expr) {
        #define RpcEndExcept }
        #define RpcTryFinally __try {
        #define RpcFinally } __finally {
        #define RpcEndFinally }
        #define RpcExceptionCode() GetExceptionCode()
        #define RpcAbnormalTermination() AbnormalTermination()
    #else
        /* FIXME ReactOS SEH support, we need remove this when gcc support native seh */

        #ifdef _SEH_NO_NATIVE_NLG
            /* hack for  _SEH_NO_NATIVE_NLG */
                #define RpcTryExcept if (1) {
                #define RpcExcept(expr) } else {
                #define RpcEndExcept }
                #define RpcTryFinally
                #define RpcFinally
                #define RpcEndFinally
                #define RpcExceptionCode() 0
        #else
            #define RpcTryExcept _SEH_TRY {
            #define RpcExcept(expr) } _SEH_HANDLE { \
                                      if (expr) \
                                      {
            #define RpcEndExcept } \
                                 } \
                                 _SEH_END;

            #define RpcTryFinally
            #define RpcFinally
            #define RpcEndFinally
            #define RpcExceptionCode() _SEH_GetExceptionCode()

            /* #define RpcAbnormalTermination() abort() */
        #endif
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


