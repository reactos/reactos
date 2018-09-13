/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Sep 29 10:54:33 1998
 */
/* Compiler settings for ISCSa.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef UNIX
#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__
#endif

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ISCSa_h__
#define __ISCSa_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IInputSequenceChecker_FWD_DEFINED__
#define __IInputSequenceChecker_FWD_DEFINED__
typedef interface IInputSequenceChecker IInputSequenceChecker;
#endif 	/* __IInputSequenceChecker_FWD_DEFINED__ */


#ifndef __IEnumInputSequenceCheckers_FWD_DEFINED__
#define __IEnumInputSequenceCheckers_FWD_DEFINED__
typedef interface IEnumInputSequenceCheckers IEnumInputSequenceCheckers;
#endif 	/* __IEnumInputSequenceCheckers_FWD_DEFINED__ */


#ifndef __IInputSequenceCheckerContainer_FWD_DEFINED__
#define __IInputSequenceCheckerContainer_FWD_DEFINED__
typedef interface IInputSequenceCheckerContainer IInputSequenceCheckerContainer;
#endif 	/* __IInputSequenceCheckerContainer_FWD_DEFINED__ */


#ifndef __ISCThai_FWD_DEFINED__
#define __ISCThai_FWD_DEFINED__

#ifdef __cplusplus
typedef class ISCThai ISCThai;
#else
typedef struct ISCThai ISCThai;
#endif /* __cplusplus */

#endif 	/* __ISCThai_FWD_DEFINED__ */


#ifndef __InputSequenceCheckerContainer_FWD_DEFINED__
#define __InputSequenceCheckerContainer_FWD_DEFINED__

#ifdef __cplusplus
typedef class InputSequenceCheckerContainer InputSequenceCheckerContainer;
#else
typedef struct InputSequenceCheckerContainer InputSequenceCheckerContainer;
#endif /* __cplusplus */

#endif 	/* __InputSequenceCheckerContainer_FWD_DEFINED__ */


#ifndef __EnumInputSequenceCheckers_FWD_DEFINED__
#define __EnumInputSequenceCheckers_FWD_DEFINED__

#ifdef __cplusplus
typedef class EnumInputSequenceCheckers EnumInputSequenceCheckers;
#else
typedef struct EnumInputSequenceCheckers EnumInputSequenceCheckers;
#endif /* __cplusplus */

#endif 	/* __EnumInputSequenceCheckers_FWD_DEFINED__ */


#ifndef __ISCHindi_FWD_DEFINED__
#define __ISCHindi_FWD_DEFINED__

#ifdef __cplusplus
typedef class ISCHindi ISCHindi;
#else
typedef struct ISCHindi ISCHindi;
#endif /* __cplusplus */

#endif 	/* __ISCHindi_FWD_DEFINED__ */


#ifndef __ISCVietnamese_FWD_DEFINED__
#define __ISCVietnamese_FWD_DEFINED__

#ifdef __cplusplus
typedef class ISCVietnamese ISCVietnamese;
#else
typedef struct ISCVietnamese ISCVietnamese;
#endif /* __cplusplus */

#endif 	/* __ISCVietnamese_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IInputSequenceChecker_INTERFACE_DEFINED__
#define __IInputSequenceChecker_INTERFACE_DEFINED__

/* interface IInputSequenceChecker */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IInputSequenceChecker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6CF60DE0-42DC-11D2-BE22-080009DC0A8D")
    IInputSequenceChecker : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLCID( 
            /* [out] */ LCID __RPC_FAR *plcid) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckInputSequence( 
            /* [string][in] */ LPWSTR pCharBuffer,
            /* [in] */ UINT ichPosition,
            /* [in] */ WCHAR chEval,
            /* [out] */ BOOL __RPC_FAR *pfValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CheckAndReplaceInputSequence( 
            /* [size_is][in] */ LPWSTR pCharBuffer,
            /* [in] */ UINT cchCharBuffer,
            /* [in] */ UINT ichPosition,
            /* [in] */ WCHAR chEval,
            /* [in] */ UINT cchBuffer,
            /* [size_is][out][in] */ LPWSTR pOutBuffer,
            /* [out] */ UINT __RPC_FAR *pchOutBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInputSequenceCheckerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IInputSequenceChecker __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IInputSequenceChecker __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IInputSequenceChecker __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLCID )( 
            IInputSequenceChecker __RPC_FAR * This,
            /* [out] */ LCID __RPC_FAR *plcid);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckInputSequence )( 
            IInputSequenceChecker __RPC_FAR * This,
            /* [string][in] */ LPWSTR pCharBuffer,
            /* [in] */ UINT ichPosition,
            /* [in] */ WCHAR chEval,
            /* [out] */ BOOL __RPC_FAR *pfValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckAndReplaceInputSequence )( 
            IInputSequenceChecker __RPC_FAR * This,
            /* [size_is][in] */ LPWSTR pCharBuffer,
            /* [in] */ UINT cchCharBuffer,
            /* [in] */ UINT ichPosition,
            /* [in] */ WCHAR chEval,
            /* [in] */ UINT cchBuffer,
            /* [size_is][out][in] */ LPWSTR pOutBuffer,
            /* [out] */ UINT __RPC_FAR *pchOutBuffer);
        
        END_INTERFACE
    } IInputSequenceCheckerVtbl;

    interface IInputSequenceChecker
    {
        CONST_VTBL struct IInputSequenceCheckerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInputSequenceChecker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInputSequenceChecker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInputSequenceChecker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInputSequenceChecker_GetLCID(This,plcid)	\
    (This)->lpVtbl -> GetLCID(This,plcid)

#define IInputSequenceChecker_CheckInputSequence(This,pCharBuffer,ichPosition,chEval,pfValue)	\
    (This)->lpVtbl -> CheckInputSequence(This,pCharBuffer,ichPosition,chEval,pfValue)

#define IInputSequenceChecker_CheckAndReplaceInputSequence(This,pCharBuffer,cchCharBuffer,ichPosition,chEval,cchBuffer,pOutBuffer,pchOutBuffer)	\
    (This)->lpVtbl -> CheckAndReplaceInputSequence(This,pCharBuffer,cchCharBuffer,ichPosition,chEval,cchBuffer,pOutBuffer,pchOutBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IInputSequenceChecker_GetLCID_Proxy( 
    IInputSequenceChecker __RPC_FAR * This,
    /* [out] */ LCID __RPC_FAR *plcid);


void __RPC_STUB IInputSequenceChecker_GetLCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IInputSequenceChecker_CheckInputSequence_Proxy( 
    IInputSequenceChecker __RPC_FAR * This,
    /* [string][in] */ LPWSTR pCharBuffer,
    /* [in] */ UINT ichPosition,
    /* [in] */ WCHAR chEval,
    /* [out] */ BOOL __RPC_FAR *pfValue);


void __RPC_STUB IInputSequenceChecker_CheckInputSequence_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IInputSequenceChecker_CheckAndReplaceInputSequence_Proxy( 
    IInputSequenceChecker __RPC_FAR * This,
    /* [size_is][in] */ LPWSTR pCharBuffer,
    /* [in] */ UINT cchCharBuffer,
    /* [in] */ UINT ichPosition,
    /* [in] */ WCHAR chEval,
    /* [in] */ UINT cchBuffer,
    /* [size_is][out][in] */ LPWSTR pOutBuffer,
    /* [out] */ UINT __RPC_FAR *pchOutBuffer);


void __RPC_STUB IInputSequenceChecker_CheckAndReplaceInputSequence_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInputSequenceChecker_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ISCSa_0209 */
/* [local] */ 

typedef struct  tagISCDATA
    {
    LCID lcidChecker;
    IInputSequenceChecker __RPC_FAR *pISC;
    }	ISCDATA;



extern RPC_IF_HANDLE __MIDL_itf_ISCSa_0209_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ISCSa_0209_v0_0_s_ifspec;

#ifndef __IEnumInputSequenceCheckers_INTERFACE_DEFINED__
#define __IEnumInputSequenceCheckers_INTERFACE_DEFINED__

/* interface IEnumInputSequenceCheckers */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumInputSequenceCheckers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6FA9A2A8-437A-11d2-9712-00C04F79E98B")
    IEnumInputSequenceCheckers : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cISCs,
            /* [length_is][size_is][out] */ ISCDATA __RPC_FAR *pISCData,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cICSs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumInputSequenceCheckersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumInputSequenceCheckers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumInputSequenceCheckers __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumInputSequenceCheckers __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumInputSequenceCheckers __RPC_FAR * This,
            /* [in] */ ULONG cISCs,
            /* [length_is][size_is][out] */ ISCDATA __RPC_FAR *pISCData,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumInputSequenceCheckers __RPC_FAR * This,
            /* [in] */ ULONG cICSs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumInputSequenceCheckers __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumInputSequenceCheckers __RPC_FAR * This,
            /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumInputSequenceCheckersVtbl;

    interface IEnumInputSequenceCheckers
    {
        CONST_VTBL struct IEnumInputSequenceCheckersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumInputSequenceCheckers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumInputSequenceCheckers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumInputSequenceCheckers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumInputSequenceCheckers_Next(This,cISCs,pISCData,pcFetched)	\
    (This)->lpVtbl -> Next(This,cISCs,pISCData,pcFetched)

#define IEnumInputSequenceCheckers_Skip(This,cICSs)	\
    (This)->lpVtbl -> Skip(This,cICSs)

#define IEnumInputSequenceCheckers_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumInputSequenceCheckers_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumInputSequenceCheckers_Next_Proxy( 
    IEnumInputSequenceCheckers __RPC_FAR * This,
    /* [in] */ ULONG cISCs,
    /* [length_is][size_is][out] */ ISCDATA __RPC_FAR *pISCData,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumInputSequenceCheckers_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumInputSequenceCheckers_Skip_Proxy( 
    IEnumInputSequenceCheckers __RPC_FAR * This,
    /* [in] */ ULONG cICSs);


void __RPC_STUB IEnumInputSequenceCheckers_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumInputSequenceCheckers_Reset_Proxy( 
    IEnumInputSequenceCheckers __RPC_FAR * This);


void __RPC_STUB IEnumInputSequenceCheckers_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnumInputSequenceCheckers_Clone_Proxy( 
    IEnumInputSequenceCheckers __RPC_FAR * This,
    /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumInputSequenceCheckers_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumInputSequenceCheckers_INTERFACE_DEFINED__ */


#ifndef __IInputSequenceCheckerContainer_INTERFACE_DEFINED__
#define __IInputSequenceCheckerContainer_INTERFACE_DEFINED__

/* interface IInputSequenceCheckerContainer */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IInputSequenceCheckerContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02D887FA-4358-11D2-BE22-080009DC0A8D")
    IInputSequenceCheckerContainer : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnumISCs( 
            /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInputSequenceCheckerContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IInputSequenceCheckerContainer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IInputSequenceCheckerContainer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IInputSequenceCheckerContainer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumISCs )( 
            IInputSequenceCheckerContainer __RPC_FAR * This,
            /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IInputSequenceCheckerContainerVtbl;

    interface IInputSequenceCheckerContainer
    {
        CONST_VTBL struct IInputSequenceCheckerContainerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInputSequenceCheckerContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInputSequenceCheckerContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInputSequenceCheckerContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInputSequenceCheckerContainer_EnumISCs(This,ppEnum)	\
    (This)->lpVtbl -> EnumISCs(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IInputSequenceCheckerContainer_EnumISCs_Proxy( 
    IInputSequenceCheckerContainer __RPC_FAR * This,
    /* [out] */ IEnumInputSequenceCheckers __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IInputSequenceCheckerContainer_EnumISCs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInputSequenceCheckerContainer_INTERFACE_DEFINED__ */



#ifndef __ISCSALib_LIBRARY_DEFINED__
#define __ISCSALib_LIBRARY_DEFINED__

/* library ISCSALib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ISCSALib;

EXTERN_C const CLSID CLSID_ISCThai;

#ifdef __cplusplus

class DECLSPEC_UUID("6CF60DE1-42DC-11D2-BE22-080009DC0A8D")
ISCThai;
#endif

EXTERN_C const CLSID CLSID_InputSequenceCheckerContainer;

#ifdef __cplusplus

class DECLSPEC_UUID("02D887FB-4358-11D2-BE22-080009DC0A8D")
InputSequenceCheckerContainer;
#endif

EXTERN_C const CLSID CLSID_EnumInputSequenceCheckers;

#ifdef __cplusplus

class DECLSPEC_UUID("BCB80276-4807-11d2-9717-00C04F79E98B")
EnumInputSequenceCheckers;
#endif

EXTERN_C const CLSID CLSID_ISCHindi;

#ifdef __cplusplus

class DECLSPEC_UUID("0666DB29-4823-11d2-9717-00C04F79E98B")
ISCHindi;
#endif

EXTERN_C const CLSID CLSID_ISCVietnamese;

#ifdef __cplusplus

class DECLSPEC_UUID("75624FA1-4826-11d2-9717-00C04F79E98B")
ISCVietnamese;
#endif
#endif /* __ISCSALib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
