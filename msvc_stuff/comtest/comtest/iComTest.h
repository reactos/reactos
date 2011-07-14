

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sun Jul 03 16:48:35 2011
 */
/* Compiler settings for iComTest.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __iComTest_h__
#define __iComTest_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IComTest_FWD_DEFINED__
#define __IComTest_FWD_DEFINED__
typedef interface IComTest IComTest;
#endif 	/* __IComTest_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IComTest_INTERFACE_DEFINED__
#define __IComTest_INTERFACE_DEFINED__

/* interface IComTest */
/* [uuid][object] */ 


EXTERN_C const IID IID_IComTest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("10CDF249-A336-406F-B472-20F08660D609")
    IComTest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE test( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComTestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComTest * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComTest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComTest * This);
        
        HRESULT ( STDMETHODCALLTYPE *test )( 
            IComTest * This);
        
        END_INTERFACE
    } IComTestVtbl;

    interface IComTest
    {
        CONST_VTBL struct IComTestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComTest_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IComTest_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IComTest_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IComTest_test(This)	\
    ( (This)->lpVtbl -> test(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IComTest_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


