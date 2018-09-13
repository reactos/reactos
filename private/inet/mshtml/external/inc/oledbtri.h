/* This file contains a few interfaces that were once part of OLE-DB
    but that Trident still uses.  Cut-and-paste from oledb.h (version below).
*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu May 22 17:05:14 1997
 */
/* Compiler settings for C:\oledb\PRIVATE\OLEDB\IDL\oledb.idl:
    Oicf (OptLev=i2), W1, Zp2, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifdef WIN16
#define OLEDBVER 0x0200
#endif

#ifndef __oledbtri_h__
#define __oledbtri_h__

#ifdef __cplusplus
extern "C"{
#endif 

#ifndef __IRowsetExactScroll_FWD_DEFINED__
#define __IRowsetExactScroll_FWD_DEFINED__
typedef interface IRowsetExactScroll IRowsetExactScroll;
#endif 	/* __IRowsetExactScroll_FWD_DEFINED__ */


#ifndef __IRowsetNewRowAfter_FWD_DEFINED__
#define __IRowsetNewRowAfter_FWD_DEFINED__
typedef interface IRowsetNewRowAfter IRowsetNewRowAfter;
#endif 	/* __IRowsetNewRowAfter_FWD_DEFINED__ */


/* header files for imported files */
#ifndef WIN16
#include "wtypes.h"
#endif
#include "oaidl.h"
#include "transact.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


/****************************************
 * Generated header for interface: __MIDL_itf_oledb_0082
 * at Thu May 22 17:05:14 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


#if( OLEDBVER >= 0x0200 )


extern RPC_IF_HANDLE __MIDL_itf_oledb_0082_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oledb_0082_v0_0_s_ifspec;

#ifndef __IRowsetExactScroll_INTERFACE_DEFINED__
#define __IRowsetExactScroll_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetExactScroll
 * at Thu May 22 17:05:14 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetExactScroll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0c733a7f-2a1c-11ce-ade5-00aa0044773d")
    IRowsetExactScroll : public IRowsetScroll
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetExactPosition( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBBKMARK cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ DBCOUNTITEM __RPC_FAR *pulPosition,
            /* [out] */ DBCOUNTITEM __RPC_FAR *pcRows) = 0;
        
    };
    
#else 	/* C style interface */

#error "C style interface not maintained"

#endif 	/* C style interface */

#endif 	/* __IRowsetExactScroll_INTERFACE_DEFINED__ */
#endif /* OLEDBVER >= 0200 */


/****************************************
 * Generated header for interface: __MIDL_itf_oledb_0087
 * at Thu May 22 17:05:14 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


//@@@+ V2.0
#if( OLEDBVER >= 0x0200 )


extern RPC_IF_HANDLE __MIDL_itf_oledb_0087_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oledb_0087_v0_0_s_ifspec;

#ifndef __IRowsetNewRowAfter_INTERFACE_DEFINED__
#define __IRowsetNewRowAfter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetNewRowAfter
 * at Thu May 22 17:05:14 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetNewRowAfter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0c733a71-2a1c-11ce-ade5-00aa0044773d")
    IRowsetNewRowAfter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetNewDataAfter( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBBKMARK cbbmPrevious,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbmPrevious,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ BYTE __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow) = 0;
        
    };
    
#else 	/* C style interface */

#error "C style interface not maintained"

#endif 	/* C style interface */
#endif 	/* __IRowsetNewRowAfter_INTERFACE_DEFINED__ */
#endif /* OLEDBVER >= 0200 */


#if( OLEDBVER >= 0x0200 )
// IID_IRowsetExactScroll		= {0x0c733a7f,0x2a1c,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}
// IID_IRowsetNewRowAfter		= {0x0c733a71,0x2a1c,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}
#endif // OLEDBVER >= 0x0200

#ifdef __cplusplus
}
#endif

#endif

