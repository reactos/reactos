#ifndef RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#ifndef _RPC_H
#define _RPC_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define __RPC_WIN32__
#ifndef _WIN95
#define __RPC_NT__
#define RPC_UNICODE_SUPPORTED
#endif

#ifdef _RPCRT4_
#define RPCRTAPI DECLSPEC_EXPORT
#else
#define RPCRTAPI DECLSPEC_IMPORT
#endif

#ifndef __MIDL_USER_DEFINED
#define midl_user_allocate MIDL_user_allocate
#define midl_user_free     MIDL_user_free
#define __MIDL_USER_DEFINED
#endif
#define RPC_UNICODE_SUPPORTED
#define __RPC_FAR
#define __RPC_API  __stdcall
#define __RPC_USER __stdcall
#define __RPC_STUB __stdcall
#define RPC_ENTRY  __stdcall
typedef void *I_RPC_HANDLE;
typedef long RPC_STATUS;

#include <rpcdce.h>
#include <rpcnsi.h>
#include <rpcnterr.h>

#include <winerror.h>

/* SEH is not supported */
#if 0
#include <excpt.h>
#define RpcTryExcept __try {
#define RpcExcept(x) } __except (x) {
#define RpcEndExcept }
#define RpcTryFinally __try {
#define RpcFinally } __finally {
#define RpcEndFinally }
#define RpcExceptionCode() GetExceptionCode()
#define RpcAbnormalTermination() AbnormalTermination()
#endif /* 0 */

RPC_STATUS RPC_ENTRY RpcImpersonateClient(RPC_BINDING_HANDLE);
RPC_STATUS RPC_ENTRY RpcRevertToSelf(void);
long RPC_ENTRY I_RpcMapWin32Status(RPC_STATUS);
#ifdef __cplusplus
}
#endif
#endif
