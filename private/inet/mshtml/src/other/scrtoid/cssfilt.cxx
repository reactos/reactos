//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       cssfilt.cxx
//
//  History:    30-Jan-1998     terrylu     Created
//
//  Contents:   CSSFilter handler implementation
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_HANDIMPL_HXX_
#define X_HANDIMPL_HXX_
#include "handimpl.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include <formsary.hxx>
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include <objsafe.h>
#endif

#ifndef X_OCIDL_H_
#define X_OCIDL_H_
#include <ocidl.h>
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_PEERHAND_HXX_
#define X_PEERHAND_HXX_
#include "peerhand.hxx"
#endif

#ifndef X_PEERDISP_HXX_
#define X_PEERDISP_HXX_
#include "peerdisp.hxx"
#endif

#ifndef X_ATTRBAG_HXX_
#define X_ATTRBAG_HXX_
#include "attrbag.hxx"
#endif

#ifndef X_SCROID_H_
#define X_SCROID_H_
#include "scroid.h"         // For SID_ScriptletDispatch
#endif

#ifndef X_CSSFILT_HXX_
#define X_CSSFILT_HXX_
#include "cssfilt.hxx"
#endif

typedef int (__stdcall *PFNSTRCMP)(LPCTSTR, LPCTSTR);


MtDefine(CCSSFilterHandler, Scriptlet, "CCSSFilterHandler")
MtDefine(CCSSFilterCP_arySinks_pv, CCSSFilterHandler, "CCSSFilterHandler::_arySinks::_pv")


BEGIN_TEAROFF_TABLE(CCSSFilterHandler, ICSSFilter)
    TEAROFF_METHOD(CCSSFilterHandler, SetSite, setsite, (ICSSFilterSite *pSite))
    TEAROFF_METHOD(CCSSFilterHandler, OnAmbientPropertyChange, onambientpropertychange, (DISPID dispid))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CCSSFilterHandler, IObjectSafety)
    TEAROFF_METHOD(CCSSFilterHandler, GetInterfaceSafetyOptions, getinterfacesafetyoptions, (REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions))
    TEAROFF_METHOD(CCSSFilterHandler, SetInterfaceSafetyOptions, setinterfacesafetyoptions, (REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CCSSFilterHandler, IConnectionPointContainer)
    TEAROFF_METHOD(CCSSFilterHandler, EnumConnectionPoints, enumconnectionpoints, (IEnumConnectionPoints **ppEnum))
    TEAROFF_METHOD(CCSSFilterHandler, FindConnectionPoint, findconnectionpoint, (REFIID riid, IConnectionPoint **ppCP))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CCSSFilterHandler, IPersistPropertyBag2)
    TEAROFF_METHOD(CCSSFilterHandler, GetClassID, getclassid, (CLSID *pclsID))
    TEAROFF_METHOD(CCSSFilterHandler, InitNew, initnew, (void))
    TEAROFF_METHOD(CCSSFilterHandler, Load, load, (IPropertyBag2  *pPropBag, IErrorLog *pErrLog))
    TEAROFF_METHOD(CCSSFilterHandler, Save, save, (IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties))
    TEAROFF_METHOD(CCSSFilterHandler, IsDirty, isdirty, (void))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CCSSFilterHandler, IScriptletHandler)
	TEAROFF_METHOD(CCSSFilterHandler, GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk))
	TEAROFF_METHOD(CCSSFilterHandler, SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CCSSFilterHandler, IScriptletHandlerConstructor)
    TEAROFF_METHOD(CCSSFilterHandler, Load, load, (WORD wFlags, IScriptletXML *pxmlElement))
	TEAROFF_METHOD(CCSSFilterHandler, Create, create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler))
	TEAROFF_METHOD(CCSSFilterHandler, Register, register, (LPCOLESTR pstrPath))
	TEAROFF_METHOD(CCSSFilterHandler, Unregister, unregister, ())
	TEAROFF_METHOD(CCSSFilterHandler, AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass))
END_TEAROFF_TABLE()


const CBase::CLASSDESC CCSSFilterHandler::s_classdesc =
{
    &CLSID_CCSSFilterHandler,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+------------------------------------------------------------------------
//
//  Member:     CreatePeerHandler
//
//  Synopsis:   Creates a new Scriptoid Peer handler instance.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//-------------------------------------------------------------------------

HRESULT
CreateFilterHandler(IUnknown *pUnkContext, IUnknown * pUnkOuter, IUnknown **ppUnk)
{
    HRESULT             hr;
    CCSSFilterHandler  *pFilter;

    *ppUnk = NULL;

    pFilter = new CCSSFilterHandler(pUnkContext, pUnkOuter);
    if (pFilter == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pFilter->Init();
    if (!hr)
    {
        *ppUnk = (IUnknown *)pFilter;
    }

Cleanup:    
    return hr;
}


// Refcounting schema for this handler:
//
// CCSSFilterHandler holds a ref & ptr to CCSSFilterCP (if it exists).
// It's expected that clients will ref some combination of
// CCSSFilterHandler and/or CCSSFilterCP (for e.g. Trident currently refs
// only the main control and not any connection points -- it always
// QI's for ICPC and then ICP when it wants a connection pt.  However,
// we want to allow clients to keep a ref on a ICP and still work).
//
// If the client advises on us, CCSSFilterCP will hold a ref on the advise
// sink.  It is up to the client determine when it wants to break
// this circularity -- recommend the client hand CCSSFilterCP an advise sink
// with an independant ref count.  

CCSSFilterHandler::CCSSFilterHandler(IUnknown *pUnkContext, IUnknown * pUnkOuter) :
 _pUnkOuter(pUnkOuter),
 _pElement(NULL),
 _pFilterSite(NULL),
 _pWinDispCP(NULL),
 _dwDispCookie(0),
 _pDispSink(NULL),
 _pScript(NULL),
 _pAttrBag(NULL),
 _pCP(NULL),
 _pCSSDisp(NULL)

{
    return;
}

CCSSFilterHandler::~CCSSFilterHandler()
{
}


void
CCSSFilterHandler::Passivate()
{
    if ( _pCP )
    {
        _pCP->Detach();
        _pCP = NULL;
    }

    if ( _pAttrBag )
    {
        _pAttrBag->Release();
        _pAttrBag = NULL;
    }

    Assert ( _pScript == NULL );
    Assert ( _pElement == NULL );
    Assert ( _pDispSink == NULL );
    Assert ( _pWinDispCP == NULL );
    Assert ( _dwDispCookie == 0 );

    super::Passivate();
}


HRESULT
CCSSFilterHandler::Init()
{
    return S_OK;
}


STDMETHODIMP
CCSSFilterHandler::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IScriptletHandler, _pUnkOuter)
    QI_TEAROFF(this, IScriptletHandlerConstructor, _pUnkOuter)
    QI_TEAROFF(this, ICSSFilter, _pUnkOuter)
    QI_TEAROFF(this, IConnectionPointContainer, _pUnkOuter)
    QI_TEAROFF(this, IPersistPropertyBag2, _pUnkOuter)
    QI_TEAROFF(this, IObjectSafety, _pUnkOuter)
    default:
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

/**** CSSFilterHandler IScriptletHandler Implementation ****/


STDMETHODIMP
CCSSFilterHandler::GetNameSpaceObject(IUnknown **ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    if (!_pCSSDisp)
    {
        // Keep weak-ref in _pCSSDisp, object life time is controlled by caller.
        _pCSSDisp = (IDispatch *) (new CCSSFilterIntDispatch(this));
        if (!_pCSSDisp)
            return E_OUTOFMEMORY;
    }
    else
    {
        _pCSSDisp->AddRef();
    }

    *ppUnk = (IUnknown *)_pCSSDisp;
    return S_OK;
}


STDMETHODIMP
CCSSFilterHandler::SetScriptNameSpace(IUnknown *punkNameSpace)
{
    return S_OK;
}


/**** CSSFilterHandler IScriptletHandlerConstructor Implementation ****/

STDMETHODIMP
CCSSFilterHandler::Create(IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler)
{
    Assert(_pUnkOuter == punkOuter);

    RRETURN(PrivateQueryInterface(IID_IScriptletHandler, (void **)ppunkHandler));
}

/**** CSSFilterHandler IConnectionPointContainer Implementation ****/

STDMETHODIMP
CCSSFilterHandler::EnumConnectionPoints( IEnumConnectionPoints **ppEnum)
{
    // return E_NOTIMPL since we only support 1 interface via connection points
    *ppEnum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CCSSFilterHandler::FindConnectionPoint( REFIID riid, IConnectionPoint **ppCP)
{
    
    if ( !ppCP )
        return E_POINTER;
        
    if ( riid == IID_IDispatch )
    {
        if ( !_pCP )
            _pCP = new CCSSFilterCP(this);       // CCSSFilterCP ref == 1 (we hold a ref)

        *ppCP = (IConnectionPoint*)_pCP;

        if ( _pCP )
        {
            _pCP->AddRef();
            return S_OK;
        }
    }
    
    *ppCP = NULL;
    return CONNECT_E_NOCONNECTION;
}

/**** CSSFilterHandler ICSSFilter Implementation ****/

STDMETHODIMP
CCSSFilterHandler::SetSite( ICSSFilterSite *pSite )
{
    HRESULT hr = S_OK;
    
    if ( _pFilterSite )
        _pFilterSite->Release();

/*
    if ( m_pScriptSite )
    {
        m_pScriptSite->Stop();
        m_pScriptSite->Release();
        m_pScriptSite = NULL;
    }
*/

    _pFilterSite = pSite;

    if ( _pFilterSite )
    {
        _pFilterSite->AddRef();
        hr = RunFilterCode();
    }

    return hr;
}

// TODO: Maybe break this fn up a little?
STDMETHODIMP
CCSSFilterHandler::RunFilterCode()
{
    IDispatch *pDispDoc2 = NULL;
    IHTMLDocument2 *pDoc2 = NULL;
    IDispatch *pWin2 = NULL;
    IConnectionPointContainer *pCPC = NULL;
    HRESULT hr = S_OK;

    IServiceProvider        *pSP = NULL;
    IActiveScript           *pAS = NULL;

    LPOLESTR    szMethod = _T("OnFilterLoad");
    LPOLESTR   *pStrMethod = &szMethod;
    DISPID      dispid = 0;
    DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
    VARIANT     varResult;


    Assert( _pFilterSite );
    Assert( !_pElement );
    Assert( !_pWinDispCP );
    Assert( !_pDispSink );
    Assert( !_dwDispCookie );

    // Hookup to Trident so when it fires the unload event, we can
    // stop the filter.  We go through element->doc->scriptcollection
    // to get to a ConnectionPointContainer, and pass Trident an IDispatch
    // sink for it to call us when the element unloads the filter.
    // 
    
    // We hang onto the element object so we can give the script
    // access to it.
    hr = _pFilterSite->GetElement ( &_pElement );
    if ( hr )
        goto Error;

    hr = _pElement->get_document ( &pDispDoc2 );
    if ( hr )
        goto Error;

    hr = pDispDoc2->QueryInterface ( IID_IHTMLDocument2, (void**)&pDoc2 );
    if ( hr )
        goto Error;

    hr = pDoc2->get_Script( &pWin2 );
    if ( hr || !pWin2 )
        goto Error;

    hr = pWin2->QueryInterface( IID_IConnectionPointContainer, (void**)&pCPC );
    if ( hr )
        goto Error;

    hr = pCPC->FindConnectionPoint( IID_IDispatch, &_pWinDispCP );
    if ( hr )
        goto Error;

    _pDispSink = new CCSSFilterDispatchSink( this );
    if ( !_pDispSink )
        goto Error;

    hr = _pWinDispCP->Advise( (IUnknown *)_pDispSink, &_dwDispCookie );
    if ( hr )
        goto Error;

    Assert( _dwDispCookie );

    // Now run the actual script code associated with the filter.
    // We get the script engine from the outer, inject some namespace
    // ("element" representing the element the filter operates on and
    // "filter" which contains the params of the filter) and call
    // "OnFilterLoad" in the script file.
    
    // We better have an outer, otherwise architecture is screwed up..
    Assert ( _pUnkOuter );
    Assert ( !_pScript );

    hr = _pUnkOuter->QueryInterface( IID_IServiceProvider, (void **)&pSP );
    if ( hr )
        goto Error;

    hr = pSP->QueryService( SID_ScriptletDispatch, IID_IDispatch, (void **)&_pScript );
    if ( hr )
        goto Error;
       
    hr = _pScript->GetIDsOfNames(IID_NULL, pStrMethod, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if ( hr )
        goto Error;

    VariantInit(&varResult);
    hr = _pScript->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &dispParams, &varResult, NULL, NULL);
    if (hr)
        goto Error;

Cleanup:
    if ( pCPC )
        pCPC->Release();
    if ( pWin2 )
        pWin2->Release();
    if ( pDoc2 )
        pDoc2->Release();
    if ( pDispDoc2 )
        pDispDoc2->Release();
    if ( pSP )
        pSP->Release();

    return hr;

Error:
    if ( _pElement )
    {
        _pElement->Release();
        _pElement = NULL;
    }
    
    if ( _pWinDispCP )
    {
        if ( _pDispSink )
        {
            if ( _dwDispCookie )
            {
                _pWinDispCP->Unadvise( _dwDispCookie );
                _dwDispCookie = 0;
            }
            _pDispSink->Detach();
            _pDispSink = NULL;
        }
        _pWinDispCP->Release();
        _pWinDispCP = NULL;
    }

    if ( _pScript )
    {
        _pScript->Release();
        _pScript = NULL;
    }

    goto Cleanup;
}

void
CCSSFilterHandler::UnloadFilter()
{   
    // No recovery from errors; we're going away, so just do the best
    // we can (that's why we ignore return codes in ship code)
    HRESULT hr = S_OK;
    
    // Trident just told us to go away, so we'll unadvise Trident..
    Assert( _pWinDispCP );
    Assert( _dwDispCookie );
    Assert( _pDispSink );

    _pDispSink->Detach();
    _pDispSink = NULL;

    hr = _pWinDispCP->Unadvise( _dwDispCookie );
    Assert( SUCCEEDED(hr) );

    _pWinDispCP->Release();
    _pWinDispCP = NULL;
    _dwDispCookie = NULL;

    // Call script's "OnFilterUnload" so the script can clean
    // up what it did..  

    Assert( _pScript );
    
    LPOLESTR    szMethod = _T("OnFilterUnload");
    LPOLESTR   *pStrMethod = &szMethod;
    DISPID      dispid = 0;
    DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
    VARIANT     varResult;

    hr = _pScript->GetIDsOfNames(IID_NULL, pStrMethod, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    Assert( SUCCEEDED(hr) );

    VariantInit(&varResult);
    hr = _pScript->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &dispParams, &varResult, NULL, NULL);
    Assert( SUCCEEDED(hr) );

    _pScript->Release();
    _pScript = NULL;

    // Release our ref on the element the filter is acting on
    Assert( _pElement );
    _pElement->Release();
    _pElement = NULL;
}

/**** CSSFilterHandler IPersist Implementation ****/

STDMETHODIMP
CCSSFilterHandler::GetClassID( CLSID *pclsID )
{
    if ( !pclsID )
        return E_POINTER;
        
    *pclsID = CLSID_CCSSFilterHandler;
    return S_OK;
}

/**** CSSFilterHandler IPersistPropertyBag2 Implementation ****/

STDMETHODIMP
CCSSFilterHandler::Load( IPropertyBag2 *pPropBag,
                         IErrorLog *pErrLog)
{
    HRESULT hr;
    IPropertyBag *pBag = NULL;
    IPropertyBag2 *pBag2 = NULL;

    if ( _pAttrBag )
        _pAttrBag->Release();

    _pAttrBag = new CAttrBag();
    if ( !_pAttrBag )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    
    hr = pPropBag->QueryInterface ( IID_IPropertyBag, (void**)&pBag );
    if ( hr )
        goto Error;

    // BUGBUG: Shouldn't need to do this GPF if I don't
    hr = pPropBag->QueryInterface ( IID_IPropertyBag2, (void**)&pBag2 );
    if ( hr )
        goto Error;

    hr = _pAttrBag->Load ( pBag, pBag2 );
    if ( hr )
        goto Error;
    
Cleanup:
    if ( pBag )
        pBag->Release();
    if ( pBag2 )
        pBag2->Release();
    return hr;
    
Error:
    if ( _pAttrBag )
    {
        _pAttrBag->Release();
        _pAttrBag = NULL;
    }
        
    goto Cleanup;
}

/**** CSSFilterHandler IObjectSafety Implementation ****/

STDMETHODIMP
CCSSFilterHandler::GetInterfaceSafetyOptions(
        REFIID riid,                                    // Interface that we want options for
        DWORD *pdwSupportedOptions,         // Options meaningful on this interface
        DWORD *pdwEnabledOptions)               // current option values on this interface
{
    if ( riid == IID_IPersistPropertyBag ||
         riid == IID_IPersistPropertyBag2 )
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP
CCSSFilterHandler::SetInterfaceSafetyOptions(
        REFIID riid,                            // Interface to set options for
        DWORD dwOptionSetMask,          // Options to change
        DWORD dwEnabledOptions)     // New option values
{
    if ( riid == IID_IDispatch )
        return S_OK;
    else
        return S_FALSE;
}


/****                                               ****/
/****                                               ****/
/**** CSSFilterCP Implementation                    ****/
/****                                               ****/
/****                                               ****/

CCSSFilterCP::CCSSFilterCP( CCSSFilterHandler *pContainer ) :
 _cRef( 1 ),
 _pContainer( pContainer )
{
    Assert( pContainer );
    return;
}

CCSSFilterCP::~CCSSFilterCP()
{
    // Check that we have no connection points still advised..
    // If we do, then a client forgot to unadvise us --> we'll
    // Assert in debug, but always release them..
    int i;
    IDispatch *pSink = NULL;
    for ( i=0 ; i < _arySinks.Size() ; ++i )
    {
        pSink = _arySinks[i];
        if ( pSink )
        {
            Assert( FALSE && "Client forgot to unadvise!");
            pSink->Release();
        }
    }
    
    return;
}

void CCSSFilterCP::Detach()
{
    _pContainer = NULL;
    Release();
}

STDMETHODIMP
CCSSFilterCP::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IConnectionPoint) {
        *ppv = (IConnectionPoint*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CCSSFilterCP::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG)
CCSSFilterCP::Release() 
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }
    
    return _cRef;
}

STDMETHODIMP
CCSSFilterCP::GetConnectionInterface( IID *pIID )
{
    if ( !pIID )
        return E_POINTER;

    *pIID = IID_IDispatch;
    return S_OK;
}

STDMETHODIMP
CCSSFilterCP::GetConnectionPointContainer( IConnectionPointContainer **ppCPC)
{
    return ( _pContainer ?
             _pContainer->PrivateQueryInterface( IID_IConnectionPointContainer, (void**)ppCPC ) :
             E_UNEXPECTED );
}

STDMETHODIMP
CCSSFilterCP::Advise( IUnknown *pUnkSink, DWORD *pdwCookie )
{
    // NOTE: pUnkSink doesn't need to be addref'ed since
    // we don't hang onto it..

    HRESULT hr;
    
    if ( !pUnkSink || !pdwCookie )
        return E_POINTER;

    // Return cookie of 0 on failure.
    *pdwCookie = 0;

    IDispatch *pSink = NULL;
    hr = pUnkSink->QueryInterface(IID_IDispatch, (void**)&pSink);
    if ( FAILED(hr) )
        return CONNECT_E_CANNOTCONNECT;

    int i = _arySinks.Size(); 
    if ( !_arySinks.EnsureSize( i+1 ) )
        return E_OUTOFMEMORY;
    if ( !_arySinks.Insert(i, pSink) )
        return E_FAIL;

    // Docs claim that returning a cookie value of 0
    // implies failure, so we can't use the index directly. 
    *pdwCookie = i+1;
    return S_OK;
}

STDMETHODIMP
CCSSFilterCP::Unadvise( DWORD dwCookie )
{
    // 0 is an invalid connection
    if ( dwCookie == 0 )
        return CONNECT_E_NOCONNECTION;
        
    IDispatch *pSink = NULL;
    pSink = _arySinks[dwCookie-1];
    if (pSink)
    {
        Assert( pSink );
        pSink->Release();
        pSink = NULL;
        _arySinks.Insert( dwCookie-1, pSink );
        return S_OK;
    }
    
    return CONNECT_E_NOCONNECTION;
}

STDMETHODIMP
CCSSFilterCP::EnumConnections( IEnumConnections **ppEnum )
{
    if ( !ppEnum )
        return E_POINTER;

    *ppEnum = NULL;
    return E_NOTIMPL;
}

/****                                               ****/
/****                                               ****/
/**** CSSFilterDispatchSink Implementation          ****/
/****                                               ****/
/****                                               ****/

CCSSFilterDispatchSink::CCSSFilterDispatchSink(CCSSFilterHandler *_pCSSFilterHandler) :
 _cRef(1),
 _pCSSFilterHandler(_pCSSFilterHandler)
{
    Assert( _pCSSFilterHandler );
}

CCSSFilterDispatchSink::~CCSSFilterDispatchSink()
{
     return;
}

void
CCSSFilterDispatchSink::Detach()
{
    _pCSSFilterHandler = NULL;
    Release();
}

STDMETHODIMP CCSSFilterDispatchSink::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IDispatch) {
        *ppv = (IDispatch*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCSSFilterDispatchSink::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CCSSFilterDispatchSink::Release() 
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }
    
    return _cRef;
}

/*
STDMETHODIMP CCSSFilterDispatchSink::GetTypeInfoCount(UINT *pctinfo)
{
}

STDMETHODIMP CCSSFilterDispatchSink::GetTypeInfo(UINT iTInfo, 
                                            LCID lcid,
                                            ITypeInfo **ppTInfo)
{
}
*/    

STDMETHODIMP CCSSFilterDispatchSink::GetIDsOfNames(REFIID riid , LPOLESTR *rgszNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *rgDispId)
{
    Assert( rgDispId );

    rgDispId[0] = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}
    
STDMETHODIMP CCSSFilterDispatchSink::Invoke( 
                     /* [in] */ DISPID dispIdMember,
                     /* [in] */ REFIID /* riid */,
                     /* [in] */ LCID /* lcid */,
                     /* [in] */ WORD wFlags,
                     /* [out][in] */ DISPPARAMS *pDispParams,
                     /* [out] */ VARIANT * /* pVarResult */,
                     /* [out] */ EXCEPINFO * /* pExcepInfo */,
                     /* [out] */ UINT * /* puArgErr */)
{
    switch ( dispIdMember )
    {
        case DISPID_HTMLWINDOWEVENTS_ONUNLOAD:
            _pCSSFilterHandler->UnloadFilter();
            break;
        default:
            break;
    }
    return S_OK;
}

/****                                               ****/
/****                                               ****/
/**** CSSFilterIntDispatch Implementation           ****/
/****                                               ****/
/****                                               ****/

CCSSFilterIntDispatch::CCSSFilterIntDispatch(CCSSFilterHandler *_pCSSFilterHandler) :
 _cRef(1),
 _pCSSFilterHandler(_pCSSFilterHandler)
{
    Assert( _pCSSFilterHandler );
}

CCSSFilterIntDispatch::~CCSSFilterIntDispatch()
{
     return;
}

STDMETHODIMP CCSSFilterIntDispatch::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if ( riid==IID_IUnknown || riid==IID_IDispatch || riid==IID_IDispatchEx ) {
        *ppv = (IDispatchEx*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCSSFilterIntDispatch::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CCSSFilterIntDispatch::Release() 
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }
    
    return _cRef;
}

/*
STDMETHODIMP CCSSFilterIntDispatch::GetTypeInfoCount(UINT *pctinfo)
{
}

STDMETHODIMP CCSSFilterIntDispatch::GetTypeInfo(UINT iTInfo, 
                                            LCID lcid,
                                            ITypeInfo **ppTInfo)
{
}
*/    

STDMETHODIMP
CCSSFilterIntDispatch::GetIDsOfNames(REFIID riid , LPOLESTR *rgszNames,
                                UINT cNames, LCID lcid,
                                DISPID *rgDispId)
{
    HRESULT hr;
    
    Assert(cNames == 1);

    if ( !rgszNames || !rgDispId )
        return E_POINTER;

    BSTR bstrName = SysAllocString( rgszNames[0] );
    if ( bstrName )
    {
        hr = GetDispID( bstrName, 0, rgDispId );
        SysFreeString( bstrName );
    }
    else 
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
    
STDMETHODIMP
CCSSFilterIntDispatch::Invoke( /* [in] */ DISPID dispIdMember,
                     /* [in] */ REFIID /* riid */,
                     /* [in] */ LCID lcid,
                     /* [in] */ WORD wFlags,
                     /* [out][in] */ DISPPARAMS *pDispParams,
                     /* [out] */ VARIANT * pVarResult,
                     /* [out] */ EXCEPINFO *pExcepInfo,
                     /* [out] */ UINT * /* puArgErr */)
{
    return InvokeEx( dispIdMember, lcid, wFlags,
                     pDispParams, pVarResult, pExcepInfo, NULL );
}

STDMETHODIMP
CCSSFilterIntDispatch::GetDispID( BSTR bstrName, DWORD grfdex, DISPID *pid )
{
    HRESULT hr = S_OK;
    STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    if ( !pid )
        return E_POINTER;

    // BUGBUG: Allocate some real dispid's instead of faking..

    if ( !pfnStrCmp(_T("element"), bstrName) )
    {
        *pid = DISPID_CSSFILTERHANDLER_ELEMENT;
    }
    else if ( !pfnStrCmp(_T("params"), bstrName) )
    {
        *pid = DISPID_CSSFILTERHANDLER_PARAMS;
    }
    else
    {
        *pid = DISPID_UNKNOWN;
        hr = DISP_E_UNKNOWNNAME;
    }

    return hr;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::InvokeEx( DISPID id, LCID lcid, WORD wFlags,
                      DISPPARAMS *pdp, VARIANT *pvarRes,
                      EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HRESULT hr = S_OK;

    // BUGBUG: Even more robust checking?  Add proper support for
    // various dispids.  Fill puArgErr.

    if ( id == DISPID_CSSFILTERHANDLER_ELEMENT )
    {
        if ( !(wFlags & DISPATCH_PROPERTYGET) ) {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        if ( pdp->cArgs != 0 ) {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }
        Assert( _pCSSFilterHandler );
        if ( !(_pCSSFilterHandler->_pElement) )
        {
            // Scriptoid script tried to access element before
            // SetSite was called (i.e. outside of OnFilterxxx fns)
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        Assert( pvarRes );
        V_VT(pvarRes) = VT_DISPATCH;
        hr = _pCSSFilterHandler->_pElement->QueryInterface( IID_IDispatch, (void**)&(V_DISPATCH(pvarRes)) );
    }
    else if ( id == DISPID_CSSFILTERHANDLER_PARAMS )
    {
        if ( !(wFlags & DISPATCH_PROPERTYGET) ) {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        if ( pdp->cArgs != 0 ) {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }
        Assert( _pCSSFilterHandler );
        if ( !(_pCSSFilterHandler->_pAttrBag) )
        {
            // Scriptoid script tried to access element before
            // Load was called (i.e. outside of OnFilterxxx fns)
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        Assert( pvarRes );
        V_VT(pvarRes) = VT_DISPATCH;
        hr = _pCSSFilterHandler->_pAttrBag->QueryInterface( IID_IDispatch, (void**)&(V_DISPATCH(pvarRes)) );
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

Cleanup:
    return hr;
}

STDMETHODIMP
CCSSFilterIntDispatch::DeleteMemberByName( /* [in] */ BSTR bstr,
                                /* [in] */ DWORD grfdex)
{
    return S_OK;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::DeleteMemberByDispID( /* [in] */ DISPID id)
{
    return S_OK;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::GetMemberProperties( /* [in] */ DISPID id,
                                 /* [in] */ DWORD grfdexFetch,
                                 /* [out] */ DWORD *pgrfdex )
{
    HRESULT hr;

    if ( !pgrfdex )
        return E_POINTER;

    switch ( id )
    {
        case DISPID_CSSFILTERHANDLER_ELEMENT:
        case DISPID_CSSFILTERHANDLER_PARAMS:
            *pgrfdex =  fdexPropCanGet |
                        fdexPropCannotPut |
                        fdexPropCannotPutRef |
                        fdexPropNoSideEffects |
                        fdexPropCannotCall;
            hr = S_OK;
            break;
        default:
            *pgrfdex = grfdexPropCannotAll;
            hr = DISP_E_MEMBERNOTFOUND;
    }

    *pgrfdex &= grfdexFetch;

/*
BUGBUG: Need to handle the following.

fdexPropCanGet
fdexPropCannotGet
fdexPropCanPut
fdexPropCannotPut
fdexPropCanPutRef
fdexPropCannotPutRef
fdexPropNoSideEffects
fdexPropDynamicType
fdexPropCanCall
fdexPropCannotCall
fdexPropCanConstruct
fdexPropCannotConstruct
fdexPropCanSourceEvents
fdexPropCannotSourceEvents
*/

    return hr;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::GetMemberName( /* [in] */ DISPID id,
                           /* [out] */ BSTR *pbstrName)
{
    // Caller will free pbstrName
    HRESULT hr = S_OK;
    
    if ( !pbstrName )
        return E_POINTER;
        
    switch ( id )
    {
        case DISPID_CSSFILTERHANDLER_ELEMENT:
            *pbstrName = SysAllocString(_T("element"));
            break;
        case DISPID_CSSFILTERHANDLER_PARAMS:
            *pbstrName = SysAllocString(_T("params"));
            break;
        default:
            *pbstrName = NULL;
            hr = DISP_E_MEMBERNOTFOUND;
    }
    
    return hr;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::GetNextDispID( /* [in] */ DWORD grfdex,
                                  /* [in] */ DISPID id,
                                  /* [out] */ DISPID *pid)
{
    if ( !pid )
        return E_POINTER;

    // BUGBUG: Behave the same way for all values of grfdex
    // (fdexEnumDefault and fdexEnumAll)

    // Allow -ve dispids to also function as "start enum"
    if ( id == DISPID_STARTENUM || id < 0 )
    {
        *pid = DISPID_CSSFILTERHANDLER_ELEMENT;
        return S_OK;
    }
    // TODO: allocate a range of dispids and iterate.
    else if ( id == DISPID_CSSFILTERHANDLER_ELEMENT )
    {
        *pid = DISPID_CSSFILTERHANDLER_PARAMS;
        return S_OK;
    }

    return S_FALSE;
}
        
STDMETHODIMP
CCSSFilterIntDispatch::GetNameSpaceParent( /* [out] */ IUnknown **ppunk)
{
    if ( !ppunk )
        return E_POINTER;

    *ppunk = NULL;
    return S_OK;
}
