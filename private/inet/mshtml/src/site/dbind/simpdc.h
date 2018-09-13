/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Tue Aug 25 10:20:38 1998
 */
/* Compiler settings for S:\zen\ocp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )


#ifndef I_SIMPDC_H_
#define I_SIMPDC_H_

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISimpleDataConverter_FWD_DEFINED__
#define __ISimpleDataConverter_FWD_DEFINED__
typedef interface ISimpleDataConverter ISimpleDataConverter;
#endif 	/* __ISimpleDataConverter_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ISimpleDataConverter_INTERFACE_DEFINED__
#define __ISimpleDataConverter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISimpleDataConverter
 * at Tue Aug 25 10:20:38 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [hidden][helpcontext][unique][uuid][object] */ 



DEFINE_GUID(IID_ISimpleDataConverter,0x78667670,0x3C3D,0x11d2,0x91,0xF9,0x00,0x60,0x97,0xC9,0x7F,0x9B);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("78667670-3C3D-11d2-91F9-006097C97F9B")
    ISimpleDataConverter : public IUnknown
    {
    public:
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ConvertData( 
            VARIANT varSrc,
            long vtDest,
            IUnknown __RPC_FAR *pUnknownElement,
            VARIANT __RPC_FAR *pvarDest) = 0;
        
        virtual /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CanConvertData( 
            long vt1,
            long vt2) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleDataConverterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISimpleDataConverter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISimpleDataConverter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISimpleDataConverter __RPC_FAR * This);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConvertData )( 
            ISimpleDataConverter __RPC_FAR * This,
            VARIANT varSrc,
            long vtDest,
            IUnknown __RPC_FAR *pUnknownElement,
            VARIANT __RPC_FAR *pvarDest);
        
        /* [helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanConvertData )( 
            ISimpleDataConverter __RPC_FAR * This,
            long vt1,
            long vt2);
        
        END_INTERFACE
    } ISimpleDataConverterVtbl;

    interface ISimpleDataConverter
    {
        CONST_VTBL struct ISimpleDataConverterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleDataConverter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleDataConverter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleDataConverter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleDataConverter_ConvertData(This,varSrc,vtDest,pUnknownElement,pvarDest)	\
    (This)->lpVtbl -> ConvertData(This,varSrc,vtDest,pUnknownElement,pvarDest)

#define ISimpleDataConverter_CanConvertData(This,vt1,vt2)	\
    (This)->lpVtbl -> CanConvertData(This,vt1,vt2)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext] */ HRESULT STDMETHODCALLTYPE ISimpleDataConverter_ConvertData_Proxy( 
    ISimpleDataConverter __RPC_FAR * This,
    VARIANT varSrc,
    long vtDest,
    IUnknown __RPC_FAR *pUnknownElement,
    VARIANT __RPC_FAR *pvarDest);


void __RPC_STUB ISimpleDataConverter_ConvertData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext] */ HRESULT STDMETHODCALLTYPE ISimpleDataConverter_CanConvertData_Proxy( 
    ISimpleDataConverter __RPC_FAR * This,
    long vt1,
    long vt2);


void __RPC_STUB ISimpleDataConverter_CanConvertData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleDataConverter_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif I_SIMPDC_H_
