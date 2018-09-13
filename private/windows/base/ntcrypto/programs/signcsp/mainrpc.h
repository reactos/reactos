/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0102 */
/* at Fri Mar 08 11:33:02 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __mainrpc_h__
#define __mainrpc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __hello_INTERFACE_DEFINED__
#define __hello_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: hello
 * at Fri Mar 08 11:33:02 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [implicit_handle][version][uuid] */ 


			/* size is 4 */
typedef unsigned long DWORD;

			/* size is 1 */
typedef unsigned char UCHAR;

			/* size is 1 */
boolean GenSignature( 
    /* [size_is][in] */ UCHAR __RPC_FAR *InData,
    /* [out][in] */ DWORD __RPC_FAR *cbSignatureLen,
    /* [size_is][out] */ UCHAR __RPC_FAR *pbSignature);


extern handle_t hello_IfHandle;


extern RPC_IF_HANDLE hello_v1_0_c_ifspec;
extern RPC_IF_HANDLE hello_v1_0_s_ifspec;
#endif /* __hello_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
