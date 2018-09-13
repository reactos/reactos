/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.00.0140 */
/* at Thu Mar 11 12:57:09 1999
 */
/* Compiler settings for regwizctrl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


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

#ifndef __regwizctrl_h__
#define __regwizctrl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRegWizCtrl_FWD_DEFINED__
#define __IRegWizCtrl_FWD_DEFINED__
typedef interface IRegWizCtrl IRegWizCtrl;
#endif 	/* __IRegWizCtrl_FWD_DEFINED__ */


#ifndef __RegWizCtrl_FWD_DEFINED__
#define __RegWizCtrl_FWD_DEFINED__

#ifdef __cplusplus
typedef class RegWizCtrl RegWizCtrl;
#else
typedef struct RegWizCtrl RegWizCtrl;
#endif /* __cplusplus */

#endif 	/* __RegWizCtrl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IRegWizCtrl_INTERFACE_DEFINED__
#define __IRegWizCtrl_INTERFACE_DEFINED__

/* interface IRegWizCtrl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRegWizCtrl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50E5E3CF-C07E-11D0-B9FD-00A0249F6B00")
    IRegWizCtrl : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsRegistered( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbStatus) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_IsRegistered( 
            /* [in] */ BSTR strText) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InvokeRegWizard( 
            BSTR ProductPath) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegWizCtrlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRegWizCtrl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRegWizCtrl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsRegistered )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbStatus);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsRegistered )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [in] */ BSTR strText);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvokeRegWizard )( 
            IRegWizCtrl __RPC_FAR * This,
            BSTR ProductPath);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IRegWizCtrl __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IRegWizCtrlVtbl;

    interface IRegWizCtrl
    {
        CONST_VTBL struct IRegWizCtrlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegWizCtrl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegWizCtrl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegWizCtrl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegWizCtrl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRegWizCtrl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRegWizCtrl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRegWizCtrl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRegWizCtrl_get_IsRegistered(This,pbStatus)	\
    (This)->lpVtbl -> get_IsRegistered(This,pbStatus)

#define IRegWizCtrl_put_IsRegistered(This,strText)	\
    (This)->lpVtbl -> put_IsRegistered(This,strText)

#define IRegWizCtrl_InvokeRegWizard(This,ProductPath)	\
    (This)->lpVtbl -> InvokeRegWizard(This,ProductPath)

#define IRegWizCtrl_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRegWizCtrl_get_IsRegistered_Proxy( 
    IRegWizCtrl __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbStatus);


void __RPC_STUB IRegWizCtrl_get_IsRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRegWizCtrl_put_IsRegistered_Proxy( 
    IRegWizCtrl __RPC_FAR * This,
    /* [in] */ BSTR strText);


void __RPC_STUB IRegWizCtrl_put_IsRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRegWizCtrl_InvokeRegWizard_Proxy( 
    IRegWizCtrl __RPC_FAR * This,
    BSTR ProductPath);


void __RPC_STUB IRegWizCtrl_InvokeRegWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRegWizCtrl_get_Version_Proxy( 
    IRegWizCtrl __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IRegWizCtrl_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegWizCtrl_INTERFACE_DEFINED__ */



#ifndef __REGWIZCTRLLib_LIBRARY_DEFINED__
#define __REGWIZCTRLLib_LIBRARY_DEFINED__

/* library REGWIZCTRLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_REGWIZCTRLLib;

EXTERN_C const CLSID CLSID_RegWizCtrl;

#ifdef __cplusplus

class DECLSPEC_UUID("50E5E3D1-C07E-11D0-B9FD-00A0249F6B00")
RegWizCtrl;
#endif
#endif /* __REGWIZCTRLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
