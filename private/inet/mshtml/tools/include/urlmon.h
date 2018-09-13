/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Aug 02 15:31:22 1996
 */
/* Compiler settings for urlmon.idl:
    Oic (OptLev=i1), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __urlmon_h__
#define __urlmon_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPersistMoniker_FWD_DEFINED__
#define __IPersistMoniker_FWD_DEFINED__
typedef interface IPersistMoniker IPersistMoniker;
#endif 	/* __IPersistMoniker_FWD_DEFINED__ */


#ifndef __IBindProtocol_FWD_DEFINED__
#define __IBindProtocol_FWD_DEFINED__
typedef interface IBindProtocol IBindProtocol;
#endif 	/* __IBindProtocol_FWD_DEFINED__ */


#ifndef __IBinding_FWD_DEFINED__
#define __IBinding_FWD_DEFINED__
typedef interface IBinding IBinding;
#endif 	/* __IBinding_FWD_DEFINED__ */


#ifndef __IBindStatusCallback_FWD_DEFINED__
#define __IBindStatusCallback_FWD_DEFINED__
typedef interface IBindStatusCallback IBindStatusCallback;
#endif 	/* __IBindStatusCallback_FWD_DEFINED__ */


#ifndef __IAuthenticate_FWD_DEFINED__
#define __IAuthenticate_FWD_DEFINED__
typedef interface IAuthenticate IAuthenticate;
#endif 	/* __IAuthenticate_FWD_DEFINED__ */


#ifndef __IHttpNegotiate_FWD_DEFINED__
#define __IHttpNegotiate_FWD_DEFINED__
typedef interface IHttpNegotiate IHttpNegotiate;
#endif 	/* __IHttpNegotiate_FWD_DEFINED__ */


#ifndef __IWindowForBindingUI_FWD_DEFINED__
#define __IWindowForBindingUI_FWD_DEFINED__
typedef interface IWindowForBindingUI IWindowForBindingUI;
#endif 	/* __IWindowForBindingUI_FWD_DEFINED__ */


#ifndef __ICodeInstall_FWD_DEFINED__
#define __ICodeInstall_FWD_DEFINED__
typedef interface ICodeInstall ICodeInstall;
#endif 	/* __ICodeInstall_FWD_DEFINED__ */


#ifndef __IWinInetInfo_FWD_DEFINED__
#define __IWinInetInfo_FWD_DEFINED__
typedef interface IWinInetInfo IWinInetInfo;
#endif 	/* __IWinInetInfo_FWD_DEFINED__ */


#ifndef __IHttpSecurity_FWD_DEFINED__
#define __IHttpSecurity_FWD_DEFINED__
typedef interface IHttpSecurity IHttpSecurity;
#endif 	/* __IHttpSecurity_FWD_DEFINED__ */


#ifndef __IWinInetHttpInfo_FWD_DEFINED__
#define __IWinInetHttpInfo_FWD_DEFINED__
typedef interface IWinInetHttpInfo IWinInetHttpInfo;
#endif 	/* __IWinInetHttpInfo_FWD_DEFINED__ */


#ifndef __IBindHost_FWD_DEFINED__
#define __IBindHost_FWD_DEFINED__
typedef interface IBindHost IBindHost;
#endif 	/* __IBindHost_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "servprov.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// UrlMon.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// URL Moniker Interfaces.











// These are for backwards compatibility with previous URLMON versions
#define BINDF_DONTUSECACHE BINDF_GETNEWESTVERSION
#define BINDF_DONTPUTINCACHE BINDF_NOWRITECACHE
#define BINDF_NOCOPYDATA BINDF_PULLDATA
EXTERN_C const IID IID_IAsyncMoniker;    
EXTERN_C const IID CLSID_StdURLMoniker;  
EXTERN_C const IID CLSID_HttpProtocol;   
EXTERN_C const IID CLSID_FtpProtocol;    
EXTERN_C const IID CLSID_GopherProtocol; 
EXTERN_C const IID CLSID_HttpSProtocol;  
EXTERN_C const IID CLSID_FileProtocol;   
EXTERN_C const IID CLSID_MkProtocol;     
 
#define SZ_URLCONTEXT           OLESTR("URL Context")
#define SZ_ASYNC_CALLEE         OLESTR("AsyncCallee")
#define MKSYS_URLMONIKER    6                 
 
STDAPI CreateURLMoniker(LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk);             
STDAPI GetClassURL(LPCWSTR szURL, CLSID *pClsID);                                           
STDAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb,                       
                                IEnumFORMATETC *pEFetc, IBindCtx **ppBC);                   
STDAPI MkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten,          
                                LPMONIKER *ppmk);                                           
STDAPI RegisterBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb,                     
                                IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved);       
STDAPI RevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb);                      
STDAPI GetClassFileOrMime(LPBC pBC, LPCWSTR szFilename, LPVOID pBuffer, DWORD cbSize, LPCWSTR szMime, DWORD dwReserved, CLSID *pclsid); 
STDAPI IsValidURL(LPBC pBC, LPCWSTR szURL, DWORD dwReserved);                               
STDAPI CoGetClassObjectFromURL( REFCLSID rCLASSID,
            LPCWSTR szCODE, DWORD dwFileVersionMS, 
            DWORD dwFileVersionLS, LPCWSTR szTYPE,
            LPBINDCTX pBindCtx, DWORD dwClsContext,
            LPVOID pvReserved, REFIID riid, LPVOID * ppv);
 
//helper apis                                                                               
STDAPI IsAsyncMoniker(IMoniker* pmk);                                                       
STDAPI CreateURLBinding(LPCWSTR lpszUrl, IBindCtx *pbc, IBinding **ppBdg);                  
 
STDAPI RegisterMediaTypesW(UINT ctypes, const LPCWSTR* rgszTypes, CLIPFORMAT* rgcfTypes);          
STDAPI RegisterMediaTypes(UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes);            
STDAPI FindMediaType(LPCSTR rgszTypes, CLIPFORMAT* rgcfTypes);                                       
STDAPI CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc); 
STDAPI RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved);          
STDAPI RevokeFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc);                            
STDAPI RegisterMediaTypeClass(LPBC pBC,UINT ctypes, const LPCSTR* rgszTypes, CLSID *rgclsID, DWORD reserved);    
STDAPI FindMediaTypeClass(LPBC pBC, LPCSTR szType, CLSID *pclsID, DWORD reserved);                          
STDAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved);       
 
// URLMON-specific defines for UrlMkSetSessionOption() above
#define URLMON_OPTION_USERAGENT  0x10000001
 
#define CF_NULL                 0                                  
#define CFSTR_MIME_NULL         NULL                               
#define CFSTR_MIME_TEXT         (TEXT("text/plain"))             
#define CFSTR_MIME_RICHTEXT     (TEXT("text/richtext"))          
#define CFSTR_MIME_X_BITMAP     (TEXT("image/x-xbitmap"))        
#define CFSTR_MIME_POSTSCRIPT   (TEXT("application/postscript")) 
#define CFSTR_MIME_AIFF         (TEXT("audio/aiff"))             
#define CFSTR_MIME_BASICAUDIO   (TEXT("audio/basic"))            
#define CFSTR_MIME_WAV          (TEXT("audio/wav"))              
#define CFSTR_MIME_X_WAV        (TEXT("audio/x-wav"))            
#define CFSTR_MIME_GIF          (TEXT("image/gif"))              
#define CFSTR_MIME_PJPEG        (TEXT("image/pjpeg"))            
#define CFSTR_MIME_JPEG         (TEXT("image/jpeg"))             
#define CFSTR_MIME_TIFF         (TEXT("image/tiff"))             
#define CFSTR_MIME_X_PNG        (TEXT("image/x-png"))            
#define CFSTR_MIME_BMP          (TEXT("image/bmp"))              
#define CFSTR_MIME_X_ART        (TEXT("image/x-jg"))             
#define CFSTR_MIME_AVI          (TEXT("video/avi"))              
#define CFSTR_MIME_MPEG         (TEXT("video/mpeg"))             
#define CFSTR_MIME_FRACTALS     (TEXT("application/fractals"))   
#define CFSTR_MIME_RAWDATA      (TEXT("application/octet-stream"))
#define CFSTR_MIME_RAWDATASTRM  (TEXT("application/octet-stream"))
#define CFSTR_MIME_PDF          (TEXT("application/pdf"))        
#define CFSTR_MIME_X_AIFF       (TEXT("audio/x-aiff"))           
#define CFSTR_MIME_X_REALAUDIO  (TEXT("audio/x-pn-realaudio"))   
#define CFSTR_MIME_XBM          (TEXT("image/xbm"))              
#define CFSTR_MIME_QUICKTIME    (TEXT("video/quicktime"))        
#define CFSTR_MIME_X_MSVIDEO    (TEXT("video/x-msvideo"))        
#define CFSTR_MIME_X_SGI_MOVIE  (TEXT("video/x-sgi-movie"))      
#define CFSTR_MIME_HTML         (TEXT("text/html"))              
 
// MessageId: MK_S_ASYNCHRONOUS                                              
// MessageText: Operation is successful, but will complete asynchronously.   
//                                                                           
#define MK_S_ASYNCHRONOUS    _HRESULT_TYPEDEF_(0x000401E8L)                  
#define S_ASYNCHRONOUS       MK_S_ASYNCHRONOUS                               
                                                                             
#ifndef E_PENDING                                                            
#define E_PENDING _HRESULT_TYPEDEF_(0x8000000AL)                             
#endif                                                                       
                                                                             
//                                                                           
//                                                                           
// WinINet and protocol specific errors are mapped to one of the following   
// error which are returned in IBSC::OnStopBinding                           
//                                                                           
//                                                                           
#define INET_E_INVALID_URL               _HRESULT_TYPEDEF_(0x800C0002L)      
#define INET_E_NO_SESSION                _HRESULT_TYPEDEF_(0x800C0003L)      
#define INET_E_CANNOT_CONNECT            _HRESULT_TYPEDEF_(0x800C0004L)      
#define INET_E_RESOURCE_NOT_FOUND        _HRESULT_TYPEDEF_(0x800C0005L)      
#define INET_E_OBJECT_NOT_FOUND          _HRESULT_TYPEDEF_(0x800C0006L)      
#define INET_E_DATA_NOT_AVAILABLE        _HRESULT_TYPEDEF_(0x800C0007L)      
#define INET_E_DOWNLOAD_FAILURE          _HRESULT_TYPEDEF_(0x800C0008L)      
#define INET_E_AUTHENTICATION_REQUIRED   _HRESULT_TYPEDEF_(0x800C0009L)      
#define INET_E_NO_VALID_MEDIA            _HRESULT_TYPEDEF_(0x800C000AL)      
#define INET_E_CONNECTION_TIMEOUT        _HRESULT_TYPEDEF_(0x800C000BL)      
#define INET_E_INVALID_REQUEST           _HRESULT_TYPEDEF_(0x800C000CL)      
#define INET_E_UNKNOWN_PROTOCOL          _HRESULT_TYPEDEF_(0x800C000DL)      
#define INET_E_SECURITY_PROBLEM          _HRESULT_TYPEDEF_(0x800C000EL)      
#define INET_E_CANNOT_LOAD_DATA          _HRESULT_TYPEDEF_(0x800C000FL)      
#define INET_E_CANNOT_INSTANTIATE_OBJECT _HRESULT_TYPEDEF_(0x800C0010L)      
#define INET_E_ERROR_FIRST               _HRESULT_TYPEDEF_(0x800C0002L)      
#define INET_E_ERROR_LAST                INET_E_CANNOT_INSTANTIATE_OBJECT    
#ifndef _LPPERSISTMONIKER_DEFINED
#define _LPPERSISTMONIKER_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IPersistMoniker_INTERFACE_DEFINED__
#define __IPersistMoniker_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPersistMoniker
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IPersistMoniker __RPC_FAR *LPPERSISTMONIKER;


EXTERN_C const IID IID_IPersistMoniker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPersistMoniker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveCompleted( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurMoniker( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistMonikerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPersistMoniker __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPersistMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )( 
            IPersistMoniker __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )( 
            IPersistMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveCompleted )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCurMoniker )( 
            IPersistMoniker __RPC_FAR * This,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);
        
        END_INTERFACE
    } IPersistMonikerVtbl;

    interface IPersistMoniker
    {
        CONST_VTBL struct IPersistMonikerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistMoniker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistMoniker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistMoniker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistMoniker_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)

#define IPersistMoniker_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IPersistMoniker_Load(This,fFullyAvailable,pimkName,pibc,grfMode)	\
    (This)->lpVtbl -> Load(This,fFullyAvailable,pimkName,pibc,grfMode)

#define IPersistMoniker_Save(This,pimkName,pbc,fRemember)	\
    (This)->lpVtbl -> Save(This,pimkName,pbc,fRemember)

#define IPersistMoniker_SaveCompleted(This,pimkName,pibc)	\
    (This)->lpVtbl -> SaveCompleted(This,pimkName,pibc)

#define IPersistMoniker_GetCurMoniker(This,ppimkName)	\
    (This)->lpVtbl -> GetCurMoniker(This,ppimkName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistMoniker_GetClassID_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pClassID);


void __RPC_STUB IPersistMoniker_GetClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_IsDirty_Proxy( 
    IPersistMoniker __RPC_FAR * This);


void __RPC_STUB IPersistMoniker_IsDirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_Load_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ BOOL fFullyAvailable,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pibc,
    /* [in] */ DWORD grfMode);


void __RPC_STUB IPersistMoniker_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_Save_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pbc,
    /* [in] */ BOOL fRemember);


void __RPC_STUB IPersistMoniker_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_SaveCompleted_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pibc);


void __RPC_STUB IPersistMoniker_SaveCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_GetCurMoniker_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);


void __RPC_STUB IPersistMoniker_GetCurMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistMoniker_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0082
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDPROTOCOL_DEFINED
#define _LPBINDPROTOCOL_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0082_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0082_v0_0_s_ifspec;

#ifndef __IBindProtocol_INTERFACE_DEFINED__
#define __IBindProtocol_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindProtocol
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IBindProtocol __RPC_FAR *LPBINDPROTOCOL;


EXTERN_C const IID IID_IBindProtocol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IBindProtocol : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateBinding( 
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindProtocolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindProtocol __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindProtocol __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindProtocol __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateBinding )( 
            IBindProtocol __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb);
        
        END_INTERFACE
    } IBindProtocolVtbl;

    interface IBindProtocol
    {
        CONST_VTBL struct IBindProtocolVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindProtocol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindProtocol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindProtocol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindProtocol_CreateBinding(This,szUrl,pbc,ppb)	\
    (This)->lpVtbl -> CreateBinding(This,szUrl,pbc,ppb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindProtocol_CreateBinding_Proxy( 
    IBindProtocol __RPC_FAR * This,
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb);


void __RPC_STUB IBindProtocol_CreateBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindProtocol_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0083
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDING_DEFINED
#define _LPBINDING_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0083_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0083_v0_0_s_ifspec;

#ifndef __IBinding_INTERFACE_DEFINED__
#define __IBinding_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBinding
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBinding __RPC_FAR *LPBINDING;


EXTERN_C const IID IID_IBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ LONG nPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG __RPC_FAR *pnPriority) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindResult( 
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBinding __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBinding __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Suspend )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriority )( 
            IBinding __RPC_FAR * This,
            /* [in] */ LONG nPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBinding __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindResult )( 
            IBinding __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved);
        
        END_INTERFACE
    } IBindingVtbl;

    interface IBinding
    {
        CONST_VTBL struct IBindingVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBinding_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IBinding_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IBinding_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IBinding_SetPriority(This,nPriority)	\
    (This)->lpVtbl -> SetPriority(This,nPriority)

#define IBinding_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBinding_GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)	\
    (This)->lpVtbl -> GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBinding_Abort_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_Suspend_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Suspend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_Resume_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_SetPriority_Proxy( 
    IBinding __RPC_FAR * This,
    /* [in] */ LONG nPriority);


void __RPC_STUB IBinding_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_GetPriority_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnPriority);


void __RPC_STUB IBinding_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBinding_RemoteGetBindResult_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBinding_RemoteGetBindResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBinding_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0084
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDSTATUSCALLBACK_DEFINED
#define _LPBINDSTATUSCALLBACK_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0084_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0084_v0_0_s_ifspec;

#ifndef __IBindStatusCallback_INTERFACE_DEFINED__
#define __IBindStatusCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindStatusCallback
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBindStatusCallback __RPC_FAR *LPBINDSTATUSCALLBACK;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0001
    {	BINDVERB_GET	= 0,
	BINDVERB_POST	= 0x1,
	BINDVERB_PUT	= 0x2,
	BINDVERB_CUSTOM	= 0x3
    }	BINDVERB;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0002
    {	BINDINFOF_URLENCODESTGMEDDATA	= 0x1,
	BINDINFOF_URLENCODEDEXTRAINFO	= 0x2
    }	BINDINFOF;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0003
    {	BINDF_ASYNCHRONOUS	= 0x1,
	BINDF_ASYNCSTORAGE	= 0x2,
	BINDF_NOPROGRESSIVERENDERING	= 0x4,
	BINDF_OFFLINEOPERATION	= 0x8,
	BINDF_GETNEWESTVERSION	= 0x10,
	BINDF_NOWRITECACHE	= 0x20,
	BINDF_PULLDATA	= 0x80,
	BINDF_IGNORESECURITYPROBLEM	= 0x100,
	BINDF_RESYNCHRONIZE	= 0x200,
	BINDF_HYPERLINK	= 0x400,
	BINDF_INLINESGETNEWESTVERSION	= 0x10000000,
	BINDF_INLINESRESYNCHRONIZE	= 0x20000000,
	BINDF_CONTAINER_NOWRITECACHE	= 0x40000000
    }	BINDF;

typedef struct  _tagBINDINFO
    {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    STGMEDIUM stgmedData;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbstgmedData;
    }	BINDINFO;

typedef struct  _tagRemBINDINFO
    {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbstgmedData;
    }	RemBINDINFO;

typedef struct  tagRemFORMATETC
    {
    DWORD cfFormat;
    DWORD ptd;
    DWORD dwAspect;
    LONG lindex;
    DWORD tymed;
    }	RemFORMATETC;

typedef struct tagRemFORMATETC __RPC_FAR *LPREMFORMATETC;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0004
    {	BSCF_FIRSTDATANOTIFICATION	= 0x1,
	BSCF_INTERMEDIATEDATANOTIFICATION	= 0x2,
	BSCF_LASTDATANOTIFICATION	= 0x4
    }	BSCF;

typedef 
enum tagBINDSTATUS
    {	BINDSTATUS_FINDINGRESOURCE	= 1,
	BINDSTATUS_CONNECTING	= BINDSTATUS_FINDINGRESOURCE + 1,
	BINDSTATUS_REDIRECTING	= BINDSTATUS_CONNECTING + 1,
	BINDSTATUS_BEGINDOWNLOADDATA	= BINDSTATUS_REDIRECTING + 1,
	BINDSTATUS_DOWNLOADINGDATA	= BINDSTATUS_BEGINDOWNLOADDATA + 1,
	BINDSTATUS_ENDDOWNLOADDATA	= BINDSTATUS_DOWNLOADINGDATA + 1,
	BINDSTATUS_BEGINDOWNLOADCOMPONENTS	= BINDSTATUS_ENDDOWNLOADDATA + 1,
	BINDSTATUS_INSTALLINGCOMPONENTS	= BINDSTATUS_BEGINDOWNLOADCOMPONENTS + 1,
	BINDSTATUS_ENDDOWNLOADCOMPONENTS	= BINDSTATUS_INSTALLINGCOMPONENTS + 1,
	BINDSTATUS_USINGCACHEDCOPY	= BINDSTATUS_ENDDOWNLOADCOMPONENTS + 1,
	BINDSTATUS_SENDINGREQUEST	= BINDSTATUS_USINGCACHEDCOPY + 1,
	BINDSTATUS_CLASSIDAVAILABLE	= BINDSTATUS_SENDINGREQUEST + 1,
	BINDSTATUS_MIMETYPEAVAILABLE	= BINDSTATUS_CLASSIDAVAILABLE + 1,
	BINDSTATUS_CACHEFILENAMEAVAILABLE	= BINDSTATUS_MIMETYPEAVAILABLE + 1
    }	BINDSTATUS;


EXTERN_C const IID IID_IBindStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IBindStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartBinding( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG __RPC_FAR *pnPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DWORD reserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStopBinding( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindInfo( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE OnDataAvailable( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindStatusCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindStatusCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartBinding )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnLowResource )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStopBinding )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataAvailable )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnObjectAvailable )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);
        
        END_INTERFACE
    } IBindStatusCallbackVtbl;

    interface IBindStatusCallback
    {
        CONST_VTBL struct IBindStatusCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindStatusCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindStatusCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindStatusCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindStatusCallback_OnStartBinding(This,dwReserved,pib)	\
    (This)->lpVtbl -> OnStartBinding(This,dwReserved,pib)

#define IBindStatusCallback_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBindStatusCallback_OnLowResource(This,reserved)	\
    (This)->lpVtbl -> OnLowResource(This,reserved)

#define IBindStatusCallback_OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)

#define IBindStatusCallback_OnStopBinding(This,hresult,szError)	\
    (This)->lpVtbl -> OnStopBinding(This,hresult,szError)

#define IBindStatusCallback_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IBindStatusCallback_OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)	\
    (This)->lpVtbl -> OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)

#define IBindStatusCallback_OnObjectAvailable(This,riid,punk)	\
    (This)->lpVtbl -> OnObjectAvailable(This,riid,punk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnStartBinding_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD dwReserved,
    /* [in] */ IBinding __RPC_FAR *pib);


void __RPC_STUB IBindStatusCallback_OnStartBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetPriority_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnPriority);


void __RPC_STUB IBindStatusCallback_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnLowResource_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD reserved);


void __RPC_STUB IBindStatusCallback_OnLowResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnProgress_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText);


void __RPC_STUB IBindStatusCallback_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnStopBinding_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ HRESULT hresult,
    /* [unique][in] */ LPCWSTR szError);


void __RPC_STUB IBindStatusCallback_OnStopBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_RemoteGetBindInfo_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ RemBINDINFO __RPC_FAR *pbindinfo,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);


void __RPC_STUB IBindStatusCallback_RemoteGetBindInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_RemoteOnDataAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ RemFORMATETC __RPC_FAR *pformatetc,
    /* [in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);


void __RPC_STUB IBindStatusCallback_RemoteOnDataAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnObjectAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown __RPC_FAR *punk);


void __RPC_STUB IBindStatusCallback_OnObjectAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindStatusCallback_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0085
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPAUTHENTICATION_DEFINED
#define _LPAUTHENTICATION_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0085_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0085_v0_0_s_ifspec;

#ifndef __IAuthenticate_INTERFACE_DEFINED__
#define __IAuthenticate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAuthenticate
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IAuthenticate __RPC_FAR *LPAUTHENTICATION;


EXTERN_C const IID IID_IAuthenticate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAuthenticate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Authenticate( 
            /* [out] */ HWND __RPC_FAR *phwnd,
            /* [out] */ LPWSTR __RPC_FAR *pszUsername,
            /* [out] */ LPWSTR __RPC_FAR *pszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAuthenticateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAuthenticate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAuthenticate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAuthenticate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Authenticate )( 
            IAuthenticate __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd,
            /* [out] */ LPWSTR __RPC_FAR *pszUsername,
            /* [out] */ LPWSTR __RPC_FAR *pszPassword);
        
        END_INTERFACE
    } IAuthenticateVtbl;

    interface IAuthenticate
    {
        CONST_VTBL struct IAuthenticateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuthenticate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAuthenticate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAuthenticate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAuthenticate_Authenticate(This,phwnd,pszUsername,pszPassword)	\
    (This)->lpVtbl -> Authenticate(This,phwnd,pszUsername,pszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAuthenticate_Authenticate_Proxy( 
    IAuthenticate __RPC_FAR * This,
    /* [out] */ HWND __RPC_FAR *phwnd,
    /* [out] */ LPWSTR __RPC_FAR *pszUsername,
    /* [out] */ LPWSTR __RPC_FAR *pszPassword);


void __RPC_STUB IAuthenticate_Authenticate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAuthenticate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0086
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPHTTPNEGOTIATE_DEFINED
#define _LPHTTPNEGOTIATE_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0086_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0086_v0_0_s_ifspec;

#ifndef __IHttpNegotiate_INTERFACE_DEFINED__
#define __IHttpNegotiate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHttpNegotiate
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IHttpNegotiate __RPC_FAR *LPHTTPNEGOTIATE;


EXTERN_C const IID IID_IHttpNegotiate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IHttpNegotiate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginningTransaction( 
            /* [in] */ LPCWSTR szURL,
            /* [unique][in] */ LPCWSTR szHeaders,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ DWORD dwResponseCode,
            /* [unique][in] */ LPCWSTR szResponseHeaders,
            /* [unique][in] */ LPCWSTR szRequestHeaders,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpNegotiateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpNegotiate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpNegotiate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginningTransaction )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ LPCWSTR szURL,
            /* [unique][in] */ LPCWSTR szHeaders,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnResponse )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ DWORD dwResponseCode,
            /* [unique][in] */ LPCWSTR szResponseHeaders,
            /* [unique][in] */ LPCWSTR szRequestHeaders,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders);
        
        END_INTERFACE
    } IHttpNegotiateVtbl;

    interface IHttpNegotiate
    {
        CONST_VTBL struct IHttpNegotiateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpNegotiate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpNegotiate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpNegotiate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpNegotiate_BeginningTransaction(This,szURL,szHeaders,dwReserved,pszAdditionalHeaders)	\
    (This)->lpVtbl -> BeginningTransaction(This,szURL,szHeaders,dwReserved,pszAdditionalHeaders)

#define IHttpNegotiate_OnResponse(This,dwResponseCode,szResponseHeaders,szRequestHeaders,pszAdditionalRequestHeaders)	\
    (This)->lpVtbl -> OnResponse(This,dwResponseCode,szResponseHeaders,szRequestHeaders,pszAdditionalRequestHeaders)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpNegotiate_BeginningTransaction_Proxy( 
    IHttpNegotiate __RPC_FAR * This,
    /* [in] */ LPCWSTR szURL,
    /* [unique][in] */ LPCWSTR szHeaders,
    /* [in] */ DWORD dwReserved,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders);


void __RPC_STUB IHttpNegotiate_BeginningTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHttpNegotiate_OnResponse_Proxy( 
    IHttpNegotiate __RPC_FAR * This,
    /* [in] */ DWORD dwResponseCode,
    /* [unique][in] */ LPCWSTR szResponseHeaders,
    /* [unique][in] */ LPCWSTR szRequestHeaders,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders);


void __RPC_STUB IHttpNegotiate_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpNegotiate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0087
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWINDOWFORBINDINGUI_DEFINED
#define _LPWINDOWFORBINDINGUI_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0087_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0087_v0_0_s_ifspec;

#ifndef __IWindowForBindingUI_INTERFACE_DEFINED__
#define __IWindowForBindingUI_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWindowForBindingUI
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IWindowForBindingUI __RPC_FAR *LPWINDOWFORBINDINGUI;


EXTERN_C const IID IID_IWindowForBindingUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWindowForBindingUI : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetWindow( 
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWindowForBindingUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWindowForBindingUI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWindowForBindingUI __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWindowForBindingUI __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            IWindowForBindingUI __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        END_INTERFACE
    } IWindowForBindingUIVtbl;

    interface IWindowForBindingUI
    {
        CONST_VTBL struct IWindowForBindingUIVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWindowForBindingUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWindowForBindingUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWindowForBindingUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWindowForBindingUI_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWindowForBindingUI_GetWindow_Proxy( 
    IWindowForBindingUI __RPC_FAR * This,
    /* [in] */ REFGUID rguidReason,
    /* [out] */ HWND __RPC_FAR *phwnd);


void __RPC_STUB IWindowForBindingUI_GetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWindowForBindingUI_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0088
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPCODEINSTALL_DEFINED
#define _LPCODEINSTALL_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0088_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0088_v0_0_s_ifspec;

#ifndef __ICodeInstall_INTERFACE_DEFINED__
#define __ICodeInstall_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICodeInstall
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ ICodeInstall __RPC_FAR *LPCODEINSTALL;

typedef /* [public] */ 
enum __MIDL_ICodeInstall_0001
    {	CIP_DISK_FULL	= 0,
	CIP_ACCESS_DENIED	= CIP_DISK_FULL + 1,
	CIP_NEWER_VERSION_EXISTS	= CIP_ACCESS_DENIED + 1,
	CIP_OLDER_VERSION_EXISTS	= CIP_NEWER_VERSION_EXISTS + 1,
	CIP_NAME_CONFLICT	= CIP_OLDER_VERSION_EXISTS + 1,
	CIP_TRUST_VERIFICATION_COMPONENT_MISSING	= CIP_NAME_CONFLICT + 1,
	CIP_EXE_SELF_REGISTERATION_TIMEOUT	= CIP_TRUST_VERIFICATION_COMPONENT_MISSING + 1,
	CIP_UNSAFE_TO_ABORT	= CIP_EXE_SELF_REGISTERATION_TIMEOUT + 1,
	CIP_NEED_REBOOT	= CIP_UNSAFE_TO_ABORT + 1
    }	CIP_STATUS;


EXTERN_C const IID IID_ICodeInstall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICodeInstall : public IWindowForBindingUI
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCodeInstallProblem( 
            /* [in] */ ULONG ulStatusCode,
            /* [unique][in] */ LPCWSTR szDestination,
            /* [unique][in] */ LPCWSTR szSource,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodeInstallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICodeInstall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICodeInstall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnCodeInstallProblem )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ ULONG ulStatusCode,
            /* [unique][in] */ LPCWSTR szDestination,
            /* [unique][in] */ LPCWSTR szSource,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } ICodeInstallVtbl;

    interface ICodeInstall
    {
        CONST_VTBL struct ICodeInstallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodeInstall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodeInstall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodeInstall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodeInstall_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)


#define ICodeInstall_OnCodeInstallProblem(This,ulStatusCode,szDestination,szSource,dwReserved)	\
    (This)->lpVtbl -> OnCodeInstallProblem(This,ulStatusCode,szDestination,szSource,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodeInstall_OnCodeInstallProblem_Proxy( 
    ICodeInstall __RPC_FAR * This,
    /* [in] */ ULONG ulStatusCode,
    /* [unique][in] */ LPCWSTR szDestination,
    /* [unique][in] */ LPCWSTR szSource,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ICodeInstall_OnCodeInstallProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodeInstall_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0089
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWININETINFO_DEFINED
#define _LPWININETINFO_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0089_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0089_v0_0_s_ifspec;

#ifndef __IWinInetInfo_INTERFACE_DEFINED__
#define __IWinInetInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWinInetInfo
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IWinInetInfo __RPC_FAR *LPWININETINFO;


EXTERN_C const IID IID_IWinInetInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWinInetInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryOption( 
            /* [in] */ DWORD dwOption,
            /* [out] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWinInetInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWinInetInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWinInetInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWinInetInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IWinInetInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [out] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf);
        
        END_INTERFACE
    } IWinInetInfoVtbl;

    interface IWinInetInfo
    {
        CONST_VTBL struct IWinInetInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWinInetInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWinInetInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWinInetInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWinInetInfo_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWinInetInfo_QueryOption_Proxy( 
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [out] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf);


void __RPC_STUB IWinInetInfo_QueryOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWinInetInfo_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0090
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPHTTPSECURITY_DEFINED
#define _LPHTTPSECURITY_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0090_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0090_v0_0_s_ifspec;

#ifndef __IHttpSecurity_INTERFACE_DEFINED__
#define __IHttpSecurity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHttpSecurity
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IHttpSecurity __RPC_FAR *LPHTTPSECURITY;


EXTERN_C const IID IID_IHttpSecurity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IHttpSecurity : public IWindowForBindingUI
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSecurityProblem( 
            /* [in] */ DWORD dwProblem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpSecurityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpSecurity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpSecurity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSecurityProblem )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ DWORD dwProblem);
        
        END_INTERFACE
    } IHttpSecurityVtbl;

    interface IHttpSecurity
    {
        CONST_VTBL struct IHttpSecurityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpSecurity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpSecurity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpSecurity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpSecurity_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)


#define IHttpSecurity_OnSecurityProblem(This,dwProblem)	\
    (This)->lpVtbl -> OnSecurityProblem(This,dwProblem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpSecurity_OnSecurityProblem_Proxy( 
    IHttpSecurity __RPC_FAR * This,
    /* [in] */ DWORD dwProblem);


void __RPC_STUB IHttpSecurity_OnSecurityProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpSecurity_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0091
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWININETHTTPINFO_DEFINED
#define _LPWININETHTTPINFO_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0091_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0091_v0_0_s_ifspec;

#ifndef __IWinInetHttpInfo_INTERFACE_DEFINED__
#define __IWinInetHttpInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWinInetHttpInfo
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IWinInetHttpInfo __RPC_FAR *LPWININETHTTPINFO;


EXTERN_C const IID IID_IWinInetHttpInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWinInetHttpInfo : public IWinInetInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInfo( 
            /* [in] */ DWORD dwOption,
            /* [out] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWinInetHttpInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWinInetHttpInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWinInetHttpInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [out] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInfo )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [out] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved);
        
        END_INTERFACE
    } IWinInetHttpInfoVtbl;

    interface IWinInetHttpInfo
    {
        CONST_VTBL struct IWinInetHttpInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWinInetHttpInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWinInetHttpInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWinInetHttpInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWinInetHttpInfo_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)


#define IWinInetHttpInfo_QueryInfo(This,dwOption,pBuffer,pcbBuf,pdwFlags,pdwReserved)	\
    (This)->lpVtbl -> QueryInfo(This,dwOption,pBuffer,pcbBuf,pdwFlags,pdwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_QueryInfo_Proxy( 
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [out] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);


void __RPC_STUB IWinInetHttpInfo_QueryInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWinInetHttpInfo_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0092
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#define SID_IBindHost IID_IBindHost
#define SID_SBindHost IID_IBindHost
#ifndef _LPBINDHOST_DEFINED
#define _LPBINDHOST_DEFINED
EXTERN_C const GUID SID_BindHost;


extern RPC_IF_HANDLE __MIDL__intf_0092_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0092_v0_0_s_ifspec;

#ifndef __IBindHost_INTERFACE_DEFINED__
#define __IBindHost_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindHost
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBindHost __RPC_FAR *LPBINDHOST;


EXTERN_C const IID IID_IBindHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IBindHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateMoniker( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE MonikerBindToStorage( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE MonikerBindToObject( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindHost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindHost __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateMoniker )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
            /* [in] */ DWORD dwReserved);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonikerBindToStorage )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonikerBindToObject )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
        END_INTERFACE
    } IBindHostVtbl;

    interface IBindHost
    {
        CONST_VTBL struct IBindHostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindHost_CreateMoniker(This,szName,pBC,ppmk,dwReserved)	\
    (This)->lpVtbl -> CreateMoniker(This,szName,pBC,ppmk,dwReserved)

#define IBindHost_MonikerBindToStorage(This,pMk,pBC,pBSC,riid,ppvObj)	\
    (This)->lpVtbl -> MonikerBindToStorage(This,pMk,pBC,pBSC,riid,ppvObj)

#define IBindHost_MonikerBindToObject(This,pMk,pBC,pBSC,riid,ppvObj)	\
    (This)->lpVtbl -> MonikerBindToObject(This,pMk,pBC,pBSC,riid,ppvObj)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindHost_CreateMoniker_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBindHost_CreateMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_RemoteMonikerBindToStorage_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);


void __RPC_STUB IBindHost_RemoteMonikerBindToStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_RemoteMonikerBindToObject_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);


void __RPC_STUB IBindHost_RemoteMonikerBindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindHost_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0093
 * at Fri Aug 02 15:31:22 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
                                                                                                           
// These are for backwards compatibility with previous URLMON versions
// Flags for the UrlDownloadToCacheFile                                                                    
#define URLOSTRM_USECACHEDCOPY_ONLY             0x1      // Only get from cache                            
#define URLOSTRM_USECACHEDCOPY                  0x2      // Get from cache if available else download      
#define URLOSTRM_GETNEWESTVERSION               0x3      // Get new version only. But put it in cache too  
                                                                                                           
                                                                                                           
struct IBindStatusCallback;                                                                                
STDAPI HlinkSimpleNavigateToString(                                                                        
    /* [in] */ LPCWSTR szTarget,         // required - target document - null if local jump w/in doc       
    /* [in] */ LPCWSTR szLocation,       // optional, for navigation into middle of a doc                  
    /* [in] */ LPCWSTR szTargetFrameName,// optional, for targeting frame-sets                             
    /* [in] */ IUnknown *pUnk,           // required - we'll search this for other necessary interfaces    
    /* [in] */ IBindCtx *pbc,            // optional. caller may register an IBSC in this                  
    /* [in] */ IBindStatusCallback *,                                                                      
    /* [in] */ DWORD grfHLNF,            // flags                                                          
    /* [in] */ DWORD dwReserved          // for future use, must be NULL                                   
);                                                                                                         
                                                                                                           
STDAPI HlinkSimpleNavigateToMoniker(                                                                       
    /* [in] */ IMoniker *pmkTarget,      // required - target document - (may be null                      
    /* [in] */ LPCWSTR szLocation,       // optional, for navigation into middle of a doc                  
    /* [in] */ LPCWSTR szTargetFrameName,// optional, for targeting frame-sets                             
    /* [in] */ IUnknown *pUnk,           // required - we'll search this for other necessary interfaces    
    /* [in] */ IBindCtx *pbc,            // optional. caller may register an IBSC in this                  
    /* [in] */ IBindStatusCallback *,                                                                      
    /* [in] */ DWORD grfHLNF,            // flags                                                          
    /* [in] */ DWORD dwReserved          // for future use, must be NULL                                   
);                                                                                                         
                                                                                                           
STDAPI URLOpenStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                                        
STDAPI URLOpenStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                                       
STDAPI URLOpenPullStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                                    
STDAPI URLOpenPullStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                                   
STDAPI URLDownloadToFileA(LPUNKNOWN,LPCSTR,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                             
STDAPI URLDownloadToFileW(LPUNKNOWN,LPCWSTR,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                           
STDAPI URLDownloadToCacheFileA(LPUNKNOWN,LPCSTR,LPTSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);                  
STDAPI URLDownloadToCacheFileW(LPUNKNOWN,LPCWSTR,LPWSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);                 
STDAPI URLOpenBlockingStreamA(LPUNKNOWN,LPCSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);                      
STDAPI URLOpenBlockingStreamW(LPUNKNOWN,LPCWSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);                     
                                                                                                           
#ifdef UNICODE                                                                                             
#define URLOpenStream            URLOpenStreamW                                                            
#define URLOpenPullStream        URLOpenPullStreamW                                                        
#define URLDownloadToFile        URLDownloadToFileW                                                        
#define URLDownloadToCacheFile   URLDownloadToCacheFileW                                                   
#define URLOpenBlockingStream    URLOpenBlockingStreamW                                                    
#else                                                                                                      
#define URLOpenStream            URLOpenStreamA                                                            
#define URLOpenPullStream        URLOpenPullStreamA                                                        
#define URLDownloadToFile        URLDownloadToFileA                                                        
#define URLDownloadToCacheFile   URLDownloadToCacheFileA                                                   
#define URLOpenBlockingStream    URLOpenBlockingStreamA                                                    
#endif // !UNICODE                                                                                         
                                                                                                           
                                                                                                           
STDAPI HlinkGoBack(IUnknown *pUnk);                                                                        
STDAPI HlinkGoForward(IUnknown *pUnk);                                                                     
STDAPI HlinkNavigateString(IUnknown *pUnk, LPCWSTR szTarget);                                              
STDAPI HlinkNavigateMoniker(IUnknown *pUnk, IMoniker *pmkTarget);                                          
                                                                                                           


extern RPC_IF_HANDLE __MIDL__intf_0093_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0093_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* [local] */ HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Stub( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Stub( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ RemBINDINFO __RPC_FAR *pbindinfo,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Stub( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ RemFORMATETC __RPC_FAR *pformatetc,
    /* [in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Stub( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Stub( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
