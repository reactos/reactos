/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Fri Nov 14 11:20:18 1997
 */
/* Compiler settings for ShellExtensions.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __ShellExtensions_h__
#define __ShellExtensions_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __EI_FWD_DEFINED__
#define __EI_FWD_DEFINED__

#ifdef __cplusplus
typedef class EI EI;
#else
typedef struct EI EI;
#endif /* __cplusplus */

#endif 	/* __EI_FWD_DEFINED__ */


#ifndef __CM_FWD_DEFINED__
#define __CM_FWD_DEFINED__

#ifdef __cplusplus
typedef class CM CM;
#else
typedef struct CM CM;
#endif /* __cplusplus */

#endif 	/* __CM_FWD_DEFINED__ */


#ifndef __PS_FWD_DEFINED__
#define __PS_FWD_DEFINED__

#ifdef __cplusplus
typedef class PS PS;
#else
typedef struct PS PS;
#endif /* __cplusplus */

#endif 	/* __PS_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "shlobj.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ShellExtensionsLib_LIBRARY_DEFINED__
#define __ShellExtensionsLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ShellExtensionsLib
 * at Fri Nov 14 11:20:18 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_ShellExtensionsLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_EI;

class DECLSPEC_UUID("B0C53C9E-5B8D-11D1-8CC9-00C04FD918D0")
EI;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CM;

class DECLSPEC_UUID("D571A8F6-5AE0-11D1-8CC9-00C04FD918D0")
CM;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_PS;

class DECLSPEC_UUID("8DEDB8E3-5C6D-11D1-8CCD-00C04FD918D0")
PS;
#endif
#endif /* __ShellExtensionsLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
