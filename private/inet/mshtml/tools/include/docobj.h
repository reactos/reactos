/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Sun Jul 07 10:43:37 1996
 */
/* Compiler settings for docobj.idl:
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

#ifndef __docobj_h__
#define __docobj_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IOleDocument_FWD_DEFINED__
#define __IOleDocument_FWD_DEFINED__
typedef interface IOleDocument IOleDocument;
#endif 	/* __IOleDocument_FWD_DEFINED__ */


#ifndef __IOleDocumentSite_FWD_DEFINED__
#define __IOleDocumentSite_FWD_DEFINED__
typedef interface IOleDocumentSite IOleDocumentSite;
#endif 	/* __IOleDocumentSite_FWD_DEFINED__ */


#ifndef __IOleDocumentView_FWD_DEFINED__
#define __IOleDocumentView_FWD_DEFINED__
typedef interface IOleDocumentView IOleDocumentView;
#endif 	/* __IOleDocumentView_FWD_DEFINED__ */


#ifndef __IEnumOleDocumentViews_FWD_DEFINED__
#define __IEnumOleDocumentViews_FWD_DEFINED__
typedef interface IEnumOleDocumentViews IEnumOleDocumentViews;
#endif 	/* __IEnumOleDocumentViews_FWD_DEFINED__ */


#ifndef __IContinueCallback_FWD_DEFINED__
#define __IContinueCallback_FWD_DEFINED__
typedef interface IContinueCallback IContinueCallback;
#endif 	/* __IContinueCallback_FWD_DEFINED__ */


#ifndef __IPrint_FWD_DEFINED__
#define __IPrint_FWD_DEFINED__
typedef interface IPrint IPrint;
#endif 	/* __IPrint_FWD_DEFINED__ */


#ifndef __IOleCommandTarget_FWD_DEFINED__
#define __IOleCommandTarget_FWD_DEFINED__
typedef interface IOleCommandTarget IOleCommandTarget;
#endif 	/* __IOleCommandTarget_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"
#include "servprov.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// DocObj.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//--------------------------------------------------------------------------
// OLE Document Object Interfaces.








////////////////////////////////////////////////////////////////////////////
//  Interface Definitions
#ifndef _LPOLEDOCUMENT_DEFINED
#define _LPOLEDOCUMENT_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IOleDocument_INTERFACE_DEFINED__
#define __IOleDocument_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOleDocument
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IOleDocument __RPC_FAR *LPOLEDOCUMENT;

typedef /* [public] */ 
enum __MIDL_IOleDocument_0001
    {	DOCMISC_CANCREATEMULTIPLEVIEWS	= 1,
	DOCMISC_SUPPORTCOMPLEXRECTANGLES	= 2,
	DOCMISC_CANTOPENEDIT	= 4,
	DOCMISC_NOFILESUPPORT	= 8
    }	DOCMISC;


EXTERN_C const IID IID_IOleDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleDocument : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateView( 
            /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite,
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ DWORD dwReserved,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDocMiscStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumViews( 
            /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleDocumentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOleDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOleDocument __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOleDocument __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateView )( 
            IOleDocument __RPC_FAR * This,
            /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite,
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ DWORD dwReserved,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocMiscStatus )( 
            IOleDocument __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumViews )( 
            IOleDocument __RPC_FAR * This,
            /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView);
        
        END_INTERFACE
    } IOleDocumentVtbl;

    interface IOleDocument
    {
        CONST_VTBL struct IOleDocumentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleDocument_CreateView(This,pIPSite,pstm,dwReserved,ppView)	\
    (This)->lpVtbl -> CreateView(This,pIPSite,pstm,dwReserved,ppView)

#define IOleDocument_GetDocMiscStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetDocMiscStatus(This,pdwStatus)

#define IOleDocument_EnumViews(This,ppEnum,ppView)	\
    (This)->lpVtbl -> EnumViews(This,ppEnum,ppView)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOleDocument_CreateView_Proxy( 
    IOleDocument __RPC_FAR * This,
    /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite,
    /* [unique][in] */ IStream __RPC_FAR *pstm,
    /* [in] */ DWORD dwReserved,
    /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView);


void __RPC_STUB IOleDocument_CreateView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocument_GetDocMiscStatus_Proxy( 
    IOleDocument __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IOleDocument_GetDocMiscStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocument_EnumViews_Proxy( 
    IOleDocument __RPC_FAR * This,
    /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum,
    /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppView);


void __RPC_STUB IOleDocument_EnumViews_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOleDocument_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0142
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOLEDOCUMENTSITE_DEFINED
#define _LPOLEDOCUMENTSITE_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0142_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0142_v0_0_s_ifspec;

#ifndef __IOleDocumentSite_INTERFACE_DEFINED__
#define __IOleDocumentSite_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOleDocumentSite
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IOleDocumentSite __RPC_FAR *LPOLEDOCUMENTSITE;


EXTERN_C const IID IID_IOleDocumentSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleDocumentSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ActivateMe( 
            /* [in] */ IOleDocumentView __RPC_FAR *pViewToActivate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleDocumentSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOleDocumentSite __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOleDocumentSite __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOleDocumentSite __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ActivateMe )( 
            IOleDocumentSite __RPC_FAR * This,
            /* [in] */ IOleDocumentView __RPC_FAR *pViewToActivate);
        
        END_INTERFACE
    } IOleDocumentSiteVtbl;

    interface IOleDocumentSite
    {
        CONST_VTBL struct IOleDocumentSiteVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleDocumentSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleDocumentSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleDocumentSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleDocumentSite_ActivateMe(This,pViewToActivate)	\
    (This)->lpVtbl -> ActivateMe(This,pViewToActivate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOleDocumentSite_ActivateMe_Proxy( 
    IOleDocumentSite __RPC_FAR * This,
    /* [in] */ IOleDocumentView __RPC_FAR *pViewToActivate);


void __RPC_STUB IOleDocumentSite_ActivateMe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOleDocumentSite_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0143
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOLEDOCUMENTVIEW_DEFINED
#define _LPOLEDOCUMENTVIEW_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0143_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0143_v0_0_s_ifspec;

#ifndef __IOleDocumentView_INTERFACE_DEFINED__
#define __IOleDocumentView_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOleDocumentView
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IOleDocumentView __RPC_FAR *LPOLEDOCUMENTVIEW;


EXTERN_C const IID IID_IOleDocumentView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleDocumentView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInPlaceSite( 
            /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInPlaceSite( 
            /* [out] */ IOleInPlaceSite __RPC_FAR *__RPC_FAR *ppIPSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDocument( 
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk) = 0;
        
        virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE SetRect( 
            /* [in] */ LPRECT prcView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRect( 
            /* [out] */ LPRECT prcView) = 0;
        
        virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE SetRectComplex( 
            /* [unique][in] */ LPRECT prcView,
            /* [unique][in] */ LPRECT prcHScroll,
            /* [unique][in] */ LPRECT prcVScroll,
            /* [unique][in] */ LPRECT prcSizeBox) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ BOOL fShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UIActivate( 
            /* [in] */ BOOL fUIActivate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseView( 
            DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveViewState( 
            /* [in] */ LPSTREAM pstm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplyViewState( 
            /* [in] */ LPSTREAM pstm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [in] */ IOleInPlaceSite __RPC_FAR *pIPSiteNew,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppViewNew) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleDocumentViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOleDocumentView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOleDocumentView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInPlaceSite )( 
            IOleDocumentView __RPC_FAR * This,
            /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInPlaceSite )( 
            IOleDocumentView __RPC_FAR * This,
            /* [out] */ IOleInPlaceSite __RPC_FAR *__RPC_FAR *ppIPSite);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocument )( 
            IOleDocumentView __RPC_FAR * This,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk);
        
        /* [input_sync] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRect )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ LPRECT prcView);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRect )( 
            IOleDocumentView __RPC_FAR * This,
            /* [out] */ LPRECT prcView);
        
        /* [input_sync] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRectComplex )( 
            IOleDocumentView __RPC_FAR * This,
            /* [unique][in] */ LPRECT prcView,
            /* [unique][in] */ LPRECT prcHScroll,
            /* [unique][in] */ LPRECT prcVScroll,
            /* [unique][in] */ LPRECT prcSizeBox);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Show )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UIActivate )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ BOOL fUIActivate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IOleDocumentView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseView )( 
            IOleDocumentView __RPC_FAR * This,
            DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveViewState )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ LPSTREAM pstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ApplyViewState )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ LPSTREAM pstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IOleDocumentView __RPC_FAR * This,
            /* [in] */ IOleInPlaceSite __RPC_FAR *pIPSiteNew,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppViewNew);
        
        END_INTERFACE
    } IOleDocumentViewVtbl;

    interface IOleDocumentView
    {
        CONST_VTBL struct IOleDocumentViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleDocumentView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleDocumentView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleDocumentView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleDocumentView_SetInPlaceSite(This,pIPSite)	\
    (This)->lpVtbl -> SetInPlaceSite(This,pIPSite)

#define IOleDocumentView_GetInPlaceSite(This,ppIPSite)	\
    (This)->lpVtbl -> GetInPlaceSite(This,ppIPSite)

#define IOleDocumentView_GetDocument(This,ppunk)	\
    (This)->lpVtbl -> GetDocument(This,ppunk)

#define IOleDocumentView_SetRect(This,prcView)	\
    (This)->lpVtbl -> SetRect(This,prcView)

#define IOleDocumentView_GetRect(This,prcView)	\
    (This)->lpVtbl -> GetRect(This,prcView)

#define IOleDocumentView_SetRectComplex(This,prcView,prcHScroll,prcVScroll,prcSizeBox)	\
    (This)->lpVtbl -> SetRectComplex(This,prcView,prcHScroll,prcVScroll,prcSizeBox)

#define IOleDocumentView_Show(This,fShow)	\
    (This)->lpVtbl -> Show(This,fShow)

#define IOleDocumentView_UIActivate(This,fUIActivate)	\
    (This)->lpVtbl -> UIActivate(This,fUIActivate)

#define IOleDocumentView_Open(This)	\
    (This)->lpVtbl -> Open(This)

#define IOleDocumentView_CloseView(This,dwReserved)	\
    (This)->lpVtbl -> CloseView(This,dwReserved)

#define IOleDocumentView_SaveViewState(This,pstm)	\
    (This)->lpVtbl -> SaveViewState(This,pstm)

#define IOleDocumentView_ApplyViewState(This,pstm)	\
    (This)->lpVtbl -> ApplyViewState(This,pstm)

#define IOleDocumentView_Clone(This,pIPSiteNew,ppViewNew)	\
    (This)->lpVtbl -> Clone(This,pIPSiteNew,ppViewNew)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOleDocumentView_SetInPlaceSite_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [unique][in] */ IOleInPlaceSite __RPC_FAR *pIPSite);


void __RPC_STUB IOleDocumentView_SetInPlaceSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_GetInPlaceSite_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [out] */ IOleInPlaceSite __RPC_FAR *__RPC_FAR *ppIPSite);


void __RPC_STUB IOleDocumentView_GetInPlaceSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_GetDocument_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk);


void __RPC_STUB IOleDocumentView_GetDocument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [input_sync] */ HRESULT STDMETHODCALLTYPE IOleDocumentView_SetRect_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ LPRECT prcView);


void __RPC_STUB IOleDocumentView_SetRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_GetRect_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [out] */ LPRECT prcView);


void __RPC_STUB IOleDocumentView_GetRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [input_sync] */ HRESULT STDMETHODCALLTYPE IOleDocumentView_SetRectComplex_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [unique][in] */ LPRECT prcView,
    /* [unique][in] */ LPRECT prcHScroll,
    /* [unique][in] */ LPRECT prcVScroll,
    /* [unique][in] */ LPRECT prcSizeBox);


void __RPC_STUB IOleDocumentView_SetRectComplex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_Show_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ BOOL fShow);


void __RPC_STUB IOleDocumentView_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_UIActivate_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ BOOL fUIActivate);


void __RPC_STUB IOleDocumentView_UIActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_Open_Proxy( 
    IOleDocumentView __RPC_FAR * This);


void __RPC_STUB IOleDocumentView_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_CloseView_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    DWORD dwReserved);


void __RPC_STUB IOleDocumentView_CloseView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_SaveViewState_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ LPSTREAM pstm);


void __RPC_STUB IOleDocumentView_SaveViewState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_ApplyViewState_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ LPSTREAM pstm);


void __RPC_STUB IOleDocumentView_ApplyViewState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleDocumentView_Clone_Proxy( 
    IOleDocumentView __RPC_FAR * This,
    /* [in] */ IOleInPlaceSite __RPC_FAR *pIPSiteNew,
    /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *ppViewNew);


void __RPC_STUB IOleDocumentView_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOleDocumentView_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0144
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPENUMOLEDOCUMENTVIEWS_DEFINED
#define _LPENUMOLEDOCUMENTVIEWS_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0144_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0144_v0_0_s_ifspec;

#ifndef __IEnumOleDocumentViews_INTERFACE_DEFINED__
#define __IEnumOleDocumentViews_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumOleDocumentViews
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEnumOleDocumentViews __RPC_FAR *LPENUMOLEDOCUMENTVIEWS;


EXTERN_C const IID IID_IEnumOleDocumentViews;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumOleDocumentViews : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT __stdcall Next( 
            /* [in] */ ULONG cViews,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cViews) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumOleDocumentViewsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumOleDocumentViews __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumOleDocumentViews __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumOleDocumentViews __RPC_FAR * This);
        
        /* [local] */ HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumOleDocumentViews __RPC_FAR * This,
            /* [in] */ ULONG cViews,
            /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumOleDocumentViews __RPC_FAR * This,
            /* [in] */ ULONG cViews);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumOleDocumentViews __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumOleDocumentViews __RPC_FAR * This,
            /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumOleDocumentViewsVtbl;

    interface IEnumOleDocumentViews
    {
        CONST_VTBL struct IEnumOleDocumentViewsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumOleDocumentViews_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumOleDocumentViews_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumOleDocumentViews_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumOleDocumentViews_Next(This,cViews,rgpView,pcFetched)	\
    (This)->lpVtbl -> Next(This,cViews,rgpView,pcFetched)

#define IEnumOleDocumentViews_Skip(This,cViews)	\
    (This)->lpVtbl -> Skip(This,cViews)

#define IEnumOleDocumentViews_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumOleDocumentViews_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT __stdcall IEnumOleDocumentViews_RemoteNext_Proxy( 
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews,
    /* [length_is][size_is][out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IEnumOleDocumentViews_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumOleDocumentViews_Skip_Proxy( 
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews);


void __RPC_STUB IEnumOleDocumentViews_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumOleDocumentViews_Reset_Proxy( 
    IEnumOleDocumentViews __RPC_FAR * This);


void __RPC_STUB IEnumOleDocumentViews_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumOleDocumentViews_Clone_Proxy( 
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [out] */ IEnumOleDocumentViews __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumOleDocumentViews_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumOleDocumentViews_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0145
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPCONTINUECALLBACK_DEFINED
#define _LPCONTINUECALLBACK_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0145_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0145_v0_0_s_ifspec;

#ifndef __IContinueCallback_INTERFACE_DEFINED__
#define __IContinueCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IContinueCallback
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IContinueCallback __RPC_FAR *LPCONTINUECALLBACK;


EXTERN_C const IID IID_IContinueCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IContinueCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FContinue( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FContinuePrinting( 
            /* [in] */ LONG nCntPrinted,
            /* [in] */ LONG nCurPage,
            /* [unique][in] */ wchar_t __RPC_FAR *pwszPrintStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContinueCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContinueCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContinueCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContinueCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FContinue )( 
            IContinueCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FContinuePrinting )( 
            IContinueCallback __RPC_FAR * This,
            /* [in] */ LONG nCntPrinted,
            /* [in] */ LONG nCurPage,
            /* [unique][in] */ wchar_t __RPC_FAR *pwszPrintStatus);
        
        END_INTERFACE
    } IContinueCallbackVtbl;

    interface IContinueCallback
    {
        CONST_VTBL struct IContinueCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContinueCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContinueCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContinueCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContinueCallback_FContinue(This)	\
    (This)->lpVtbl -> FContinue(This)

#define IContinueCallback_FContinuePrinting(This,nCntPrinted,nCurPage,pwszPrintStatus)	\
    (This)->lpVtbl -> FContinuePrinting(This,nCntPrinted,nCurPage,pwszPrintStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IContinueCallback_FContinue_Proxy( 
    IContinueCallback __RPC_FAR * This);


void __RPC_STUB IContinueCallback_FContinue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IContinueCallback_FContinuePrinting_Proxy( 
    IContinueCallback __RPC_FAR * This,
    /* [in] */ LONG nCntPrinted,
    /* [in] */ LONG nCurPage,
    /* [unique][in] */ wchar_t __RPC_FAR *pwszPrintStatus);


void __RPC_STUB IContinueCallback_FContinuePrinting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IContinueCallback_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0146
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPPRINT_DEFINED
#define _LPPRINT_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0146_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0146_v0_0_s_ifspec;

#ifndef __IPrint_INTERFACE_DEFINED__
#define __IPrint_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPrint
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IPrint __RPC_FAR *LPPRINT;

typedef /* [public] */ 
enum __MIDL_IPrint_0001
    {	PRINTFLAG_MAYBOTHERUSER	= 1,
	PRINTFLAG_PROMPTUSER	= 2,
	PRINTFLAG_USERMAYCHANGEPRINTER	= 4,
	PRINTFLAG_RECOMPOSETODEVICE	= 8,
	PRINTFLAG_DONTACTUALLYPRINT	= 16,
	PRINTFLAG_FORCEPROPERTIES	= 32,
	PRINTFLAG_PRINTTOFILE	= 64
    }	PRINTFLAG;

typedef struct  tagPAGERANGE
    {
    LONG nFromPage;
    LONG nToPage;
    }	PAGERANGE;

typedef struct  tagPAGESET
    {
    ULONG cbStruct;
    BOOL fOddPages;
    BOOL fEvenPages;
    ULONG cPageRange;
    /* [size_is] */ PAGERANGE rgPages[ 1 ];
    }	PAGESET;

#define PAGESET_TOLASTPAGE   ((WORD)(-1L))

EXTERN_C const IID IID_IPrint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPrint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInitialPageNum( 
            /* [in] */ LONG nFirstPage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPageInfo( 
            /* [out] */ LONG __RPC_FAR *pnFirstPage,
            /* [out] */ LONG __RPC_FAR *pcPages) = 0;
        
        virtual /* [local] */ HRESULT __stdcall Print( 
            /* [in] */ DWORD grfFlags,
            /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
            /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *ppPageSet,
            /* [unique][out][in] */ STGMEDIUM __RPC_FAR *pstgmOptions,
            /* [in] */ IContinueCallback __RPC_FAR *pcallback,
            /* [in] */ LONG nFirstPage,
            /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
            /* [out] */ LONG __RPC_FAR *pnLastPage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrintVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPrint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPrint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPrint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInitialPageNum )( 
            IPrint __RPC_FAR * This,
            /* [in] */ LONG nFirstPage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPageInfo )( 
            IPrint __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnFirstPage,
            /* [out] */ LONG __RPC_FAR *pcPages);
        
        /* [local] */ HRESULT ( __stdcall __RPC_FAR *Print )( 
            IPrint __RPC_FAR * This,
            /* [in] */ DWORD grfFlags,
            /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
            /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *ppPageSet,
            /* [unique][out][in] */ STGMEDIUM __RPC_FAR *pstgmOptions,
            /* [in] */ IContinueCallback __RPC_FAR *pcallback,
            /* [in] */ LONG nFirstPage,
            /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
            /* [out] */ LONG __RPC_FAR *pnLastPage);
        
        END_INTERFACE
    } IPrintVtbl;

    interface IPrint
    {
        CONST_VTBL struct IPrintVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrint_SetInitialPageNum(This,nFirstPage)	\
    (This)->lpVtbl -> SetInitialPageNum(This,nFirstPage)

#define IPrint_GetPageInfo(This,pnFirstPage,pcPages)	\
    (This)->lpVtbl -> GetPageInfo(This,pnFirstPage,pcPages)

#define IPrint_Print(This,grfFlags,pptd,ppPageSet,pstgmOptions,pcallback,nFirstPage,pcPagesPrinted,pnLastPage)	\
    (This)->lpVtbl -> Print(This,grfFlags,pptd,ppPageSet,pstgmOptions,pcallback,nFirstPage,pcPagesPrinted,pnLastPage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrint_SetInitialPageNum_Proxy( 
    IPrint __RPC_FAR * This,
    /* [in] */ LONG nFirstPage);


void __RPC_STUB IPrint_SetInitialPageNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrint_GetPageInfo_Proxy( 
    IPrint __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPages);


void __RPC_STUB IPrint_GetPageInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT __stdcall IPrint_RemotePrint_Proxy( 
    IPrint __RPC_FAR * This,
    /* [in] */ DWORD grfFlags,
    /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
    /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *pppageset,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmOptions,
    /* [in] */ IContinueCallback __RPC_FAR *pcallback,
    /* [in] */ LONG nFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
    /* [out] */ LONG __RPC_FAR *pnLastPage);


void __RPC_STUB IPrint_RemotePrint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrint_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0147
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOLECOMMANDTARGET_DEFINED
#define _LPOLECOMMANDTARGET_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0147_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0147_v0_0_s_ifspec;

#ifndef __IOleCommandTarget_INTERFACE_DEFINED__
#define __IOleCommandTarget_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOleCommandTarget
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IOleCommandTarget __RPC_FAR *LPOLECOMMANDTARGET;

typedef /* [public] */ 
enum __MIDL_IOleCommandTarget_0001
    {	OLECMDF_SUPPORTED	= 0x1,
	OLECMDF_ENABLED	= 0x2,
	OLECMDF_LATCHED	= 0x4,
	OLECMDF_NINCHED	= 0x8
    }	OLECMDF;

typedef struct  _tagOLECMD
    {
    ULONG cmdID;
    DWORD cmdf;
    }	OLECMD;

typedef struct  _tagOLECMDTEXT
    {
    DWORD cmdtextf;
    ULONG cwActual;
    ULONG cwBuf;
    /* [size_is] */ wchar_t rgwz[ 1 ];
    }	OLECMDTEXT;

typedef /* [public] */ 
enum __MIDL_IOleCommandTarget_0002
    {	OLECMDTEXTF_NONE	= 0,
	OLECMDTEXTF_NAME	= 1,
	OLECMDTEXTF_STATUS	= 2
    }	OLECMDTEXTF;

typedef /* [public] */ 
enum __MIDL_IOleCommandTarget_0003
    {	OLECMDEXECOPT_DODEFAULT	= 0,
	OLECMDEXECOPT_PROMPTUSER	= 1,
	OLECMDEXECOPT_DONTPROMPTUSER	= 2,
	OLECMDEXECOPT_SHOWHELP	= 3
    }	OLECMDEXECOPT;

/* OLECMDID_STOPDOWNLOAD is supported for QueryStatus Only */
typedef /* [public] */ 
enum __MIDL_IOleCommandTarget_0004
    {	OLECMDID_OPEN	= 1,
	OLECMDID_NEW	= 2,
	OLECMDID_SAVE	= 3,
	OLECMDID_SAVEAS	= 4,
	OLECMDID_SAVECOPYAS	= 5,
	OLECMDID_PRINT	= 6,
	OLECMDID_PRINTPREVIEW	= 7,
	OLECMDID_PAGESETUP	= 8,
	OLECMDID_SPELL	= 9,
	OLECMDID_PROPERTIES	= 10,
	OLECMDID_CUT	= 11,
	OLECMDID_COPY	= 12,
	OLECMDID_PASTE	= 13,
	OLECMDID_PASTESPECIAL	= 14,
	OLECMDID_UNDO	= 15,
	OLECMDID_REDO	= 16,
	OLECMDID_SELECTALL	= 17,
	OLECMDID_CLEARSELECTION	= 18,
	OLECMDID_ZOOM	= 19,
	OLECMDID_GETZOOMRANGE	= 20,
	OLECMDID_UPDATECOMMANDS	= 21,
	OLECMDID_REFRESH	= 22,
	OLECMDID_STOP	= 23,
	OLECMDID_HIDETOOLBARS	= 24,
	OLECMDID_SETPROGRESSMAX	= 25,
	OLECMDID_SETPROGRESSPOS	= 26,
	OLECMDID_SETPROGRESSTEXT	= 27,
	OLECMDID_SETTITLE	= 28,
	OLECMDID_SETDOWNLOADSTATE	= 29,
	OLECMDID_STOPDOWNLOAD	= 30
    }	OLECMDID;

#define OLECMDERR_E_FIRST            (OLE_E_LAST+1)
#define OLECMDERR_E_NOTSUPPORTED (OLECMDERR_E_FIRST)
#define OLECMDERR_E_DISABLED         (OLECMDERR_E_FIRST+1)
#define OLECMDERR_E_NOHELP           (OLECMDERR_E_FIRST+2)
#define OLECMDERR_E_CANCELED         (OLECMDERR_E_FIRST+3)
#define OLECMDERR_E_UNKNOWNGROUP     (OLECMDERR_E_FIRST+4)
#define MSOCMDERR_E_FIRST OLECMDERR_E_FIRST
#define MSOCMDERR_E_NOTSUPPORTED OLECMDERR_E_NOTSUPPORTED
#define MSOCMDERR_E_DISABLED OLECMDERR_E_DISABLED
#define MSOCMDERR_E_NOHELP OLECMDERR_E_NOHELP
#define MSOCMDERR_E_CANCELED OLECMDERR_E_CANCELED
#define MSOCMDERR_E_UNKNOWNGROUP OLECMDERR_E_UNKNOWNGROUP

EXTERN_C const IID IID_IOleCommandTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleCommandTarget : public IUnknown
    {
    public:
        virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Exec( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleCommandTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOleCommandTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOleCommandTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOleCommandTarget __RPC_FAR * This);
        
        /* [input_sync] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryStatus )( 
            IOleCommandTarget __RPC_FAR * This,
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Exec )( 
            IOleCommandTarget __RPC_FAR * This,
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut);
        
        END_INTERFACE
    } IOleCommandTargetVtbl;

    interface IOleCommandTarget
    {
        CONST_VTBL struct IOleCommandTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleCommandTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleCommandTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleCommandTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleCommandTarget_QueryStatus(This,pguidCmdGroup,cCmds,prgCmds,pCmdText)	\
    (This)->lpVtbl -> QueryStatus(This,pguidCmdGroup,cCmds,prgCmds,pCmdText)

#define IOleCommandTarget_Exec(This,pguidCmdGroup,nCmdID,nCmdexecopt,pvaIn,pvaOut)	\
    (This)->lpVtbl -> Exec(This,pguidCmdGroup,nCmdID,nCmdexecopt,pvaIn,pvaOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [input_sync] */ HRESULT STDMETHODCALLTYPE IOleCommandTarget_QueryStatus_Proxy( 
    IOleCommandTarget __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
    /* [in] */ ULONG cCmds,
    /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
    /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText);


void __RPC_STUB IOleCommandTarget_QueryStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOleCommandTarget_Exec_Proxy( 
    IOleCommandTarget __RPC_FAR * This,
    /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
    /* [in] */ DWORD nCmdID,
    /* [in] */ DWORD nCmdexecopt,
    /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
    /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut);


void __RPC_STUB IOleCommandTarget_Exec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOleCommandTarget_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0148
 * at Sun Jul 07 10:43:37 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#endif
typedef enum
{
      OLECMDIDF_REFRESH_NORMAL     = 0,
      OLECMDIDF_REFRESH_IFEXPIRED  = 1,
      OLECMDIDF_REFRESH_CONTINUE   = 2,
      OLECMDIDF_REFRESH_COMPLETELY = 3,
} OLECMDID_REFRESHFLAG;

////////////////////////////////////////////////////////////////////////////
//  Aliases to original office-compatible names
#define IMsoDocument             IOleDocument
#define IMsoDocumentSite         IOleDocumentSite
#define IMsoView                 IOleDocumentView
#define IEnumMsoView             IEnumOleDocumentViews
#define IMsoCommandTarget        IOleCommandTarget
#define LPMSODOCUMENT            LPOLEDOCUMENT
#define LPMSODOCUMENTSITE        LPOLEDOCUMENTSITE
#define LPMSOVIEW                LPOLEDOCUMENTVIEW
#define LPENUMMSOVIEW            LPENUMOLEDOCUMENTVIEWS
#define LPMSOCOMMANDTARGET       LPOLECOMMANDTARGET
#define MSOCMD                   OLECMD
#define MSOCMDTEXT               OLECMDTEXT
#define IID_IMsoDocument         IID_IOleDocument
#define IID_IMsoDocumentSite     IID_IOleDocumentSite
#define IID_IMsoView             IID_IOleDocumentView
#define IID_IEnumMsoView         IID_IEnumOleDocumentViews
#define IID_IMsoCommandTarget    IID_IOleCommandTarget
#define MSOCMDF_SUPPORTED OLECMDF_SUPPORTED
#define MSOCMDF_ENABLED OLECMDF_ENABLED
#define MSOCMDF_LATCHED OLECMDF_LATCHED
#define MSOCMDF_NINCHED OLECMDF_NINCHED
#define MSOCMDTEXTF_NONE OLECMDTEXTF_NONE
#define MSOCMDTEXTF_NAME OLECMDTEXTF_NAME
#define MSOCMDTEXTF_STATUS OLECMDTEXTF_STATUS
#define MSOCMDEXECOPT_DODEFAULT OLECMDEXECOPT_DODEFAULT
#define MSOCMDEXECOPT_PROMPTUSER OLECMDEXECOPT_PROMPTUSER
#define MSOCMDEXECOPT_DONTPROMPTUSER OLECMDEXECOPT_DONTPROMPTUSER
#define MSOCMDEXECOPT_SHOWHELP OLECMDEXECOPT_SHOWHELP
#define MSOCMDID_OPEN OLECMDID_OPEN
#define MSOCMDID_NEW OLECMDID_NEW
#define MSOCMDID_SAVE OLECMDID_SAVE
#define MSOCMDID_SAVEAS OLECMDID_SAVEAS
#define MSOCMDID_SAVECOPYAS OLECMDID_SAVECOPYAS
#define MSOCMDID_PRINT OLECMDID_PRINT
#define MSOCMDID_PRINTPREVIEW OLECMDID_PRINTPREVIEW
#define MSOCMDID_PAGESETUP OLECMDID_PAGESETUP
#define MSOCMDID_SPELL OLECMDID_SPELL
#define MSOCMDID_PROPERTIES OLECMDID_PROPERTIES
#define MSOCMDID_CUT OLECMDID_CUT
#define MSOCMDID_COPY OLECMDID_COPY
#define MSOCMDID_PASTE OLECMDID_PASTE
#define MSOCMDID_PASTESPECIAL OLECMDID_PASTESPECIAL
#define MSOCMDID_UNDO OLECMDID_UNDO
#define MSOCMDID_REDO OLECMDID_REDO
#define MSOCMDID_SELECTALL OLECMDID_SELECTALL
#define MSOCMDID_CLEARSELECTION OLECMDID_CLEARSELECTION
#define MSOCMDID_ZOOM OLECMDID_ZOOM
#define MSOCMDID_GETZOOMRANGE OLECMDID_GETZOOMRANGE
EXTERN_C const GUID SID_SContainerDispatch;


extern RPC_IF_HANDLE __MIDL__intf_0148_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0148_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */


void __RPC_USER UserVARIANT_from_local( VARIANT __RPC_FAR *, UserVARIANT __RPC_FAR * __RPC_FAR * );
void __RPC_USER UserVARIANT_to_local( UserVARIANT __RPC_FAR *, VARIANT __RPC_FAR * );
 void __RPC_USER UserVARIANT_free_inst( UserVARIANT __RPC_FAR * );
void __RPC_USER UserVARIANT_free_local( VARIANT __RPC_FAR * );

/* [local] */ HRESULT __stdcall IEnumOleDocumentViews_Next_Proxy( 
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews,
    /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT __stdcall IEnumOleDocumentViews_Next_Stub( 
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews,
    /* [length_is][size_is][out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
    /* [out] */ ULONG __RPC_FAR *pcFetched);

/* [local] */ HRESULT __stdcall IPrint_Print_Proxy( 
    IPrint __RPC_FAR * This,
    /* [in] */ DWORD grfFlags,
    /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
    /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *ppPageSet,
    /* [unique][out][in] */ STGMEDIUM __RPC_FAR *pstgmOptions,
    /* [in] */ IContinueCallback __RPC_FAR *pcallback,
    /* [in] */ LONG nFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
    /* [out] */ LONG __RPC_FAR *pnLastPage);


/* [call_as] */ HRESULT __stdcall IPrint_Print_Stub( 
    IPrint __RPC_FAR * This,
    /* [in] */ DWORD grfFlags,
    /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
    /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *pppageset,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmOptions,
    /* [in] */ IContinueCallback __RPC_FAR *pcallback,
    /* [in] */ LONG nFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
    /* [out] */ LONG __RPC_FAR *pnLastPage);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
