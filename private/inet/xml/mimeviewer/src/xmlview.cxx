/*
 * @(#)Viewer.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

 
#include "core.hxx"
#pragma hdrstop

#include <windows.h>
#ifndef UNIX
#include <comdef.h>
#endif // UNIX
#include <objsafe.h>        // IObjectSafety    
#include <docobj.h>         // IOleCommandTarget
#include <advpub.h>
#include <stdio.h>
#include <urlmon.h>
#include <perhist.h>
#include <mshtml.h>

#include <shlguid.h>
#include <shlguidp.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <mshtmhst.h>
#include <mshtmcid.h>
#include <windowsx.h>

#include <msxml.h>
#include <commdlg.h>
#include <shlwapi.h>

#include "xml/dll/resource.h"
#include "utils.hxx"
#ifdef BUILD_AS_DLL
#include "staticunknown.hxx"
#endif // BUILD_AS_DLL
#include "viewerfactory.hxx"
#include "bufferedstream.hxx"
#include "behaviour.hxx"
#include "callback.hxx"
#include "xmlview.hxx"
#include "expando.hxx"

// {48123BC4-99D9-11d1-A6B3-00C04FD91555}
EXTERN_C const CLSID CLSID_Viewer = 
{0x48123bc4,0x99d9,0x11d1,{0xa6,0xb3,0x0,0xc0,0x4f,0xd9,0x15,0x55}};

// {4419DD31-28A5-11d2-AE08-0080C7337EA1}
EXTERN_C const CLSID CLSID_PeerFactory = 
{ 0x4419dd31, 0x28a5, 0x11d2, { 0xae, 0x8, 0x0, 0x80, 0xc7, 0x33, 0x7e, 0xa1 } };

// {7E3FCEA1-31B4-11d2-AE1F-0080C7337EA1}
EXTERN_C const CLSID CLSID_BufferedMoniker = 
{ 0x7e3fcea1, 0x31b4, 0x11d2, { 0xae, 0x1f, 0x0, 0x80, 0xc7, 0x33, 0x7e, 0xa1 } };

// {259EF953-37A5-11d2-AE30-0080C7337EA1}
EXTERN_C const CLSID CLSID_ViewerPeer = 
{ 0x259ef953, 0x37a5, 0x11d2, { 0xae, 0x30, 0x0, 0x80, 0xc7, 0x33, 0x7e, 0xa1 } };

// {CC0BAF51-3321-11d2-AE28-0080C7337EA1}
EXTERN_C const IID IID_IXMLViewerIdentity = 
{ 0xcc0baf51, 0x3321, 0x11d2, { 0xae, 0x28, 0x0, 0x80, 0xc7, 0x33, 0x7e, 0xa1 } };

// {620CF7D1-3922-11d2-AE35-0080C7337EA1}
EXTERN_C const IID IID_IXMLViewerScriptMask = 
{ 0x620cf7d1, 0x3922, 0x11d2, { 0xae, 0x35, 0x0, 0x80, 0xc7, 0x33, 0x7e, 0xa1 } };

EXTERN_C const CLSID CLSID_HTMLDocument;

extern HRESULT CreateStreamOnFile(LPCTSTR lpstrFile, DWORD dwSTGM, IStream **ppstrm);


#if MIMEASYNC
long Viewer::s_objCount = 0;
#endif

//======================================================================
Viewer::Viewer()
{
    _ulRefs = 1;
    _pTrident = NULL;
    _pDocument = NULL;
    _pimkCurrent = NULL;
    _pBinding = NULL;

#if MIMEASYNC
    InterlockedIncrement(&s_objCount);
    _evtLoadComplete = NULL;
#endif
    IncrementComponents();
}


Viewer::~Viewer()
{
    cleanupBinding();

    // okay now for some trident magic.  Trident is going to passivate the
    // tree, deleting it, then will call and add-ref release pair through
    // the delegatingQI.  On that release we may invoke the destructor again
    // so we have to notice if we are being recursed into

    SafeRelease(_pDocument);
    SafeRelease(_pimkCurrent);
    SafeRelease(_pTrident);

    // BUGBUG right now for some reason the shell hangs if we run xsl script
    // and the download thread is around. Our DllCanUnloadNow never gets called
    // So we have to destroy the thread manually ourselves. Do this when we
    // know there are no more .xml files to browse
    // the download kill will wait for all of the outstanding parsers on the other
    // thread to complete, to avoid any race conditions
#if MIMEASYNC
    if (_evtLoadComplete)
    {
        CloseHandle(_evtLoadComplete);
        _evtLoadComplete = NULL;
    }
    Assert(s_objCount != 0);
    InterlockedDecrement(&s_objCount);
    if (s_objCount == 0)
        KillMimeDownloadThread();
#endif
    DecrementComponents();
}    


HRESULT STDMETHODCALLTYPE
Viewer::QueryInterface(REFIID riid,void ** ppv)
{
    HRESULT hr;

    if (riid == IID_IUnknown) 
        *ppv = this;
    else if (riid == IID_IPersistMoniker)
        *ppv = static_cast<IPersistMoniker*>(this);
    else if (riid == IID_IPersistHistory)
        *ppv = static_cast<IPersistHistory*>(this);
    else if (riid == IID_IXMLViewerIdentity)
        *ppv = static_cast<IXMLViewerIdentity*>(this);
    else if (riid == IID_IXMLViewerScriptMask)
        *ppv = static_cast<IXMLViewerScriptMask*>(this);
    else if (riid == IID_IOleCommandTarget)
        *ppv = static_cast<IOleCommandTarget*>(this);
    else 
    {
        if (riid == IID_IPersistStreamInit)
        {
            // need to use _pDocument to load the XML...
        }
        else if (riid == IID_IObjectWithSite)
        {
            // need to pass through to _pDocument also.
        }
        else if (riid == IID_IObjectSafety)
        {
            // need to pass through to _pDocument also.
        }
        else if ((riid == IID_IHTMLDocument) || (riid == IID_IHTMLDocument2) || (riid == IID_IHTMLDocument3))
        {
            // intercept events here
        }
        return hr = _pTrident->QueryInterface(riid, ppv);
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}



ULONG STDMETHODCALLTYPE
Viewer::AddRef() 
{
    return InterlockedIncrement(&_ulRefs);
}

ULONG STDMETHODCALLTYPE 
Viewer::Release()
{
    // BUGBUG Normally we call into msxml.dll through COM interfaces and thus do not need this
    // This is one case where we call directly to low-level services (component counting)
    // since we are part of the DLL.
    // Hence STACK_ENTRY must be used here. We really should put this around all calls from the outside
    // or hide this from the viewer (by putting in DecrementComponents)
    STACK_ENTRY;
    if (_ulRefs == 1)
    {
        // okay now for some trident magic.  Trident is going to passivate the
        // tree, deleting it, then will call and add-ref release pair through
        // the delegatingQI.  On that release we don't want to invoke the destructor 
        // again so we do it here
        AddRef();

        // remove the expando documents - because they now contain _pSite pointers that    
        // that point to _pTrident - and we need to release them before we release _pTrident.
        IHTMLDocument2* pIHTML = NULL;

        if (null != _pTrident && 
            SUCCEEDED(_pTrident->QueryInterface(IID_IHTMLDocument2, (void **)&pIHTML)))
        {
            AddDOCExpandoProperty(XMLTREE, pIHTML, NULL);
            AddDOCExpandoProperty(XSLTREE, pIHTML, NULL);
            SafeRelease(pIHTML);
        }

        // And remove the _pSite pointer from _pDocument which is the last reference
        // to _pTrident.
        IObjectWithSite* pSite = NULL;
        if (null != _pDocument && 
            SUCCEEDED(_pDocument->QueryInterface(IID_IObjectWithSite, (void **)&pSite)))
        {
            pSite->SetSite(NULL);
            SafeRelease(pSite);
        }

        SafeRelease(_pTrident);
        Release();
    }
    if (InterlockedDecrement(&_ulRefs) == 0)
    {
        delete this; 
        return 0;
    }
    return _ulRefs;
}

//  ====================== IPersist interface =====================
HRESULT STDMETHODCALLTYPE 
Viewer::GetClassID( 
       /* [out] */ CLSID *pClassID)
{
    *pClassID = CLSID_Viewer;
    return S_OK;
}


// ====================== IPersistMoniker interface =====================   
HRESULT STDMETHODCALLTYPE 
Viewer::IsDirty( void)
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE 
Viewer::Load( 
        /* [in] */ BOOL fFullyAvailable,
        /* [in] */ IMoniker __RPC_FAR *pimkName,
        /* [in] */ LPBC pibc,
        /* [in] */ DWORD grfMode)
{ 
    // Load the specified XML file, and write HTML output 
    // to a stream which is then opened by Trident for
    // for rendering.
    HRESULT hr = S_OK;
    IMoniker* pMk = NULL;
    IPersistMoniker* pPMK = NULL;
    LPBC pbc = NULL;
    IUnknown *pIClientSite = NULL;

    SafeRelease(_pimkCurrent);
    _pimkCurrent = pimkName; // save away the moniker.
    _pimkCurrent->AddRef();

    // The buffered stream is held onto by the factory and written
    // to as the XML parser parses the input.
    MIMEBufferedStream* pStm = new_ne MIMEBufferedStream();
    CHECKALLOC(hr, pStm);
   
    // The same buffered stream is held onto by Trident and
    // is read from while the XML parser is writing to it.  The
    // buffered stream returns E_PENDING until the XML parser goes
    // away (i.e. is finished).
    pMk = new_ne BufferedStreamMoniker(pStm, pimkName);
    CHECKALLOC(hr, pMk);

    // new bind context for trident
    hr = ::CreateBindCtx(0, &pbc);
    CHECKHR(hr);

    // Now we have to pass thru certain objects that SHDOCVW
    // wants to tell trident about, such as the client site 
    // However, we may not be coming from SHDOCVW, but URLMON
    // directly (such as the case when coming from HLINK)
    // So if we fail the Get, it is non-fatal.
    hr = pibc->GetObjectParam(WSZGUID_OPID_DocObjClientSite, &pIClientSite);
    if (hr == S_OK && pIClientSite != NULL)
    {
        hr = pbc->RegisterObjectParam(WSZGUID_OPID_DocObjClientSite, pIClientSite);
        CHECKHR(hr);
        SafeRelease(pIClientSite);  // get rid of it immediately
    }

    // Initiate the load via moniker::load
    hr = _pTrident->QueryInterface(IID_IPersistMoniker, (void**)&pPMK);
    CHECKHR(hr);
    AddRef(); // keep ourselves alive while calling into Trident !!
    hr = pPMK->Load(FALSE, pMk, pbc, STGM_READ );
    Release();
    CHECKHR(hr);

    // hook up the callback and binding chains and initiate the xml parsing
    hr = reload(fFullyAvailable, pimkName, pbc, pibc, grfMode, pStm);

CleanUp:    
    SafeRelease(pIClientSite);
    SafeRelease(pbc);
    SafeRelease(pMk);
    SafeRelease(pPMK);
    SafeRelease(pStm);
    // returning errors here causes Trident to crash.
    // See bug 40976.  Besides we've already written the error
    // to the HTML page anyway.
    return S_OK; 
}

HRESULT STDMETHODCALLTYPE 
Viewer::Save( 
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pbc,
    /* [in] */ BOOL fRemember)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
Viewer::SaveCompleted( 
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pibc)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
Viewer::GetCurMoniker( 
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName)
{
    *ppimkName = _pimkCurrent;
    return S_OK;
}

// ====================== IPersistHistory interface =====================   
HRESULT STDMETHODCALLTYPE 
Viewer::LoadHistory( 
     /* [in] */ IStream __RPC_FAR *pStream,
     /* [in] */ IBindCtx __RPC_FAR *pbc)
{
    HRESULT hr;
    IPersistHistory *pPH = NULL;
    hr = _pTrident->QueryInterface(IID_IPersistHistory, (void **)&pPH);
    if (SUCCEEDED(hr) && pPH)
    {
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pPH->LoadHistory(pStream, pbc);
        Release();
    }
    SafeRelease(pPH);
    return hr;
}
        
HRESULT STDMETHODCALLTYPE 
Viewer::SaveHistory( 
     /* [in] */ IStream __RPC_FAR *pStream)
{
    HRESULT hr = S_OK;  
    IPersistHistory *pPH = NULL;
    hr = _pTrident->QueryInterface(IID_IPersistHistory, (void **)&pPH);
    if (SUCCEEDED(hr) && pPH)
    {
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pPH->SaveHistory(pStream);
        Release();
    }
    SafeRelease(pPH);
    return hr;
}
        
HRESULT STDMETHODCALLTYPE 
Viewer::SetPositionCookie( 
    /* [in] */ DWORD dwPositioncookie)
{
    HRESULT hr;
    IPersistHistory *pPH = NULL;
    hr = _pTrident->QueryInterface(IID_IPersistHistory, (void **)&pPH);
    if (SUCCEEDED(hr) && pPH)
    {
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pPH->SetPositionCookie(dwPositioncookie);
        Release();
    }
    SafeRelease(pPH);
    return hr;
}
        
HRESULT STDMETHODCALLTYPE 
Viewer::GetPositionCookie( 
   /* [out] */ DWORD __RPC_FAR *pdwPositioncookie)
{   
    HRESULT hr;
    IPersistHistory *pPH = NULL;
    hr = _pTrident->QueryInterface(IID_IPersistHistory, (void **)&pPH);
    if (SUCCEEDED(hr) && pPH)
    {
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pPH->GetPositionCookie(pdwPositioncookie);
        Release();
    }
    SafeRelease(pPH);
    return hr;
}
       


// ====================== IOleCommandTarget interface =====================   
HRESULT STDMETHODCALLTYPE 
Viewer::QueryStatus( 
    /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
    /* [in] */ ULONG cCmds,
    /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
     /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText)
{
    HRESULT hr;
    IOleCommandTarget *pICT = NULL;
    hr = _pTrident->QueryInterface(IID_IOleCommandTarget, (void **)&pICT);
    if (SUCCEEDED(hr) && pICT)
    {
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pICT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
        Release();
    }
    SafeRelease(pICT);
    return hr;
}
        
#undef TrackPopupMenu
#undef DestroyMenu


HRESULT STDMETHODCALLTYPE 
Viewer::Exec( 
    /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
    /* [in] */ DWORD nCmdID,
    /* [in] */ DWORD nCmdexecopt,
    /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
    /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut)
{
    HRESULT hr;
    VARIANT var;
    IOleCommandTarget *pICT = NULL;

    /* process save as */
    if (!pguidCmdGroup && (nCmdID == OLECMDID_SAVEAS))
        hr = OleCmdSaveXML(nCmdexecopt, pvaIn);

    /* process encoding open */
    /* ugh - we have to do it ourselves in order to get a chance to gray menus */
    else if ( (pguidCmdGroup) && (*pguidCmdGroup == CGID_ShellDocView) && (nCmdID == SHDVID_MIMECSETMENUOPEN) )
    {
        // get the menu and gray it by recursing into ourselves
        hr = Exec(pguidCmdGroup, SHDVID_GETMIMECSETMENU, 0, NULL, &var);
        if (hr == S_OK)
        {
            int cmdID;
            MSG msg;
            BOOL bCmd = FALSE;
            POINT pt = {GET_X_LPARAM(pvaIn->lVal), GET_Y_LPARAM(pvaIn->lVal)};
            HMENU hMenuEncoding = (HMENU)(V_I4(&var));
            HWND hWnd = saveGetHWND();
            if (hMenuEncoding && hWnd)
            {
                if (::TrackPopupMenu(hMenuEncoding, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, (RECT *)NULL))
                {
                    // The menu popped up and the item was chosen.  Peek messages
                    // until the specified command was found.
                    if (PeekMessage(&msg, hWnd, WM_COMMAND, WM_COMMAND, PM_REMOVE))
                    {
                        cmdID = GET_WM_COMMAND_ID(msg.wParam, msg.lParam);
                        bCmd = TRUE;
                    }

                }          
                DestroyMenu(hMenuEncoding);
                if (bCmd)   // recurse with the id
                    Exec((GUID *)&CGID_MSHTML, (DWORD)cmdID, 0, NULL, NULL);
            }
        }
    }


    /* otherwise farm out to trident */
    else
    {   

        BOOL fViewSource = FALSE;

        // if we get a view source, then translate it to VIEWPRETRANSFORMSOURCE before passing to trident
        if ( (pguidCmdGroup) && (*pguidCmdGroup == CGID_MSHTML) && (nCmdID == IDM_VIEWSOURCE) )
        {
            nCmdID = IDM_VIEWPRETRANSFORMSOURCE;
            fViewSource = TRUE;
        }

        // now delegate to trident
        hr = _pTrident->QueryInterface(IID_IOleCommandTarget, (void **)&pICT);
        if (SUCCEEDED(hr) && pICT)
        {
            AddRef(); // keep ourselves alive while calling into Trident !!
            hr = pICT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
            Release();
            if (hr == S_OK)
            {
                if ( (pguidCmdGroup) && 
                     (*pguidCmdGroup == CGID_ShellDocView) && 
                     (nCmdID == SHDVID_GETMIMECSETMENU)
                )
                    disableEncodingMenu((HMENU) V_I4(pvaOut));
            }
            else if (fViewSource)
            {
                // we need to report the error message.  This is usually triggered
                // if the pretransformed source is not available.  Trident
                // would just die silently, but we want to tell the user something
                alert(XMLMIME_SOURCENA);   // drop error on floor
            }
        }
    }
CleanUp: 
    SafeRelease(pICT);
    return hr;
}


// ====================== IXMLViewerIdentity interface =====================    
HRESULT STDMETHODCALLTYPE 
Viewer::get_XMLViewerObject( 
   /* [out][retval] */ VARIANT __RPC_FAR *pObj)
{
    pObj->vt = VT_BYREF;
    V_BYREF(pObj) = this;
    return S_OK;
}
    
// ====================== non-virtual functions =====================   
HRESULT 
Viewer::reloadFromTrident(
    MIMEBufferedStream *pStm, IMoniker *pimk, LPBC pbcTrident)
{
    HRESULT hr;
    LPBC pbcXML = NULL;
    hr = ::CreateBindCtx(0, &pbcXML);
    if (SUCCEEDED(hr))
        hr = reload(FALSE, pimk, pbcTrident, pbcXML, STGM_READ, pStm);
    SafeRelease(pbcXML);
    return hr;
}

HRESULT 
Viewer::reload(
    BOOL fFullyAvailable, IMoniker *pimk, LPBC pbcTrident, LPBC pbcXML, DWORD grfMode, MIMEBufferedStream *pStm)
{
    HRESULT hr = S_OK;
    IXMLParser* pParser = NULL;
    IXMLNodeFactory *pDefaultFactory = NULL;
    ViewerFactory *pFactory = NULL;
    IBindStatusCallback* bcs = NULL;
    IBindStatusCallback *bcsJunk = NULL;
    CallbackWrapper* icallback = NULL;
    CallbackMonitor* imonitor = NULL;
    IPersistMoniker *pPMK = NULL;
    IObjectWithSite* pSite = NULL;

#if MIMEASYNC
    // create the event that will later signal end of the download
    // on the worker thread
    if (_evtLoadComplete == NULL)
    {
        _evtLoadComplete = CreateEventA(NULL, TRUE, TRUE, NULL);    // the worker thread will reset the event
        if (!_evtLoadComplete)
        {
            hr = GetLastError();
            goto CleanUp;
        }
    }    
#endif    

    // Setup new bind context and bind status callback.
    icallback = new_ne CallbackWrapper();
    CHECKALLOC(hr, icallback);
    hr = ::RegisterBindStatusCallback(pbcTrident, icallback, &bcs, 0);
    CHECKHR(hr);
    icallback->SetPreviousCallback(bcs);  
    SafeRelease(bcs);

    cleanupBinding();
    _pBinding = new_ne CBinding();
    CHECKALLOC(hr, _pBinding);
    pStm->SetCallback(icallback, _pBinding);
        
    // set the bsc before calling _pDocument so XML forwards calls
    // to it and we can forward to our wrapper
    imonitor = new_ne CallbackMonitor();
    CHECKALLOC(hr, imonitor);
    hr = ::RegisterBindStatusCallback(pbcXML, imonitor, &bcsJunk, 0);
    CHECKHR(hr);
    imonitor->SetPreviousCallback(icallback);
    SafeRelease(bcsJunk);

    // In the "refresh(f5)" case, the Viewer object is re-used, and
    // so the exising _pDocument object is reused.  So first we need
    // to make sure it is in a fully reset state - otherwise previous
    // state may interfere with this load operation.
    hr = _pDocument->abort(); // this is the easiest way to reset it.
    CHECKHR(hr); 

    hr = _pDocument->put_validateOnParse(VARIANT_FALSE);
    CHECKHR(hr);
    hr = _pDocument->put_resolveExternals(VARIANT_TRUE);
    CHECKHR(hr);

    // Make sure we set a site object on _pDocument so security works correctly.
    // Actually it doesn't matter what site pointer we use, we just have
    // to make sure the site is not null.
    // (This has to be done before QI for IXMLParser).
    hr = _pDocument->QueryInterface(IID_IObjectWithSite, (void **)&pSite);
    CHECKHR(hr);
    pSite->SetSite(_pTrident);
    SafeRelease(pSite);

    // get the old factory so the new factory can delegate to it to build
    // the tree
    hr = _pDocument->QueryInterface(IID_IXMLParser, (void**)&pParser);
    CHECKHR(hr);
    hr = pParser->GetFactory(&pDefaultFactory);
    CHECKHR(hr);
    pFactory = new_ne ViewerFactory(pStm, pDefaultFactory, _pDocument, pimk, imonitor, _evtLoadComplete);
    CHECKALLOC(hr, pFactory);
    hr = pParser->SetFactory(pFactory);
    CHECKHR(hr);
    SafeRelease(pDefaultFactory);

    // Tell the binding so it can manage the abort
    pStm->SetAbortCB(this, pFactory);

    // Load the XML document -- which will use the factory we
    // just plugged in.
    hr = _pDocument->QueryInterface(IID_IPersistMoniker, (void**)&pPMK);
    CHECKHR(hr);
    hr = pPMK->Load(FALSE, pimk, pbcXML, grfMode);    
    CHECKHR(hr);

    // From here on out, tell the parser to NOT call Read on the IStream 
    // during Run but instead let the OnDataAvailable buffer up the data 
    // so that it happens in the right thread.
    pParser->SetFlags(pParser->GetFlags() | XMLFLAG_RUNBUFFERONLY);

CleanUp:
    // Make sure we commit the stream.
    if (!SUCCEEDED(hr))
    {
        if (pFactory)
        {
            if (! ((ViewerFactory *)pFactory)->isErrReported())
            {
                ((ViewerFactory *)pFactory)->reportError(hr);
            }
            hr = S_OK;
        }
        else
        {
            pStm->Commit(0);
        }
    }
    SafeRelease(pFactory);
    SafeRelease(pDefaultFactory);
    SafeRelease(pParser);
    SafeRelease(bcsJunk);
    SafeRelease(bcs);
    SafeRelease(pPMK);
    SafeRelease(icallback);
    SafeRelease(imonitor);
    return hr;
}


HRESULT 
Viewer::init(void)
{
    HRESULT hr;
    IObjectSafety* pSafe = NULL;
    IOleObject *pIOle = NULL;
    IOleClientSite *pICS = NULL;
    DWORD dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_DATA;
    DWORD dwXOptions = dwXSetMask;

    SafeRelease(_pTrident);
    SafeRelease(_pDocument);

    hr = CoCreateInstance(CLSID_HTMLDocument, reinterpret_cast<IUnknown*>(this), CLSCTX_INPROC_SERVER,
                            IID_IUnknown, (void**)&_pTrident);
    CHECKHR(hr);

    hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                            IID_IXMLDOMDocument, (void**)&_pDocument);
    CHECKHR(hr);

    // Set up the object safety options in the document right away so that 
    // DTD and schema downloads will be secure.
    hr = _pDocument->QueryInterface(IID_IObjectSafety, (void **)&pSafe);
    CHECKHR(hr);

    hr = pSafe->SetInterfaceSafetyOptions(IID_IUnknown, dwXSetMask, dwXOptions);

CleanUp:
    SafeRelease(pSafe);
    SafeRelease(pICS);
    SafeRelease(pIOle);

    return hr;
}


HRESULT
Viewer::stopDownload(void)
{
    HRESULT hr = S_FALSE;
    if (_pDocument)
        hr = _pDocument->abort();

    cleanupBinding();
    return hr;
}

IUnknown* 
Viewer::getTrident() 
{ 
    if (_pTrident)
    {
        _pTrident->AddRef(); 
    }
    return _pTrident; 
}

void
Viewer::cleanupBinding()
{
    // Cleanup the _pViewer pointer in the CBinding so it's not left dangling.
    if (_pBinding)
    {
        _pBinding->SetAbortCB(NULL,NULL);
        SafeRelease(_pBinding);
    }
}

HRESULT
Viewer::OleCmdSaveXML(DWORD nCmdexecOpt, VARIANT *pvaIn)
{
    HRESULT hr;
    TCHAR achPath[MAX_PATH+1];
    LPTSTR pchPath = NULL;

    if (nCmdexecOpt == MSOCMDEXECOPT_SHOWHELP)
        return E_NOTIMPL;
    if (pvaIn && V_VT(pvaIn) == VT_BSTR)
        pchPath = V_BSTR(pvaIn);

    // take what was passed in 
    if (nCmdexecOpt == OLECMDEXECOPT_DONTPROMPTUSER)
    {
        if (!pchPath)
            return E_INVALIDARG;
    }
    // or prompt the user
    else
    {
        achPath[0] = 0;
        if (pchPath)
        {
            _tcsncpy(achPath, pchPath, MAX_PATH);
            achPath[MAX_PATH - 1] = 0;
        }
        else
            saveGetNameFromURL(achPath);
        // BUGBUG what if blank?
        hr = savePromptFileName(achPath, sizeof(achPath) / sizeof (WCHAR));
        if (hr)
            goto CleanUp;
        pchPath = achPath;
    }
    
    // now save it out to the specified file
    hr = saveXMLFile(pchPath);
CleanUp: ;
    if (hr == S_FALSE)
        hr = OLECMDERR_E_CANCELED;
    return hr;
}


/* 
void
Viewer::disableCmds(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, const GUID *pguidCmdGroupDisable, DWORD cmdFirst, DWORD cmdLast)
{
    Assert(cCmds==0 || prgCmds);
    ULONG i;
    OLECMD *pCmd;

    if ( (!pguidCmdGroup && !pguidCmdGroupDisable) ||
         (pguidCmdGroup && pguidCmdGroupDisable && (*pguidCmdGroup == *pguidCmdGroupDisable)) )
    {
        for (i = 0, pCmd = prgCmds; i < cCmds; i++, pCmd++)
        {
            if ((pCmd->cmdID >= cmdFirst) && (pCmd->cmdID <= cmdLast))
                pCmd->cmdf &= ~(OLECMDF_SUPPORTED | OLECMDF_ENABLED);
        }
    }
}
*/    
    
void
Viewer::disableEncodingMenu(HMENU hMenu)
{
    UINT state, cmdID;
    int cnt;
    if (hMenu)
    {
        cnt = ::GetMenuItemCount(hMenu);
        for (int i = 0; i < cnt; i++)
        {
            state = ::GetMenuState(hMenu, i, MF_BYPOSITION);
            if (state != 0xFFFFFFFF)
            {
                if (HIBYTE(state) != 0)     // we have a submenu, recurse
                    disableEncodingMenu(::GetSubMenu(hMenu, i));
                else if ((LOBYTE(state) & (MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR)) == 0)
                {
                    cmdID = ::GetMenuItemID(hMenu, i);
                    if (cmdID != 0xFFFFFFFF)        // shouldn't happen but just in case
                    {
                        if (cmdID >= IDM_MIMECSET__FIRST__ && cmdID <= IDM_MIMECSET__LAST__)
                            ::EnableMenuItem(hMenu, i, MF_BYPOSITION | MF_GRAYED);
                    }
                }
            }
        }
    }
}


void
Viewer::saveGetNameFromURL(LPTSTR pchPath)
{
    HRESULT hr;
    int cchUrl;
    TCHAR *pchQuery;
    BSTR bstrURL = NULL;
    LPTSTR wURL;
    PARSEDURL puw = {0};

    hr = _pDocument->get_url(&bstrURL);
    CHECKHR(hr);
    wURL = (LPWSTR)bstrURL;
    if (wURL)
    {
        cchUrl = _tcslen(wURL);
        if ( (cchUrl) && (*(wURL+cchUrl-1) != L'/') )
        {
            puw.cbSize = sizeof(PARSEDURL);
            if (SUCCEEDED(ParseURL(wURL, &puw)))
            {
                // Temporarily, null out the '?' in the url
                pchQuery = _tcsrchr(puw.pszSuffix, _T('?'));
                if (pchQuery)
                    *pchQuery = 0;
                _tcsncpy(pchPath, PathFindFileName(puw.pszSuffix), MAX_PATH);
                if (pchQuery)
                    *pchQuery = _T('?');
            }
        }
    }
CleanUp:
    ::SysFreeString(bstrURL);
}

extern char* WideToAscii(const WCHAR* string);
typedef BOOL (WINAPI * PFNGETSAVEFILENAME)(OPENFILENAME *);

HRESULT
Viewer::savePromptFileName(LPTSTR pchPath, int cchPath)
{
    HRESULT hr = S_FALSE;
    OPENFILENAME ofn;
    HWND hIE; 
    PFNGETSAVEFILENAME pfnGetSaveFileName = NULL;
    HINSTANCE hComDlg = NULL;
    String *sExt;
    String *sDefExt;
    LPCTSTR pDSFileTypes;
    TCHAR *pVal, *pStart;
    LPCTSTR pbDefFileExt;
    LPCTSTR pbFileExt;
    char *pDSFileTypesA = NULL;
    char *pbDefFileExtA = NULL;
    char *pchPathA = NULL;
    char *pValA;
    int cbFileExt = 0;
    int len;
    LPTSTR pwFileExt;
    DWORD iFilterIndex = -1;
    DWORD iIndex = 1;
    BOOL fScanFilter = FALSE;
    BOOL fAddExt = FALSE;
    BOOL fNT;

    hIE = saveGetHWND();
    if (!hIE)
        return S_FALSE;

    sExt = Resources::LoadString(IDS_MIMEXMLFILETYPES);  
    pDSFileTypes = sExt->getData();

    // load up the default extension to append if the user leaves one off
    sDefExt = Resources::LoadString(IDS_MIMEDEFEXTENSION);
    pbDefFileExt = sDefExt->getData();

    /* get the file extension from the suggested save file name */
    /* if you don't have one then use the default */
    pwFileExt = PathFindExtension(pchPath);
    if (pwFileExt && *pwFileExt!=_T('\0'))
        pbFileExt = pwFileExt;
    else
    {
        pbFileExt = pbDefFileExt;
        fAddExt = (pbFileExt != NULL);
    }

    if (pbFileExt)
        cbFileExt = lstrlenW(pbFileExt);

    // figure out which file extension filter to use initially. We do this by
    // comparing the supplied tentative file name to filters as they are encountered
    // in the string. If there was no extension then use first filter (*.xml).
    for ( pVal = pStart = (LPTSTR)pDSFileTypes; *pVal; pVal++ )
    {
        if ( *pVal == _T('|') )
        {
            *pVal = _T('\0');

            if (fScanFilter)
            {
                if ((iFilterIndex==-1) && (cbFileExt!=0) && (cbFileExt < pVal-pStart))
                {
                    if (!StrCmpNIW(pbFileExt, pVal-cbFileExt, cbFileExt))
                        iFilterIndex = iIndex;
                }
                iIndex++;
            }
            fScanFilter = !fScanFilter;
            pStart = pVal + 1;
            *pVal = _T('|');    // set it back
        }
    }

    // if we didn't get a match then set to all files
    if (iFilterIndex == -1)
        iFilterIndex = iIndex - 1;

    // set up the path. append the extension if the user didn't supply it.
    len = lstrlenW(pchPath);
    if (fAddExt)
        len += cbFileExt;
    if (len < MAX_PATH)
        len = MAX_PATH;
    if (fAddExt)
        lstrcatW(pchPath, pbFileExt);

#ifdef UNIX
    fNT = FALSE;
#else
    fNT = (g_dwPlatformId == VER_PLATFORM_WIN32_NT);
#endif
    
    // For NT we use the unicode version, for Win9x and Unix we use the
    // ascii versions. For the latter we generally convert the strings used
    // above to their ascii equivalents.  
    
    // Note we convert the resource string, replacing | with \0
    // e.g, "XML Files (*.xml)|*.xml|XSL Files (*.xsl)|*.xsl|All Files (*.*)|*.*|"
    // into pairs of null-terminated strings as required by the dialog

    // We have to do this conversion separated for ANSI and UNICODE (UGH).
    // as WideCharToMultiByte wouldn't work past the first \0 if we
    // converted the string in UNICODE first. 
    if (fNT)
    {
        for ( pVal = (LPTSTR)pDSFileTypes; *pVal; pVal++ )
        {
            if ( *pVal == _T('|') )
               *pVal = _T('\0');
        }
    }    
    
    // use ascii GetSaveFileName, convert parameters to ascii equivalents
    else 
    {
        pDSFileTypesA = WideToAscii(pDSFileTypes);
        if (!pDSFileTypesA)
            goto CleanUp;
        for ( pValA = pDSFileTypesA; *pValA; pValA++ )
        {
            if ( *pValA == '|' )
               *pValA = '\0';
        }
        if (pbDefFileExt)
        {
            pbDefFileExtA = WideToAscii(pbDefFileExt);
            if (!pbDefFileExtA)
                goto CleanUp;
        }
        len = WideCharToMultiByte(CP_ACP, 0, pchPath, -1, NULL, 0, NULL, NULL);
        if (len < MAX_PATH)
            len = MAX_PATH;
        pchPathA = new_ne char[len+1];
        if (!pchPathA)
            goto CleanUp;
        WideCharToMultiByte(CP_ACP, 0, pchPath, -1, pchPathA, len, NULL, NULL);
    }

    // set up the ofn structure   
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hIE;
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER |
                OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST;
    if (fNT)
    {
        ofn.lpstrDefExt = pbDefFileExt ? (LPCTSTR)(pbDefFileExt+1) : NULL;
        ofn.lpstrFilter = (LPCTSTR)pDSFileTypes;
        ofn.lpstrFile = pchPath;
    }
    else
    {
        ofn.lpstrDefExt = pbDefFileExtA ? (LPCTSTR)(pbDefFileExtA+1) : NULL;
        ofn.lpstrFilter = (LPCTSTR)pDSFileTypesA;
        ofn.lpstrFile = (LPTSTR)pchPathA;
    }
    ofn.nMaxFile = cchPath;
    ofn.nFilterIndex = iFilterIndex;

    // get the proc address if you have to
    hComDlg = (HINSTANCE)LoadLibraryA("COMDLG32.DLL");
    if (hComDlg == NULL)
        goto CleanUp;
    pfnGetSaveFileName = (PFNGETSAVEFILENAME)GetProcAddress(hComDlg, fNT ? "GetSaveFileNameW" : "GetSaveFileNameA");
    if (pfnGetSaveFileName == NULL)
        goto CleanUp;;
    if (pfnGetSaveFileName(&ofn))
    {
        // convert back to wide char
        if (!fNT)
            MultiByteToWideChar(CP_ACP, 0, pchPathA, -1, pchPath, cchPath);
        hr = S_OK;
    }
CleanUp:
    if (hComDlg)
        FreeLibrary(hComDlg);
    if (pDSFileTypesA)
        delete [] pDSFileTypesA;
    if (pbDefFileExtA)
        delete [] pbDefFileExtA;
    if (pchPathA)
        delete [] pchPathA;
    return hr;
}

HRESULT
Viewer::saveXMLFile(LPCTSTR pchPath)
{
    HRESULT hr;
    IOleCommandTarget *pITgt = NULL;
    VARIANT var;
    BSTR bTgtFile = NULL;

    bTgtFile = ::SysAllocString(pchPath);
    if (bTgtFile)
    {
        hr = _pTrident->QueryInterface(IID_IOleCommandTarget, (void **)&pITgt);
        CHECKHR(hr);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bTgtFile;
        AddRef(); // keep ourselves alive while calling into Trident !!
        hr = pITgt->Exec((GUID *)&CGID_MSHTML, (DWORD)IDM_SAVEPRETRANSFORMSOURCE, 0, &var, NULL);
        Release();
    }
    else
        hr = E_OUTOFMEMORY;

CleanUp:
    SafeRelease(pITgt);
    SafeFreeString(bTgtFile);
    return hr;
}


HWND
Viewer::saveGetHWND(void)
{
    HRESULT hr;
    HWND hRet, h;
    IOleObject *pIOle = NULL;
    IOleClientSite *pICS = NULL;
    IOleWindow *pIWindow = NULL;

    AddRef(); // keep ourselves alive while calling into Trident !!

    hRet = NULL;
    hr = _pTrident->QueryInterface(IID_IOleObject, (void **)&pIOle);
    CHECKHR(hr);
    hr = pIOle->GetClientSite(&pICS);
    CHECKHR(hr);
    hr = pICS->QueryInterface(IID_IOleWindow, (void **)&pIWindow);
    CHECKHR(hr);
    hr = pIWindow->GetWindow(&h);
    CHECKHR(hr);
    hRet = h;

CleanUp:
    Release();
    SafeRelease(pIWindow);
    SafeRelease(pICS);
    SafeRelease(pIOle);
    return hRet;
}


HRESULT
Viewer::alert(BSTR bstrMsg)
{
    HRESULT hr;
    IHTMLDocument2 *pTridentDoc = NULL;
    IHTMLWindow2 *pTridentWdw = NULL;

    // use Trident alert logic which will go Host UI handlers.  For IE
    // this is usually the shell, but by using this logic we can be
    // hosted by others just like MSHTML.
    if (!_pTrident)
        return E_FAIL;

    AddRef(); // keep ourselves alive while calling into Trident !!
    hr = _pTrident->QueryInterface(IID_IHTMLDocument2, (void **)&pTridentDoc);
    CHECKHRPTR(hr, pTridentDoc);
    hr = pTridentDoc->get_parentWindow(&pTridentWdw);
    CHECKHRPTR(hr, pTridentWdw);
    pTridentWdw->alert(bstrMsg);

CleanUp:
    Release();
    SafeRelease(pTridentWdw);
    SafeRelease(pTridentDoc);
    return hr;
}

HRESULT
Viewer::alert(ResourceID resID)
{
    HRESULT hr;
    BSTR bstrMsg = NULL;
    String* s = Resources::FormatMessage(resID, NULL);
    bstrMsg = s->getBSTR();
    if (bstrMsg == NULL) {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    hr = alert(bstrMsg);
CleanUp:
    SafeFreeString(bstrMsg);
    return hr;
}

//======================================================================
    
// This class is not needed until we enable the HTML DOM wrappers
#if 0
class CPeerFactory : public IElementBehaviorFactory
{
public:
    CPeerFactory()
    {
        _ulRefs = 1;
        IncrementComponents();
    }

    ~CPeerFactory()
    {
        DecrementComponents();
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void ** ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IElementBehaviorFactory) 
        {
            *ppv = this;
            AddRef();
            return S_OK;    
        }
        // should eventually implement IDispatch 
        else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }
 
    virtual ULONG STDMETHODCALLTYPE AddRef() 
    {
        return InterlockedIncrement(&_ulRefs);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        if (InterlockedDecrement(&_ulRefs) == 0)
        {
            delete this;
            return 0;
        }
        return _ulRefs;
    }
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindBehavior( 
        /* [in] */ BSTR bstrBehavior,
        /* [in] */ BSTR bstrBehaviorUrl,
        /* [in] */ IElementBehaviorSite *pSite,
        /* [out] */ IElementBehavior **ppBehavior)
    {
        HRESULT     hr = E_FAIL;

        DOMNodeWrapper *pVP = new_ne DOMNodeWrapper();
        if (!pVP)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        hr = pVP->QueryInterface(IID_IElementBehavior, (void **)ppBehavior);
        if (hr)
        {
            delete pVP;
            goto Error;
        }

        SafeRelease(pVP);

    Error:
        return hr;
    }


private:
    long _ulRefs;
};
#endif

//======================================================================
// Component Creation Functions (used by factory) : should be collapsed


HRESULT STDMETHODCALLTYPE
CreateViewer(REFIID iid, void **ppvObj)
{
    HRESULT hr = S_OK;

#ifdef RENTAL_MODEL
    Model model(Rental);
#endif
    Viewer * pViewer = new_ne Viewer();
    if (!pViewer)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    hr = pViewer->init();
    if (hr)
    {
        delete pViewer;
        goto Error;
    }

    hr = pViewer->QueryInterface(iid, ppvObj);
    if (hr)
        goto Error;

    SafeRelease(pViewer);

Error:
    return hr;
}

#if 0
HRESULT STDMETHODCALLTYPE
CreatePeerFactory(REFIID iid, void **ppvObj)
{
    HRESULT hr = S_OK;

    CPeerFactory * pVP = new_ne CPeerFactory();
    if (!pVP)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = pVP->QueryInterface(iid, ppvObj);
    if (hr)
        goto Error;

    SafeRelease(pVP);

Error:
    return hr;
}
#endif

HRESULT STDMETHODCALLTYPE
CreateBufferedMoniker(REFIID iid, void **ppvObj)
{
    HRESULT hr = S_OK;

    BufferedStreamMoniker * pVP = new_ne BufferedStreamMoniker(NULL, NULL);
    if (!pVP)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = pVP->QueryInterface(iid, ppvObj);
    if (hr)
        goto Error;

    SafeRelease(pVP);

Error:
    return hr;
}

#if 0
HRESULT STDMETHODCALLTYPE
CreateViewerPeer(REFIID iid, void **ppvObj)
{
    HRESULT hr = S_OK;

#ifdef RENTAL_MODEL
    Model model(Rental);
#endif
    DOMNodeWrapper * pVP = new_ne DOMNodeWrapper();
    if (!pVP)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    
    hr = pVP->QueryInterface(iid, ppvObj);
    if (hr)
        goto Error;

    SafeRelease(pVP);

Error:
    return hr;
}
#endif
