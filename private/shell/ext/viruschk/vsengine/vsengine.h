/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Fri May 16 15:56:31 1997
 */
/* Compiler settings for vsengine.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __vsengine_h__
#define __vsengine_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __Provider_FWD_DEFINED__
#define __Provider_FWD_DEFINED__

#ifdef __cplusplus
typedef class Provider Provider;
#else
typedef struct Provider Provider;
#endif /* __cplusplus */

#endif 	/* __Provider_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "vrsscan.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __VSENGINELib_LIBRARY_DEFINED__
#define __VSENGINELib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: VSENGINELib
 * at Fri May 16 15:56:31 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_VSENGINELib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Provider;

class DECLSPEC_UUID("68E721E0-CD58-11D0-BD3D-00AA00B92AF1")
Provider;
#endif
#endif /* __VSENGINELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
