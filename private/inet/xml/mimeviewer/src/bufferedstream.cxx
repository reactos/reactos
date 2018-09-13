/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop
#include "utils.hxx"
#include "staticunknown.hxx"
#include "xmlmimeguid.hxx"
#include "bufferedstream.hxx"
#include "xmlview.hxx"

DeclareTag(tagMIME, "MimeViewer", "Trace Mimeviewer actions");

CLIPFORMAT MIMEBufferedStream::_cfHtml = CF_NULL;

#ifdef _DEBUG
static LONG g_cMonikers = 0;
static LONG g_cMimeStreams = 0;
#endif

//======================================================================
MIMEBufferedStream::MIMEBufferedStream()
{
    IncrementComponents();  
#ifdef _DEBUG
    InterlockedIncrement(&g_cMimeStreams);
#endif
    _fCommit = false;
    _cRefs = 1;
    _pchBuffer = NULL;
    _cSize = _cWritten = _cRead = _cLastNotify = 0;
    _fCanRead = false;
    _fFirstNotify = true;
    _pbs = NULL;
    _pBinding = NULL;
#if MIMEASYNC
    _mdhAsync = NULL;
    _fStopAsync = false;
    InitializeCriticalSection(&_csRW);
#endif

#if DBG == 1
    _xslReads = _xslWrites = _ntfData = _ntfReceive = 0;
    _nConflicts = 0;
    _bCSActive = false;
#endif
    // EnableTag(tagMIME, TRUE);
}

MIMEBufferedStream::~MIMEBufferedStream() 
{         
#ifdef _DEBUG
    InterlockedDecrement(&g_cMimeStreams);
#endif
#if MIMEASYNC
    // clean up the gui wnd here
    if (_mdhAsync && g_pMimeDwnWndMgr)
    {
        g_pMimeDwnWndMgr->ReleaseGUIWnd(_mdhAsync);
        _mdhAsync = NULL;
    }
#endif
    CleanUp();
    delete _pchBuffer;
#if MIMEASYNC
    DeleteCriticalSection(&_csRW);
#endif
    DecrementComponents();
}

ULONG STDMETHODCALLTYPE 
MIMEBufferedStream::AddRef(void)
{ 
     return (ULONG)InterlockedIncrement((LPLONG)&_cRefs);
}

ULONG STDMETHODCALLTYPE 
MIMEBufferedStream::Release( void) 
{ 
    ULONG refs = (ULONG)InterlockedDecrement((LPLONG)&_cRefs);
    if (refs == 0) 
    {
        delete this;
    }
    return refs;
}


//======================================================================
BufferedStreamMoniker::BufferedStreamMoniker(MIMEBufferedStream* pStm, IMoniker *pimkSrc)
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cMonikers);
#endif
    _ulRefs = 1;
    _pStm = pStm;
    if (_pStm)
        _pStm->AddRef();
    _pimkSrc = pimkSrc;
    if (_pimkSrc)
        _pimkSrc->AddRef();
    IncrementComponents();  
}

BufferedStreamMoniker::~BufferedStreamMoniker()
{
#ifdef _DEBUG
    InterlockedDecrement(&g_cMonikers);
#endif
    SafeRelease(_pStm);
    SafeRelease(_pimkSrc);
    DecrementComponents();
}

HRESULT STDMETHODCALLTYPE 
BufferedStreamMoniker::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IPersist || riid == IID_IPersistStream || riid == IID_IMoniker) 
    {
        *ppv = this;
        AddRef();
        return S_OK;    
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE 
BufferedStreamMoniker::AddRef(void)
{
    return InterlockedIncrement(&_ulRefs);
}

ULONG STDMETHODCALLTYPE 
BufferedStreamMoniker::Release(void)
{
    ULONG refs = InterlockedDecrement(&_ulRefs);
    if (refs == 0)
    {
        delete this;
        return 0;
    }
    return refs;
}

HRESULT STDMETHODCALLTYPE 
BufferedStreamMoniker::GetClassID( 
    /* [out] */ CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_BufferedMoniker;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
BufferedStreamMoniker::Load( 
    /* [unique][in] */ IStream __RPC_FAR *pStm)
{
    if (!_pimkSrc)
    {
        HRESULT hr = CoCreateInstance(CLSID_StdURLMoniker, NULL, CLSCTX_INPROC_SERVER, IID_IMoniker, (void**)&_pimkSrc);
        if (!SUCCEEDED(hr))
            return hr;
        if (!_pimkSrc)
                return E_FAIL;
    }
    return _pimkSrc->Load(pStm); 
}

HRESULT STDMETHODCALLTYPE 
BufferedStreamMoniker::Save( 
    /* [unique][in] */ IStream __RPC_FAR *pStm,
    /* [in] */ BOOL fClearDirty) 
{ 
    return _pimkSrc->Save(pStm, fClearDirty); 
}

HRESULT STDMETHODCALLTYPE 
BufferedStreamMoniker::BindToStorage( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
{ 
    HRESULT hr = S_OK;
    IUnknown *pIUnk = NULL;
    IXMLViewerIdentity *pIVI = NULL;
    LPBC pbcXML = NULL;
    Viewer *pCV;
    VARIANT var;
    BOOL fStmCreated = FALSE;

    if (!_pStm) 
    {       
        // coming from trident, initiate the load
        fStmCreated = TRUE;

        _pStm = new_ne MIMEBufferedStream();
        CHECKALLOC(hr, _pStm);
//      _pStm->AddRef();         already ref'ed by new_ne for nonGC object

        // get pointer to the viewer object
        if (!pbc)
        {
            hr = E_FAIL;        // probably should be an assert, but don't assume anything about trident
            goto CleanUp;
        }
        hr = pbc->GetObjectParam(L"AGG", &pIUnk);
        CHECKHR(hr);
        hr = pIUnk->QueryInterface(IID_IXMLViewerIdentity, (void **)&pIVI);
        CHECKHR(hr);
        VariantInit(&var);
        hr = pIVI->get_XMLViewerObject(&var);
        CHECKHR(hr);
        pCV = (Viewer *)(V_BYREF(&var));
        
#if MIMEASYNC
        // if we're in the middle of a download on this object
        // we have to wait for the worker thread to unwind itself 
        // before we can reinitiate the download.
        HANDLE evt = pCV->getLoadCompleteEvent();
        if (evt)
            WaitForSingleObject(evt, INFINITE);
#endif        
        // finally, call the viewer to do the load
        hr = pCV->reloadFromTrident(_pStm, _pimkSrc, pbc);
        CHECKHR(hr);
    }

    // we're saying we've bound but we actually don't have any data yet
    // or have returned OnStartBinding.  If we return S_OK, Trident
    // thinks all the data is avail and tries to read immediately
    // upon return.  This causes ASSERTs and subsequent errors in MSHTML.
    hr = S_ASYNCHRONOUS;
    *ppvObj = NULL;

CleanUp:
    if (pbc)
        HRESULT hr2 = pbc->RevokeObjectParam(L"AGG");
    if (fStmCreated && !SUCCEEDED(hr))
        SafeRelease(_pStm);
    SafeRelease(pIUnk);
    SafeRelease(pIVI);
    SafeRelease(pbcXML);
    return hr;
}
    