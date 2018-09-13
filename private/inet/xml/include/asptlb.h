/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Wed Dec 09 12:35:12 1998
 */
/* Compiler settings for asp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef ___asptlb_h__
#define ___asptlb_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IStringList_FWD_DEFINED__
#define __IStringList_FWD_DEFINED__
typedef interface IStringList IStringList;
#endif 	/* __IStringList_FWD_DEFINED__ */


#ifndef __IRequestDictionary_FWD_DEFINED__
#define __IRequestDictionary_FWD_DEFINED__
typedef interface IRequestDictionary IRequestDictionary;
#endif 	/* __IRequestDictionary_FWD_DEFINED__ */


#ifndef __IRequest_FWD_DEFINED__
#define __IRequest_FWD_DEFINED__
typedef interface IRequest IRequest;
#endif 	/* __IRequest_FWD_DEFINED__ */


#ifndef __Request_FWD_DEFINED__
#define __Request_FWD_DEFINED__

#ifdef __cplusplus
typedef class Request Request;
#else
typedef struct Request Request;
#endif /* __cplusplus */

#endif 	/* __Request_FWD_DEFINED__ */


#ifndef __IReadCookie_FWD_DEFINED__
#define __IReadCookie_FWD_DEFINED__
typedef interface IReadCookie IReadCookie;
#endif 	/* __IReadCookie_FWD_DEFINED__ */


#ifndef __IWriteCookie_FWD_DEFINED__
#define __IWriteCookie_FWD_DEFINED__
typedef interface IWriteCookie IWriteCookie;
#endif 	/* __IWriteCookie_FWD_DEFINED__ */


#ifndef __IResponse_FWD_DEFINED__
#define __IResponse_FWD_DEFINED__
typedef interface IResponse IResponse;
#endif 	/* __IResponse_FWD_DEFINED__ */


#ifndef __Response_FWD_DEFINED__
#define __Response_FWD_DEFINED__

#ifdef __cplusplus
typedef class Response Response;
#else
typedef struct Response Response;
#endif /* __cplusplus */

#endif 	/* __Response_FWD_DEFINED__ */


#ifndef __IVariantDictionary_FWD_DEFINED__
#define __IVariantDictionary_FWD_DEFINED__
typedef interface IVariantDictionary IVariantDictionary;
#endif 	/* __IVariantDictionary_FWD_DEFINED__ */


#ifndef __ISessionObject_FWD_DEFINED__
#define __ISessionObject_FWD_DEFINED__
typedef interface ISessionObject ISessionObject;
#endif 	/* __ISessionObject_FWD_DEFINED__ */


#ifndef __Session_FWD_DEFINED__
#define __Session_FWD_DEFINED__

#ifdef __cplusplus
typedef class Session Session;
#else
typedef struct Session Session;
#endif /* __cplusplus */

#endif 	/* __Session_FWD_DEFINED__ */


#ifndef __IApplicationObject_FWD_DEFINED__
#define __IApplicationObject_FWD_DEFINED__
typedef interface IApplicationObject IApplicationObject;
#endif 	/* __IApplicationObject_FWD_DEFINED__ */


#ifndef __Application_FWD_DEFINED__
#define __Application_FWD_DEFINED__

#ifdef __cplusplus
typedef class Application Application;
#else
typedef struct Application Application;
#endif /* __cplusplus */

#endif 	/* __Application_FWD_DEFINED__ */


#ifndef __IServer_FWD_DEFINED__
#define __IServer_FWD_DEFINED__
typedef interface IServer IServer;
#endif 	/* __IServer_FWD_DEFINED__ */


#ifndef __Server_FWD_DEFINED__
#define __Server_FWD_DEFINED__

#ifdef __cplusplus
typedef class Server Server;
#else
typedef struct Server Server;
#endif /* __cplusplus */

#endif 	/* __Server_FWD_DEFINED__ */


#ifndef __IScriptingContext_FWD_DEFINED__
#define __IScriptingContext_FWD_DEFINED__
typedef interface IScriptingContext IScriptingContext;
#endif 	/* __IScriptingContext_FWD_DEFINED__ */


#ifndef __ScriptingContext_FWD_DEFINED__
#define __ScriptingContext_FWD_DEFINED__

#ifdef __cplusplus
typedef class ScriptingContext ScriptingContext;
#else
typedef struct ScriptingContext ScriptingContext;
#endif /* __cplusplus */

#endif 	/* __ScriptingContext_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ASPTypeLibrary_LIBRARY_DEFINED__
#define __ASPTypeLibrary_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ASPTypeLibrary
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 



DEFINE_GUID(LIBID_ASPTypeLibrary,0xD97A6DA0,0xA85C,0x11cf,0x83,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#ifndef __IStringList_INTERFACE_DEFINED__
#define __IStringList_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IStringList
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IStringList,0xD97A6DA0,0xA85D,0x11cf,0x83,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A85D-11cf-83AE-00A0C90C2BD8")
    IStringList : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in][optional] */ VARIANT i,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *cStrRet) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStringListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IStringList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IStringList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IStringList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IStringList __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IStringList __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IStringList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IStringList __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IStringList __RPC_FAR * This,
            /* [in][optional] */ VARIANT i,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IStringList __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *cStrRet);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IStringList __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IStringListVtbl;

    interface IStringList
    {
        CONST_VTBL struct IStringListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStringList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStringList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStringList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStringList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IStringList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IStringList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IStringList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IStringList_get_Item(This,i,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,i,pVariantReturn)

#define IStringList_get_Count(This,cStrRet)	\
    (This)->lpVtbl -> get_Count(This,cStrRet)

#define IStringList_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IStringList_get_Item_Proxy( 
    IStringList __RPC_FAR * This,
    /* [in][optional] */ VARIANT i,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IStringList_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IStringList_get_Count_Proxy( 
    IStringList __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *cStrRet);


void __RPC_STUB IStringList_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IStringList_get__NewEnum_Proxy( 
    IStringList __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IStringList_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStringList_INTERFACE_DEFINED__ */


#ifndef __IRequestDictionary_INTERFACE_DEFINED__
#define __IRequestDictionary_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRequestDictionary
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IRequestDictionary,0xD97A6DA0,0xA85F,0x11df,0x83,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A85F-11df-83AE-00A0C90C2BD8")
    IRequestDictionary : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in][optional] */ VARIANT Var,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *cStrRet) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Key( 
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRequestDictionaryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRequestDictionary __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRequestDictionary __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRequestDictionary __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in][optional] */ VARIANT Var,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IRequestDictionary __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IRequestDictionary __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *cStrRet);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Key )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        END_INTERFACE
    } IRequestDictionaryVtbl;

    interface IRequestDictionary
    {
        CONST_VTBL struct IRequestDictionaryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRequestDictionary_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRequestDictionary_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRequestDictionary_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRequestDictionary_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRequestDictionary_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRequestDictionary_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRequestDictionary_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRequestDictionary_get_Item(This,Var,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,Var,pVariantReturn)

#define IRequestDictionary_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#define IRequestDictionary_get_Count(This,cStrRet)	\
    (This)->lpVtbl -> get_Count(This,cStrRet)

#define IRequestDictionary_get_Key(This,VarKey,pvar)	\
    (This)->lpVtbl -> get_Key(This,VarKey,pvar)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRequestDictionary_get_Item_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [in][optional] */ VARIANT Var,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IRequestDictionary_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IRequestDictionary_get__NewEnum_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IRequestDictionary_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequestDictionary_get_Count_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *cStrRet);


void __RPC_STUB IRequestDictionary_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IRequestDictionary_get_Key_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IRequestDictionary_get_Key_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRequestDictionary_INTERFACE_DEFINED__ */


#ifndef __IRequest_INTERFACE_DEFINED__
#define __IRequest_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRequest
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][uuid] */ 



DEFINE_GUID(IID_IRequest,0xD97A6DA0,0xA861,0x11cf,0x93,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A861-11cf-93AE-00A0C90C2BD8")
    IRequest : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR bstrVar,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_QueryString( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Form( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Body( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ServerVariables( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientCertificate( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Cookies( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TotalBytes( 
            /* [retval][out] */ long __RPC_FAR *pcbTotal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BinaryRead( 
            /* [out][in] */ VARIANT __RPC_FAR *pvarCountToRead,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRequest __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRequest __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IRequest __RPC_FAR * This,
            /* [in] */ BSTR bstrVar,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QueryString )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Form )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [hidden][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Body )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServerVariables )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClientCertificate )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Cookies )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TotalBytes )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pcbTotal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BinaryRead )( 
            IRequest __RPC_FAR * This,
            /* [out][in] */ VARIANT __RPC_FAR *pvarCountToRead,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarReturn);
        
        END_INTERFACE
    } IRequestVtbl;

    interface IRequest
    {
        CONST_VTBL struct IRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRequest_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRequest_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRequest_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRequest_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRequest_get_Item(This,bstrVar,ppObjReturn)	\
    (This)->lpVtbl -> get_Item(This,bstrVar,ppObjReturn)

#define IRequest_get_QueryString(This,ppDictReturn)	\
    (This)->lpVtbl -> get_QueryString(This,ppDictReturn)

#define IRequest_get_Form(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Form(This,ppDictReturn)

#define IRequest_get_Body(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Body(This,ppDictReturn)

#define IRequest_get_ServerVariables(This,ppDictReturn)	\
    (This)->lpVtbl -> get_ServerVariables(This,ppDictReturn)

#define IRequest_get_ClientCertificate(This,ppDictReturn)	\
    (This)->lpVtbl -> get_ClientCertificate(This,ppDictReturn)

#define IRequest_get_Cookies(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Cookies(This,ppDictReturn)

#define IRequest_get_TotalBytes(This,pcbTotal)	\
    (This)->lpVtbl -> get_TotalBytes(This,pcbTotal)

#define IRequest_BinaryRead(This,pvarCountToRead,pvarReturn)	\
    (This)->lpVtbl -> BinaryRead(This,pvarCountToRead,pvarReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_Item_Proxy( 
    IRequest __RPC_FAR * This,
    /* [in] */ BSTR bstrVar,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn);


void __RPC_STUB IRequest_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_QueryString_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_Form_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Form_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_Body_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Body_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_ServerVariables_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_ServerVariables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_ClientCertificate_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_ClientCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_Cookies_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Cookies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRequest_get_TotalBytes_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pcbTotal);


void __RPC_STUB IRequest_get_TotalBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRequest_BinaryRead_Proxy( 
    IRequest __RPC_FAR * This,
    /* [out][in] */ VARIANT __RPC_FAR *pvarCountToRead,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarReturn);


void __RPC_STUB IRequest_BinaryRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRequest_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_Request,0x920c25d0,0x25d9,0x11d0,0xa5,0x5f,0x00,0xa0,0xc9,0x0c,0x20,0x91);

#ifdef __cplusplus

class DECLSPEC_UUID("920c25d0-25d9-11d0-a55f-00a0c90c2091")
Request;
#endif

#ifndef __IReadCookie_INTERFACE_DEFINED__
#define __IReadCookie_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IReadCookie
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IReadCookie,0x71EAF260,0x0CE0,0x11D0,0xA5,0x3E,0x00,0xA0,0xC9,0x0C,0x20,0x91);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("71EAF260-0CE0-11D0-A53E-00A0C90C2091")
    IReadCookie : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in][optional] */ VARIANT Var,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HasKeys( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *cStrRet) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Key( 
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReadCookieVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IReadCookie __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IReadCookie __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IReadCookie __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IReadCookie __RPC_FAR * This,
            /* [in][optional] */ VARIANT Var,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HasKeys )( 
            IReadCookie __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IReadCookie __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IReadCookie __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *cStrRet);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Key )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        END_INTERFACE
    } IReadCookieVtbl;

    interface IReadCookie
    {
        CONST_VTBL struct IReadCookieVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReadCookie_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReadCookie_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReadCookie_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReadCookie_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IReadCookie_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IReadCookie_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IReadCookie_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IReadCookie_get_Item(This,Var,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,Var,pVariantReturn)

#define IReadCookie_get_HasKeys(This,pfHasKeys)	\
    (This)->lpVtbl -> get_HasKeys(This,pfHasKeys)

#define IReadCookie_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#define IReadCookie_get_Count(This,cStrRet)	\
    (This)->lpVtbl -> get_Count(This,cStrRet)

#define IReadCookie_get_Key(This,VarKey,pvar)	\
    (This)->lpVtbl -> get_Key(This,VarKey,pvar)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IReadCookie_get_Item_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [in][optional] */ VARIANT Var,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IReadCookie_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IReadCookie_get_HasKeys_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys);


void __RPC_STUB IReadCookie_get_HasKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IReadCookie_get__NewEnum_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IReadCookie_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IReadCookie_get_Count_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *cStrRet);


void __RPC_STUB IReadCookie_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IReadCookie_get_Key_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IReadCookie_get_Key_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReadCookie_INTERFACE_DEFINED__ */


#ifndef __IWriteCookie_INTERFACE_DEFINED__
#define __IWriteCookie_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWriteCookie
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IWriteCookie,0xD97A6DA0,0xA862,0x11cf,0x84,0xAE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A862-11cf-84AE-00A0C90C2BD8")
    IWriteCookie : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Item( 
            /* [in][optional] */ VARIANT key,
            /* [in] */ BSTR bstrValue) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Expires( 
            /* [in] */ DATE dtExpires) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Domain( 
            /* [in] */ BSTR bstrDomain) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR bstrPath) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Secure( 
            /* [in] */ VARIANT_BOOL fSecure) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HasKeys( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWriteCookieVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWriteCookie __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWriteCookie __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWriteCookie __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Item )( 
            IWriteCookie __RPC_FAR * This,
            /* [in][optional] */ VARIANT key,
            /* [in] */ BSTR bstrValue);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Expires )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ DATE dtExpires);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Domain )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ BSTR bstrDomain);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Path )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ BSTR bstrPath);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Secure )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fSecure);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HasKeys )( 
            IWriteCookie __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IWriteCookie __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IWriteCookieVtbl;

    interface IWriteCookie
    {
        CONST_VTBL struct IWriteCookieVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWriteCookie_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWriteCookie_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWriteCookie_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWriteCookie_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWriteCookie_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWriteCookie_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWriteCookie_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWriteCookie_put_Item(This,key,bstrValue)	\
    (This)->lpVtbl -> put_Item(This,key,bstrValue)

#define IWriteCookie_put_Expires(This,dtExpires)	\
    (This)->lpVtbl -> put_Expires(This,dtExpires)

#define IWriteCookie_put_Domain(This,bstrDomain)	\
    (This)->lpVtbl -> put_Domain(This,bstrDomain)

#define IWriteCookie_put_Path(This,bstrPath)	\
    (This)->lpVtbl -> put_Path(This,bstrPath)

#define IWriteCookie_put_Secure(This,fSecure)	\
    (This)->lpVtbl -> put_Secure(This,fSecure)

#define IWriteCookie_get_HasKeys(This,pfHasKeys)	\
    (This)->lpVtbl -> get_HasKeys(This,pfHasKeys)

#define IWriteCookie_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IWriteCookie_put_Item_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in][optional] */ VARIANT key,
    /* [in] */ BSTR bstrValue);


void __RPC_STUB IWriteCookie_put_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IWriteCookie_put_Expires_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ DATE dtExpires);


void __RPC_STUB IWriteCookie_put_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IWriteCookie_put_Domain_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ BSTR bstrDomain);


void __RPC_STUB IWriteCookie_put_Domain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IWriteCookie_put_Path_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ BSTR bstrPath);


void __RPC_STUB IWriteCookie_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IWriteCookie_put_Secure_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fSecure);


void __RPC_STUB IWriteCookie_put_Secure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IWriteCookie_get_HasKeys_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHasKeys);


void __RPC_STUB IWriteCookie_get_HasKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IWriteCookie_get__NewEnum_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IWriteCookie_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWriteCookie_INTERFACE_DEFINED__ */


#ifndef __IResponse_INTERFACE_DEFINED__
#define __IResponse_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResponse
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][uuid] */ 



DEFINE_GUID(IID_IResponse,0xD97A6DA0,0xA864,0x11cf,0x83,0xBE,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A864-11cf-83BE-00A0C90C2BD8")
    IResponse : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Buffer( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fIsBuffering) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Buffer( 
            /* [in] */ VARIANT_BOOL fIsBuffering) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ContentType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ContentType( 
            /* [in] */ BSTR bstrContentType) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Expires( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Expires( 
            /* [in] */ long lExpiresMinutes) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExpiresAbsolute( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ExpiresAbsolute( 
            /* [in] */ DATE dtExpires) = 0;
        
        virtual /* [propget][helpstring] */ HRESULT STDMETHODCALLTYPE get_Cookies( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Status( 
            /* [in] */ BSTR bstrStatus) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrHeaderValue,
            /* [in] */ BSTR bstrHeaderName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddHeader( 
            /* [in] */ BSTR bstrHeaderName,
            /* [in] */ BSTR bstrHeaderValue) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AppendToLog( 
            /* [in] */ BSTR bstrLogEntry) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BinaryWrite( 
            /* [in] */ VARIANT varInput) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE End( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Redirect( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varText) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE WriteBlock( 
            /* [in] */ short iBlockNumber) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsClientConnected( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIsClientConnected) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CharSet( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCharSetRet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CharSet( 
            /* [in] */ BSTR bstrCharSet) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Pics( 
            /* [in] */ BSTR bstrHeaderValue) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CacheControl( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCacheControl) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CacheControl( 
            /* [in] */ BSTR bstrCacheControl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResponseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResponse __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResponse __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResponse __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IResponse __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IResponse __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IResponse __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IResponse __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Buffer )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fIsBuffering);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Buffer )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fIsBuffering);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContentType )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ContentType )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrContentType);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Expires )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Expires )( 
            IResponse __RPC_FAR * This,
            /* [in] */ long lExpiresMinutes);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExpiresAbsolute )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExpiresAbsolute )( 
            IResponse __RPC_FAR * This,
            /* [in] */ DATE dtExpires);
        
        /* [propget][helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Cookies )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Status )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Status )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrStatus);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrHeaderValue,
            /* [in] */ BSTR bstrHeaderName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddHeader )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrHeaderName,
            /* [in] */ BSTR bstrHeaderValue);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendToLog )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrLogEntry);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BinaryWrite )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT varInput);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Flush )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Redirect )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrURL);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT varText);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteBlock )( 
            IResponse __RPC_FAR * This,
            /* [in] */ short iBlockNumber);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsClientConnected )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIsClientConnected);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CharSet )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCharSetRet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CharSet )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrCharSet);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pics )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrHeaderValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CacheControl )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCacheControl);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CacheControl )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrCacheControl);
        
        END_INTERFACE
    } IResponseVtbl;

    interface IResponse
    {
        CONST_VTBL struct IResponseVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResponse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResponse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResponse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResponse_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IResponse_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IResponse_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IResponse_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IResponse_get_Buffer(This,fIsBuffering)	\
    (This)->lpVtbl -> get_Buffer(This,fIsBuffering)

#define IResponse_put_Buffer(This,fIsBuffering)	\
    (This)->lpVtbl -> put_Buffer(This,fIsBuffering)

#define IResponse_get_ContentType(This,pbstrContentTypeRet)	\
    (This)->lpVtbl -> get_ContentType(This,pbstrContentTypeRet)

#define IResponse_put_ContentType(This,bstrContentType)	\
    (This)->lpVtbl -> put_ContentType(This,bstrContentType)

#define IResponse_get_Expires(This,pvarExpiresMinutesRet)	\
    (This)->lpVtbl -> get_Expires(This,pvarExpiresMinutesRet)

#define IResponse_put_Expires(This,lExpiresMinutes)	\
    (This)->lpVtbl -> put_Expires(This,lExpiresMinutes)

#define IResponse_get_ExpiresAbsolute(This,pvarExpiresRet)	\
    (This)->lpVtbl -> get_ExpiresAbsolute(This,pvarExpiresRet)

#define IResponse_put_ExpiresAbsolute(This,dtExpires)	\
    (This)->lpVtbl -> put_ExpiresAbsolute(This,dtExpires)

#define IResponse_get_Cookies(This,ppCookies)	\
    (This)->lpVtbl -> get_Cookies(This,ppCookies)

#define IResponse_get_Status(This,pbstrStatusRet)	\
    (This)->lpVtbl -> get_Status(This,pbstrStatusRet)

#define IResponse_put_Status(This,bstrStatus)	\
    (This)->lpVtbl -> put_Status(This,bstrStatus)

#define IResponse_Add(This,bstrHeaderValue,bstrHeaderName)	\
    (This)->lpVtbl -> Add(This,bstrHeaderValue,bstrHeaderName)

#define IResponse_AddHeader(This,bstrHeaderName,bstrHeaderValue)	\
    (This)->lpVtbl -> AddHeader(This,bstrHeaderName,bstrHeaderValue)

#define IResponse_AppendToLog(This,bstrLogEntry)	\
    (This)->lpVtbl -> AppendToLog(This,bstrLogEntry)

#define IResponse_BinaryWrite(This,varInput)	\
    (This)->lpVtbl -> BinaryWrite(This,varInput)

#define IResponse_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IResponse_End(This)	\
    (This)->lpVtbl -> End(This)

#define IResponse_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#define IResponse_Redirect(This,bstrURL)	\
    (This)->lpVtbl -> Redirect(This,bstrURL)

#define IResponse_Write(This,varText)	\
    (This)->lpVtbl -> Write(This,varText)

#define IResponse_WriteBlock(This,iBlockNumber)	\
    (This)->lpVtbl -> WriteBlock(This,iBlockNumber)

#define IResponse_IsClientConnected(This,pfIsClientConnected)	\
    (This)->lpVtbl -> IsClientConnected(This,pfIsClientConnected)

#define IResponse_get_CharSet(This,pbstrCharSetRet)	\
    (This)->lpVtbl -> get_CharSet(This,pbstrCharSetRet)

#define IResponse_put_CharSet(This,bstrCharSet)	\
    (This)->lpVtbl -> put_CharSet(This,bstrCharSet)

#define IResponse_Pics(This,bstrHeaderValue)	\
    (This)->lpVtbl -> Pics(This,bstrHeaderValue)

#define IResponse_get_CacheControl(This,pbstrCacheControl)	\
    (This)->lpVtbl -> get_CacheControl(This,pbstrCacheControl)

#define IResponse_put_CacheControl(This,bstrCacheControl)	\
    (This)->lpVtbl -> put_CacheControl(This,bstrCacheControl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_Buffer_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fIsBuffering);


void __RPC_STUB IResponse_get_Buffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_Buffer_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fIsBuffering);


void __RPC_STUB IResponse_put_Buffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_ContentType_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet);


void __RPC_STUB IResponse_get_ContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_ContentType_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrContentType);


void __RPC_STUB IResponse_put_ContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_Expires_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet);


void __RPC_STUB IResponse_get_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_Expires_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ long lExpiresMinutes);


void __RPC_STUB IResponse_put_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_ExpiresAbsolute_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet);


void __RPC_STUB IResponse_get_ExpiresAbsolute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_ExpiresAbsolute_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ DATE dtExpires);


void __RPC_STUB IResponse_put_ExpiresAbsolute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_get_Cookies_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies);


void __RPC_STUB IResponse_get_Cookies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_Status_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet);


void __RPC_STUB IResponse_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_Status_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrStatus);


void __RPC_STUB IResponse_put_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden] */ HRESULT STDMETHODCALLTYPE IResponse_Add_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrHeaderValue,
    /* [in] */ BSTR bstrHeaderName);


void __RPC_STUB IResponse_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_AddHeader_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrHeaderName,
    /* [in] */ BSTR bstrHeaderValue);


void __RPC_STUB IResponse_AddHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_AppendToLog_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrLogEntry);


void __RPC_STUB IResponse_AppendToLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_BinaryWrite_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT varInput);


void __RPC_STUB IResponse_BinaryWrite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_Clear_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_End_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_Flush_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_Redirect_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB IResponse_Redirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_Write_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT varText);


void __RPC_STUB IResponse_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden] */ HRESULT STDMETHODCALLTYPE IResponse_WriteBlock_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ short iBlockNumber);


void __RPC_STUB IResponse_WriteBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_IsClientConnected_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIsClientConnected);


void __RPC_STUB IResponse_IsClientConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_CharSet_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrCharSetRet);


void __RPC_STUB IResponse_get_CharSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_CharSet_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrCharSet);


void __RPC_STUB IResponse_put_CharSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IResponse_Pics_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrHeaderValue);


void __RPC_STUB IResponse_Pics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IResponse_get_CacheControl_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrCacheControl);


void __RPC_STUB IResponse_get_CacheControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IResponse_put_CacheControl_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrCacheControl);


void __RPC_STUB IResponse_put_CacheControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResponse_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_Response,0x46E19BA0,0x25DD,0x11D0,0xA5,0x5F,0x00,0xA0,0xC9,0x0C,0x20,0x91);

#ifdef __cplusplus

class DECLSPEC_UUID("46E19BA0-25DD-11D0-A55F-00A0C90C2091")
Response;
#endif

#ifndef __IVariantDictionary_INTERFACE_DEFINED__
#define __IVariantDictionary_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVariantDictionary
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IVariantDictionary,0x4a7deb90,0xb069,0x11d0,0xb3,0x73,0x00,0xa0,0xc9,0x0c,0x2b,0xd8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4a7deb90-b069-11d0-b373-00a0c90c2bd8")
    IVariantDictionary : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Item( 
            /* [in] */ VARIANT VarKey,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Item( 
            /* [in] */ VARIANT VarKey,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Key( 
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *cStrRet) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVariantDictionaryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVariantDictionary __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVariantDictionary __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IVariantDictionary __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Item )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [in] */ VARIANT var);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Item )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [in] */ VARIANT var);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Key )( 
            IVariantDictionary __RPC_FAR * This,
            /* [in] */ VARIANT VarKey,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IVariantDictionary __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *cStrRet);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IVariantDictionary __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IVariantDictionaryVtbl;

    interface IVariantDictionary
    {
        CONST_VTBL struct IVariantDictionaryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVariantDictionary_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVariantDictionary_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVariantDictionary_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVariantDictionary_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVariantDictionary_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVariantDictionary_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVariantDictionary_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVariantDictionary_get_Item(This,VarKey,pvar)	\
    (This)->lpVtbl -> get_Item(This,VarKey,pvar)

#define IVariantDictionary_put_Item(This,VarKey,var)	\
    (This)->lpVtbl -> put_Item(This,VarKey,var)

#define IVariantDictionary_putref_Item(This,VarKey,var)	\
    (This)->lpVtbl -> putref_Item(This,VarKey,var)

#define IVariantDictionary_get_Key(This,VarKey,pvar)	\
    (This)->lpVtbl -> get_Key(This,VarKey,pvar)

#define IVariantDictionary_get_Count(This,cStrRet)	\
    (This)->lpVtbl -> get_Count(This,cStrRet)

#define IVariantDictionary_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_get_Item_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IVariantDictionary_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_put_Item_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [in] */ VARIANT var);


void __RPC_STUB IVariantDictionary_put_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_putref_Item_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [in] */ VARIANT var);


void __RPC_STUB IVariantDictionary_putref_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_get_Key_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [in] */ VARIANT VarKey,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IVariantDictionary_get_Key_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_get_Count_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *cStrRet);


void __RPC_STUB IVariantDictionary_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE IVariantDictionary_get__NewEnum_Proxy( 
    IVariantDictionary __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IVariantDictionary_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVariantDictionary_INTERFACE_DEFINED__ */


#ifndef __ISessionObject_INTERFACE_DEFINED__
#define __ISessionObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISessionObject
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][oleautomation][dual][uuid] */ 



DEFINE_GUID(IID_ISessionObject,0xD97A6DA0,0xA865,0x11cf,0x83,0xAF,0x00,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A865-11cf-83AF-00A0C90C2BD8")
    ISessionObject : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SessionID( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Timeout( 
            /* [retval][out] */ long __RPC_FAR *plvar) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Timeout( 
            /* [in] */ long lvar) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Abandon( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CodePage( 
            /* [retval][out] */ long __RPC_FAR *plvar) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CodePage( 
            /* [in] */ long lvar) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LCID( 
            /* [retval][out] */ long __RPC_FAR *plvar) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LCID( 
            /* [in] */ long lvar) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StaticObjects( 
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppTaggedObjects) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Contents( 
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISessionObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISessionObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISessionObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISessionObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SessionID )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Timeout )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plvar);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Timeout )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ long lvar);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abandon )( 
            ISessionObject __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CodePage )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plvar);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CodePage )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ long lvar);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LCID )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plvar);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LCID )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ long lvar);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StaticObjects )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppTaggedObjects);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Contents )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } ISessionObjectVtbl;

    interface ISessionObject
    {
        CONST_VTBL struct ISessionObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISessionObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISessionObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISessionObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISessionObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISessionObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISessionObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISessionObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISessionObject_get_SessionID(This,pbstrRet)	\
    (This)->lpVtbl -> get_SessionID(This,pbstrRet)

#define ISessionObject_get_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> get_Value(This,bstrValue,pvar)

#define ISessionObject_put_Value(This,bstrValue,var)	\
    (This)->lpVtbl -> put_Value(This,bstrValue,var)

#define ISessionObject_putref_Value(This,bstrValue,var)	\
    (This)->lpVtbl -> putref_Value(This,bstrValue,var)

#define ISessionObject_get_Timeout(This,plvar)	\
    (This)->lpVtbl -> get_Timeout(This,plvar)

#define ISessionObject_put_Timeout(This,lvar)	\
    (This)->lpVtbl -> put_Timeout(This,lvar)

#define ISessionObject_Abandon(This)	\
    (This)->lpVtbl -> Abandon(This)

#define ISessionObject_get_CodePage(This,plvar)	\
    (This)->lpVtbl -> get_CodePage(This,plvar)

#define ISessionObject_put_CodePage(This,lvar)	\
    (This)->lpVtbl -> put_CodePage(This,lvar)

#define ISessionObject_get_LCID(This,plvar)	\
    (This)->lpVtbl -> get_LCID(This,plvar)

#define ISessionObject_put_LCID(This,lvar)	\
    (This)->lpVtbl -> put_LCID(This,lvar)

#define ISessionObject_get_StaticObjects(This,ppTaggedObjects)	\
    (This)->lpVtbl -> get_StaticObjects(This,ppTaggedObjects)

#define ISessionObject_get_Contents(This,ppProperties)	\
    (This)->lpVtbl -> get_Contents(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_SessionID_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);


void __RPC_STUB ISessionObject_get_SessionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB ISessionObject_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISessionObject_put_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT var);


void __RPC_STUB ISessionObject_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT STDMETHODCALLTYPE ISessionObject_putref_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT var);


void __RPC_STUB ISessionObject_putref_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_Timeout_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plvar);


void __RPC_STUB ISessionObject_get_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ISessionObject_put_Timeout_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ long lvar);


void __RPC_STUB ISessionObject_put_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISessionObject_Abandon_Proxy( 
    ISessionObject __RPC_FAR * This);


void __RPC_STUB ISessionObject_Abandon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_CodePage_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plvar);


void __RPC_STUB ISessionObject_get_CodePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ISessionObject_put_CodePage_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ long lvar);


void __RPC_STUB ISessionObject_put_CodePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_LCID_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plvar);


void __RPC_STUB ISessionObject_get_LCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ISessionObject_put_LCID_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ long lvar);


void __RPC_STUB ISessionObject_put_LCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_StaticObjects_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppTaggedObjects);


void __RPC_STUB ISessionObject_get_StaticObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ISessionObject_get_Contents_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ISessionObject_get_Contents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISessionObject_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_Session,0x509F8F20,0x25DE,0x11D0,0xA5,0x5F,0x00,0xA0,0xC9,0x0C,0x20,0x91);

#ifdef __cplusplus

class DECLSPEC_UUID("509F8F20-25DE-11D0-A55F-00A0C90C2091")
Session;
#endif

#ifndef __IApplicationObject_INTERFACE_DEFINED__
#define __IApplicationObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IApplicationObject
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][uuid] */ 



DEFINE_GUID(IID_IApplicationObject,0xD97A6DA0,0xA866,0x11cf,0x83,0xAE,0x10,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A866-11cf-83AE-10A0C90C2BD8")
    IApplicationObject : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Lock( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UnLock( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StaticObjects( 
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Contents( 
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IApplicationObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IApplicationObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IApplicationObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT var);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Lock )( 
            IApplicationObject __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnLock )( 
            IApplicationObject __RPC_FAR * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StaticObjects )( 
            IApplicationObject __RPC_FAR * This,
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Contents )( 
            IApplicationObject __RPC_FAR * This,
            /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } IApplicationObjectVtbl;

    interface IApplicationObject
    {
        CONST_VTBL struct IApplicationObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IApplicationObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IApplicationObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IApplicationObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IApplicationObject_get_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> get_Value(This,bstrValue,pvar)

#define IApplicationObject_put_Value(This,bstrValue,var)	\
    (This)->lpVtbl -> put_Value(This,bstrValue,var)

#define IApplicationObject_putref_Value(This,bstrValue,var)	\
    (This)->lpVtbl -> putref_Value(This,bstrValue,var)

#define IApplicationObject_Lock(This)	\
    (This)->lpVtbl -> Lock(This)

#define IApplicationObject_UnLock(This)	\
    (This)->lpVtbl -> UnLock(This)

#define IApplicationObject_get_StaticObjects(This,ppProperties)	\
    (This)->lpVtbl -> get_StaticObjects(This,ppProperties)

#define IApplicationObject_get_Contents(This,ppProperties)	\
    (This)->lpVtbl -> get_Contents(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IApplicationObject_get_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IApplicationObject_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IApplicationObject_put_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT var);


void __RPC_STUB IApplicationObject_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT STDMETHODCALLTYPE IApplicationObject_putref_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT var);


void __RPC_STUB IApplicationObject_putref_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IApplicationObject_Lock_Proxy( 
    IApplicationObject __RPC_FAR * This);


void __RPC_STUB IApplicationObject_Lock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IApplicationObject_UnLock_Proxy( 
    IApplicationObject __RPC_FAR * This);


void __RPC_STUB IApplicationObject_UnLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IApplicationObject_get_StaticObjects_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB IApplicationObject_get_StaticObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IApplicationObject_get_Contents_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [retval][out] */ IVariantDictionary __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB IApplicationObject_get_Contents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationObject_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_Application,0x7C3BAF00,0x25DE,0x11D0,0xA5,0x5F,0x00,0xA0,0xC9,0x0C,0x20,0x91);

#ifdef __cplusplus

class DECLSPEC_UUID("7C3BAF00-25DE-11D0-A55F-00A0C90C2091")
Application;
#endif

#ifndef __IServer_INTERFACE_DEFINED__
#define __IServer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IServer
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][uuid] */ 



DEFINE_GUID(IID_IServer,0xD97A6DA0,0xA867,0x11cf,0x83,0xAE,0x01,0xA0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A867-11cf-83AE-01A0C90C2BD8")
    IServer : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ScriptTimeout( 
            /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ScriptTimeout( 
            /* [in] */ long lTimeoutSeconds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateObject( 
            /* [in] */ BSTR bstrProgID,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE HTMLEncode( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapPath( 
            /* [in] */ BSTR bstrLogicalPath,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE URLEncode( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE URLPathEncode( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IServer __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IServer __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IServer __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ScriptTimeout )( 
            IServer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ScriptTimeout )( 
            IServer __RPC_FAR * This,
            /* [in] */ long lTimeoutSeconds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateObject )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrProgID,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HTMLEncode )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapPath )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrLogicalPath,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *URLEncode )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *URLPathEncode )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);
        
        END_INTERFACE
    } IServerVtbl;

    interface IServer
    {
        CONST_VTBL struct IServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IServer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IServer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IServer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IServer_get_ScriptTimeout(This,plTimeoutSeconds)	\
    (This)->lpVtbl -> get_ScriptTimeout(This,plTimeoutSeconds)

#define IServer_put_ScriptTimeout(This,lTimeoutSeconds)	\
    (This)->lpVtbl -> put_ScriptTimeout(This,lTimeoutSeconds)

#define IServer_CreateObject(This,bstrProgID,ppDispObject)	\
    (This)->lpVtbl -> CreateObject(This,bstrProgID,ppDispObject)

#define IServer_HTMLEncode(This,bstrIn,pbstrEncoded)	\
    (This)->lpVtbl -> HTMLEncode(This,bstrIn,pbstrEncoded)

#define IServer_MapPath(This,bstrLogicalPath,pbstrPhysicalPath)	\
    (This)->lpVtbl -> MapPath(This,bstrLogicalPath,pbstrPhysicalPath)

#define IServer_URLEncode(This,bstrIn,pbstrEncoded)	\
    (This)->lpVtbl -> URLEncode(This,bstrIn,pbstrEncoded)

#define IServer_URLPathEncode(This,bstrIn,pbstrEncoded)	\
    (This)->lpVtbl -> URLPathEncode(This,bstrIn,pbstrEncoded)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IServer_get_ScriptTimeout_Proxy( 
    IServer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds);


void __RPC_STUB IServer_get_ScriptTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IServer_put_ScriptTimeout_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ long lTimeoutSeconds);


void __RPC_STUB IServer_put_ScriptTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IServer_CreateObject_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrProgID,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject);


void __RPC_STUB IServer_CreateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IServer_HTMLEncode_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);


void __RPC_STUB IServer_HTMLEncode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IServer_MapPath_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrLogicalPath,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath);


void __RPC_STUB IServer_MapPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IServer_URLEncode_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);


void __RPC_STUB IServer_URLEncode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IServer_URLPathEncode_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);


void __RPC_STUB IServer_URLPathEncode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IServer_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_Server,0xA506D160,0x25E0,0x11D0,0xA5,0x5F,0x00,0xA0,0xC9,0x0C,0x20,0x91);

#ifdef __cplusplus

class DECLSPEC_UUID("A506D160-25E0-11D0-A55F-00A0C90C2091")
Server;
#endif

#ifndef __IScriptingContext_INTERFACE_DEFINED__
#define __IScriptingContext_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScriptingContext
 * at Wed Dec 09 12:35:12 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][hidden][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID_IScriptingContext,0xD97A6DA0,0xA868,0x11cf,0x83,0xAE,0x00,0xB0,0xC9,0x0C,0x2B,0xD8);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D97A6DA0-A868-11cf-83AE-00B0C90C2BD8")
    IScriptingContext : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Request( 
            /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Response( 
            /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Server( 
            /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptingContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScriptingContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScriptingContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IScriptingContext __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Request )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Response )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Server )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Session )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication);
        
        END_INTERFACE
    } IScriptingContextVtbl;

    interface IScriptingContext
    {
        CONST_VTBL struct IScriptingContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptingContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptingContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptingContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptingContext_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IScriptingContext_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IScriptingContext_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IScriptingContext_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IScriptingContext_get_Request(This,ppRequest)	\
    (This)->lpVtbl -> get_Request(This,ppRequest)

#define IScriptingContext_get_Response(This,ppResponse)	\
    (This)->lpVtbl -> get_Response(This,ppResponse)

#define IScriptingContext_get_Server(This,ppServer)	\
    (This)->lpVtbl -> get_Server(This,ppServer)

#define IScriptingContext_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define IScriptingContext_get_Application(This,ppApplication)	\
    (This)->lpVtbl -> get_Application(This,ppApplication)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IScriptingContext_get_Request_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest);


void __RPC_STUB IScriptingContext_get_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IScriptingContext_get_Response_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse);


void __RPC_STUB IScriptingContext_get_Response_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IScriptingContext_get_Server_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer);


void __RPC_STUB IScriptingContext_get_Server_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IScriptingContext_get_Session_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession);


void __RPC_STUB IScriptingContext_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IScriptingContext_get_Application_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication);


void __RPC_STUB IScriptingContext_get_Application_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptingContext_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_ScriptingContext,0xD97A6DA0,0xA868,0x11cf,0x83,0xAE,0x11,0xB0,0xC9,0x0C,0x2B,0xD8);

#ifdef __cplusplus

class DECLSPEC_UUID("D97A6DA0-A868-11cf-83AE-11B0C90C2BD8")
ScriptingContext;
#endif
#endif /* __ASPTypeLibrary_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
