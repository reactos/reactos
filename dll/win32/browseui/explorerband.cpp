#include "precomp.h"

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

extern "C"
HRESULT WINAPI CExplorerBand_Constructor(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CExplorerBand>(riid, ppv);
}

CExplorerBand::CExplorerBand() :
    m_OnWinEventShown(FALSE)
{
#if 0
    HRESULT hResult = CoCreateInstance(CLSID_ExplorerBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &m_internalBand));
    if (FAILED(hResult))
    {
        ERR("Could not create internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IDeskBand, &m_internalDeskBand));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IDeskBand from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_internalObjectWithSite));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IObjectWithSite from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IInputObject, &m_internalInputObject));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IInputObject from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IPersistStream, &m_internalPersistStream));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IPersistStream from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_internalOleCommandTarget));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IOleCommandTarget from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_internalServiceProvider));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IServiceProvider from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        m_internalServiceProvider = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IBandNavigate, &m_internalBandNavigate));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IBandNavigate from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        m_internalServiceProvider = NULL;
        m_internalBandNavigate = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_internalWinEventHandler));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface IWinEventHandler from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        m_internalServiceProvider = NULL;
        m_internalBandNavigate = NULL;
        m_internalWinEventHandler = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(INamespaceProxy, &m_internalNamespaceProxy));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface INamespaceProxy from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        m_internalServiceProvider = NULL;
        m_internalBandNavigate = NULL;
        m_internalWinEventHandler = NULL;
        m_internalNamespaceProxy = NULL;
        return;
    }
    hResult = m_internalBand->QueryInterface(IID_PPV_ARG(IDispatch, &m_internalDispatch));
    if (FAILED(hResult))
    {
        ERR("Could not obtain interface INamespaceProxy from internal band (hr=%08lx).\n", hResult);
        m_internalBand = NULL;
        m_internalDeskBand = NULL;
        m_internalObjectWithSite = NULL;
        m_internalInputObject = NULL;
        m_internalPersistStream = NULL;
        m_internalOleCommandTarget = NULL;
        m_internalServiceProvider = NULL;
        m_internalBandNavigate = NULL;
        m_internalWinEventHandler = NULL;
        m_internalNamespaceProxy = NULL;
        m_internalDispatch = NULL;
        return;
    }
#endif
}

CExplorerBand::~CExplorerBand()
{
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetWindow(HWND *lphwnd)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetWindow(HWND *lphwnd=%p)\n", lphwnd);
        hr = m_internalDeskBand->GetWindow(lphwnd);
        if (lphwnd)
            DbgPrint("\t*lphwnd=%p\n", *lphwnd);
        DbgPrint("CExplorerBand::GetWindow returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::ContextSensitiveHelp(BOOL fEnterMode=%s)\n", fEnterMode ? "TRUE" : "FALSE");
        hr = m_internalDeskBand->ContextSensitiveHelp(fEnterMode);
        DbgPrint("CExplorerBand::ContextSensitiveHelp returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::CloseDW(DWORD dwReserved)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::CloseDW(DWORD dwReserved=%u)\n", dwReserved);
        hr = m_internalDeskBand->CloseDW(dwReserved);
        DbgPrint("CExplorerBand::CloseDW returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::ResizeBorderDW(const RECT *prcBorder=%p, IUnknown *punkToolbarSite=%p, BOOL fReserved=%s)\n",
                 prcBorder, punkToolbarSite, fReserved ? "TRUE" : "FALSE");
        if (prcBorder)
            DbgPrint("\t*prcBorder={%u, %u, %u, %u}\n", prcBorder->left, prcBorder->top, prcBorder->right, prcBorder->bottom);
        hr = m_internalDeskBand->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
        DbgPrint("CExplorerBand::ResizeBorderDW returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ShowDW(BOOL fShow)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::ShowDW(BOOL fShow=%s)\n", fShow ? "TRUE" : "FALSE");
        hr = m_internalDeskBand->ShowDW(fShow);
        DbgPrint("CExplorerBand::ShowDW returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    HRESULT hr;
    if (m_internalDeskBand)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetBandInfo(DWORD dwBandID=%u, DWORD dwViewMode=%u, DESKBANDINFO *pdbi=%p)\n", 
                 dwBandID, dwViewMode, pdbi);
        if (pdbi)
            DbgPrint("\t*pdbi={dwMask=%u, ...}\n", pdbi->dwMask);
        hr = m_internalDeskBand->GetBandInfo(dwBandID, dwViewMode, pdbi);
        if (pdbi)
            DbgPrint("\t*pdbi={dwMask=%u, crBkgnd=%u, ...}\n", pdbi->dwMask, pdbi->crBkgnd);
        DbgPrint("CExplorerBand::GetBandInfo returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;
    if (m_internalObjectWithSite)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::SetSite(IUnknown *pUnkSite=%p)\n", pUnkSite);
        hr = m_internalObjectWithSite->SetSite(pUnkSite);
        DbgPrint("CExplorerBand::SetSite returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSite(REFIID riid, void **ppvSite)
{
    HRESULT hr;
    if (m_internalObjectWithSite)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetSite(REFIID riid=%s, void **ppvSite=%p)\n", wine_dbgstr_guid(&riid), ppvSite);
        hr = m_internalObjectWithSite->GetSite(riid, ppvSite);
        if (ppvSite)
            DbgPrint("\t*ppvSite=%p\n", *ppvSite);
        DbgPrint("CExplorerBand::GetSite returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    HRESULT hr;
    if (m_internalOleCommandTarget)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::QueryStatus(const GUID *pguidCmdGroup=%s, ULONG cCmds=%08x, OLECMD prgCmds []=%p, OLECMDTEXT *pCmdText=%p)\n",
                 wine_dbgstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);
        hr = m_internalOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
        DbgPrint("CExplorerBand::QueryStatus returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HRESULT hr;
    if (m_internalOleCommandTarget)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::Exec(const GUID *pguidCmdGroup=%s, DWORD nCmdID=%u, DWORD nCmdexecopt=%u, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n",
                 wine_dbgstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, pvaIn, pvaOut);
        hr = m_internalOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
        DbgPrint("CExplorerBand::Exec returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr;
    if (m_internalServiceProvider)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n",
                 wine_dbgstr_guid(&guidService), wine_dbgstr_guid(&riid), ppvObject);
        hr = m_internalServiceProvider->QueryService(guidService, riid, ppvObject);
        if (ppvObject)
            DbgPrint("\t*ppvObject=%p\n", *ppvObject);
        DbgPrint("CExplorerBand::QueryService returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr;
    if (m_internalInputObject)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::UIActivateIO(BOOL fActivate=%s, LPMSG lpMsg=%p)\n",
                 fActivate ? "TRUE" : "FALSE", lpMsg);
        hr = m_internalInputObject->UIActivateIO(fActivate, lpMsg);
        if (lpMsg)
            DbgPrint("\t*lpMsg={hwnd=%p, message=%x, wParam=%x, lParam=%x, time=%x, pt={%u %u}}\n",
            lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam, lpMsg->time, lpMsg->pt.x, lpMsg->pt.y);
        DbgPrint("CExplorerBand::UIActivateIO returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::HasFocusIO()
{
    HRESULT hr;
    if (m_internalInputObject)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::HasFocusIO()\n");
        hr = m_internalInputObject->HasFocusIO();
        DbgPrint("CExplorerBand::HasFocusIO returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    HRESULT hr;
    if (m_internalInputObject)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", lpMsg);
        hr = m_internalInputObject->TranslateAcceleratorIO(lpMsg);
        if (lpMsg)
            DbgPrint("\t*lpMsg={hwnd=%p, message=%x, wParam=%x, lParam=%x, time=%x, pt={%u %u}}\n",
            lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam, lpMsg->time, lpMsg->pt.x, lpMsg->pt.y);
        DbgPrint("CExplorerBand::TranslateAcceleratorIO returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetClassID(CLSID *pClassID)
{
    HRESULT hr;
    if (m_internalPersistStream)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetClassID(CLSID *pClassID=%p)\n", pClassID);
        hr = m_internalPersistStream->GetClassID(pClassID);
        if (pClassID)
            DbgPrint("\t*pClassID=%s\n", wine_dbgstr_guid(pClassID));
        DbgPrint("CExplorerBand::GetClassID returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::IsDirty()
{
    HRESULT hr;
    if (m_internalPersistStream)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::IsDirty()\n");
        hr = m_internalPersistStream->IsDirty();
        DbgPrint("CExplorerBand::IsDirty returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Load(IStream *pStm)
{
    HRESULT hr;
    if (m_internalPersistStream)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::Load(IStream *pStm=%p)\n", pStm);
        hr = m_internalPersistStream->Load(pStm);
        DbgPrint("CExplorerBand::Load returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;
    if (m_internalPersistStream)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::Save(IStream *pStm=%p, BOOL fClearDirty=%s)\n",
                 pStm, fClearDirty ? "TRUE" : "FALSE");
        hr = m_internalPersistStream->Save(pStm, fClearDirty);
        DbgPrint("CExplorerBand::Save returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    HRESULT hr;
    if (m_internalPersistStream)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetSizeMax(ULARGE_INTEGER *pcbSize=%p)\n", pcbSize);
        hr = m_internalPersistStream->GetSizeMax(pcbSize);
        if (pcbSize)
            DbgPrint("\t*pcbSize=%llx\n", pcbSize->QuadPart);
        DbgPrint("CExplorerBand::GetSizeMax returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr;
    if (m_internalWinEventHandler)
    {
        if (m_OnWinEventShown)
            return m_internalWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
        m_OnWinEventShown = TRUE;

        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::OnWinEvent(HWND hWnd=%x, UINT uMsg=%x, WPARAM wParam=%x, LPARAM lParam=%x, LRESULT *theResult=%p)\n",
                 hWnd, uMsg, wParam, lParam, theResult);
        hr = m_internalWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
        if (theResult)
            DbgPrint("\t*theResult=%x\n", *theResult);
        DbgPrint("CExplorerBand::OnWinEvent returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::IsWindowOwner(HWND hWnd)
{
    HRESULT hr;
    if (m_internalWinEventHandler)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::IsWindowOwner(HWND hWnd=%x)\n", hWnd);
        hr = m_internalWinEventHandler->IsWindowOwner(hWnd);
        DbgPrint("CExplorerBand::IsWindowOwner returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IBandNavigate methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::Select(long paramC)
{
    HRESULT hr;
    if (m_internalBandNavigate)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::Select(long paramC=%x)\n", paramC);
        hr = m_internalBandNavigate->Select(paramC);
        DbgPrint("CExplorerBand::Select returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** INamespaceProxy ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetNavigateTarget(long paramC, long param10, long param14)
{
    HRESULT hr;
    if (m_internalNamespaceProxy)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetNavigateTarget(long paramC=%08x, long param10=%08x, long param14=%08x)\n",
                 paramC, param10, param14);
        hr = m_internalNamespaceProxy->GetNavigateTarget(paramC, param10, param14);
        DbgPrint("CExplorerBand::GetNavigateTarget returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(long paramC)
{
    HRESULT hr;
    if (m_internalNamespaceProxy)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(long paramC=%08x)\n", paramC);
        hr = m_internalNamespaceProxy->Invoke(paramC);
        DbgPrint("CExplorerBand::Invoke returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::OnSelectionChanged(long paramC)
{
    HRESULT hr;
    if (m_internalNamespaceProxy)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::OnSelectionChanged(long paramC=%08x)\n", paramC);
        hr = m_internalNamespaceProxy->OnSelectionChanged(paramC);
        DbgPrint("CExplorerBand::OnSelectionChanged returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::RefreshFlags(long paramC, long param10, long param14)
{
    HRESULT hr;
    if (m_internalNamespaceProxy)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::RefreshFlags(long paramC=%08x, long param10=%08x, long param14=%08x)\n",
                 paramC, param10, param14);
        hr = m_internalNamespaceProxy->RefreshFlags(paramC, param10, param14);
        DbgPrint("CExplorerBand::RefreshFlags returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::CacheItem(long paramC)
{
    HRESULT hr;
    if (m_internalNamespaceProxy)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::CacheItem(long paramC=%08x)\n", paramC);
        hr = m_internalNamespaceProxy->CacheItem(paramC);
        DbgPrint("CExplorerBand::CacheItem returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfoCount(UINT *pctinfo)
{
    HRESULT hr;
    if (m_internalDispatch)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfoCount(UINT *pctinfo=%p)\n", pctinfo);
        hr = m_internalDispatch->GetTypeInfoCount(pctinfo);
        if (pctinfo)
            DbgPrint("\t*pctinfo=%08x\n", *pctinfo);
        DbgPrint("CExplorerBand::GetTypeInfoCount returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HRESULT hr;
    if (m_internalDispatch)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfo(UINT iTInfo=%u, LCID lcid=%08x, ITypeInfo **ppTInfo=%p)\n",
                 iTInfo, lcid, ppTInfo);
        hr = m_internalDispatch->GetTypeInfo(iTInfo, lcid, ppTInfo);
        if (ppTInfo)
            DbgPrint("\t*ppTInfo=%08x\n", *ppTInfo);
        DbgPrint("CExplorerBand::GetTypeInfo returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HRESULT hr;
    if (m_internalDispatch)
    {
        DbgPrint("HRESULT STDMETHODCALLTYPE CExplorerBand::GetIDsOfNames(REFIID riid=%s, LPOLESTR *rgszNames=%S, UINT cNames=%u, LCID lcid=%08x, DISPID *rgDispId=%p)\n",
                 wine_dbgstr_guid(&riid), rgszNames, cNames, lcid, rgDispId);
        hr = m_internalDispatch->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
        if (rgDispId && SUCCEEDED(hr))
        {
            for (UINT i = 0; i < cNames; i++)
            {
                DbgPrint("\trgDispId[%d]=%08x\n", rgDispId[i]);
            }
        }
        DbgPrint("CExplorerBand::GetTypeInfo returning %08lx\n", hr);
        return hr;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

