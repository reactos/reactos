/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop
#include "callback.hxx"
#include "viewerfactory.hxx"
#include "xmlview.hxx"

#ifdef _DEBUG
static LONG g_cCallbackWrappers = 0;
static LONG g_cCallbackMonitors = 0;
static LONG g_cCBindings = 0;
#endif

//=======================================================================
CallbackWrapper::CallbackWrapper()
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cCallbackWrappers);
#endif
    _pbs = NULL;
}

CallbackWrapper::~CallbackWrapper()
{        
#ifdef _DEBUG
    InterlockedDecrement(&g_cCallbackWrappers);
#endif
    SafeRelease(_pbs);
}

void 
CallbackWrapper::SetPreviousCallback(IBindStatusCallback* pbs)
{
    _pbs = pbs;
    if (_pbs)
        _pbs->AddRef();
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnStartBinding(
    /* [in] */ DWORD grfBSCOption,
    /* [in] */ IBinding *pib)
{
    if (_pbs)       
        return _pbs->OnStartBinding(grfBSCOption, pib);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::GetPriority(
   /* [out] */ LONG *pnPriority)
{
    if (_pbs)       
        return _pbs->GetPriority(pnPriority);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnLowResource(
    /* [in] */ DWORD reserved)
{
    if (_pbs)       
        return _pbs->OnLowResource(reserved);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnProgress(
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText)
{
    if (_pbs)       
        return _pbs->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnStopBinding(
    /* [in] */ HRESULT hresult,
    /* [in] */ LPCWSTR szError)
{
    if (_pbs)       
        return _pbs->OnStopBinding(hresult, szError);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::GetBindInfo(
    /* [out] */ DWORD *grfBINDF,
    /* [unique][out][in] */ BINDINFO *pbindinfo)
{
    if (_pbs)       
        return _pbs->GetBindInfo(grfBINDF, pbindinfo);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnDataAvailable(
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC *pformatetc,
    /* [in] */ STGMEDIUM *pstgmed)
{
    if (_pbs)       
        return _pbs->OnDataAvailable(grfBSCF, dwSize, pformatetc, pstgmed);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackWrapper::OnObjectAvailable(
    /* [in] */ REFIID iid,
    /* [iid_is][in] */ IUnknown *punknown)
{
    if (_pbs)       
        return _pbs->OnObjectAvailable(iid, punknown);
    return S_OK;
}


//=======================================================================
CallbackMonitor::CallbackMonitor()
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cCallbackMonitors);
#endif
    _pbs = NULL;
#if MIMEASYNC
    _fAsync = FALSE;
#endif
}

CallbackMonitor::~CallbackMonitor()
{        
#ifdef _DEBUG
    InterlockedDecrement(&g_cCallbackMonitors);
#endif
    SafeRelease(_pbs);
}

void 
CallbackMonitor::SetPreviousCallback(IBindStatusCallback* pbs)
{
    _pbs = pbs;
    if (_pbs)
        _pbs->AddRef();
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnStartBinding(
    /* [in] */ DWORD grfBSCOption,
    /* [in] */ IBinding *pib)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::GetPriority(
   /* [out] */ LONG *pnPriority)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnLowResource(
    /* [in] */ DWORD reserved)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnProgress(
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText)
{
    // BUGBUG - anything else to filter out ?
    if (ulStatusCode != BINDSTATUS_REDIRECTING && _pbs)
        return _pbs->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnStopBinding(
    /* [in] */ HRESULT hresult,
    /* [in] */ LPCWSTR szError)
{
#if MIMEASYNC
    if (_fAsync)
        return E_PENDING;
#endif    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::GetBindInfo(
    /* [out] */ DWORD *grfBINDF,
    /* [unique][out][in] */ BINDINFO *pbindinfo)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnDataAvailable(
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC *pformatetc,
    /* [in] */ STGMEDIUM *pstgmed)
{
#if MIMEASYNC
    if (_fAsync)
        return E_PENDING;
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
CallbackMonitor::OnObjectAvailable(
    /* [in] */ REFIID iid,
    /* [iid_is][in] */ IUnknown *punknown)
{
    return S_OK;
}


//=======================================================================
CBinding::CBinding()
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cCBindings);
#endif
    _pNF = NULL;
    _pViewer = NULL;
}

CBinding::~CBinding()
{
#ifdef _DEBUG
    InterlockedDecrement(&g_cCBindings);
#endif
}

HRESULT STDMETHODCALLTYPE 
CBinding::Abort( void)
{
    // ABORT strategy
    // If we're on the worker thread, we tell the factory to stop. THe factory returns E_FAIL which
    // will bubble up through this invocation of Parser->Run.
    //
    // Otherwise we must be still processing on the main thread, but URLMON has come back async.
    // (That is, we have returned from doc.load, but are not finished processing the xml prolog, 
    // which occurs on the main thread).  In this case we simply abort the document.
    Viewer *pViewer = _pViewer;
#if MIMEASYNC
    if (_pNF && _pNF->isAsync())
    {
        Assert(_pViewer);
        HANDLE evt = _pViewer->getLoadCompleteEvent();
        _pNF->StopAsync();
        // wait for it to finish
        if (evt)
            MsgWaitForDownloadObjects(1, &evt, FALSE, INFINITE);
    }
// else
#endif
    if (pViewer)
        pViewer->stopDownload();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE 
CBinding::Suspend( void)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE 
CBinding::Resume( void)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
CBinding::SetPriority( 
    /* [in] */ LONG nPriority)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
CBinding::GetPriority( 
    /* [out] */ LONG __RPC_FAR *pnPriority)
{
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE 
CBinding::GetBindResult( 
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved)
{
    return E_NOTIMPL;
}

void
CBinding::SetAbortCB(Viewer *pViewer, ViewerFactory *pNodeFactory)
{
    // Note: we specifically do not AddRef here otherwise this would
    // cause circularity (from Viewer->ViewerFactory->BufferedStream->Binding)
    // we are guaranteed that these are alive as long as the binding is.
    // If we ref the viewer too many times, then we will not exit properly
    // if the user exits IE directly, since it Releases the OleObject
    // and expects it to go away. THe only refs at that point are
    // one from the shell and the other from trident via punkOuter.
    Assert(!_pNF || !pNodeFactory);      // can only set from or to null
    _pNF = pNodeFactory;
    Assert(!_pViewer || !pViewer);      // can only set from or to null
    _pViewer = pViewer;
}

IUnknown*
CBinding::getTrident()
{ 
    if (_pViewer)
        return _pViewer->getTrident();
    return null;
}
