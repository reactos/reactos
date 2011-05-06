
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Fri Mar 24 18:32:16 2006
 */
/* Compiler settings for ctx.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __ctx_h__
#define __ctx_h__

/* Forward Declarations */

#ifdef __cplusplus
extern "C"{
#endif

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __hello_INTERFACE_DEFINED__
#define __hello_INTERFACE_DEFINED__

/* interface hello */
/* [implicit_handle][version][uuid] */

typedef long CTXTYPE;

typedef /* [context_handle] */ CTXTYPE __RPC_FAR *PCTXTYPE;

void CtxOpen(
    /* [out] */ PCTXTYPE __RPC_FAR *pphContext,
    /* [in] */ long Value);

void CtxHello(
    /* [in] */ PCTXTYPE phContext);

void CtxClose(
    /* [out][in] */ PCTXTYPE __RPC_FAR *pphContext);


extern handle_t hBinding;


extern RPC_IF_HANDLE hello_v1_0_c_ifspec;
extern RPC_IF_HANDLE hello_v1_0_s_ifspec;
#endif /* __hello_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER PCTXTYPE_rundown( PCTXTYPE );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


