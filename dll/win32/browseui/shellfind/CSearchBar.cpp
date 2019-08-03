/*
 * ReactOS Explorer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "CSearchBar.h"
#include <psdk/wingdi.h>

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

CSearchBar::CSearchBar() :
    pSite(NULL),
    fVisible(FALSE),
    bFocused(FALSE)
{
}

CSearchBar::~CSearchBar()
{
}

void CSearchBar::InitializeSearchBar()
{
    CreateWindowExW(0, WC_STATIC, L"Search by any or all of the criteria below.",
        WS_CHILD | WS_VISIBLE,
        10, 10, 200, 40,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);

    CreateWindowExW(0, WC_STATIC, L"A &word or phrase in the file:",
        WS_CHILD | WS_VISIBLE,
        10, 50, 500, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        10, 70, 100, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);

    CreateWindowExW(0, WC_STATIC, L"&Look in:",
        WS_CHILD | WS_VISIBLE,
        10, 100, 500, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        10, 120, 100, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);

    CreateWindowExW(0, WC_STATIC, L"&Look in:",
        WS_CHILD | WS_VISIBLE,
        10, 150, 500, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL,
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        10, 180, 100, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);

    CreateWindowExW(0, WC_BUTTON, L"Sea&rch",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        10, 210, 100, 20,
        m_hWnd, NULL,
        _AtlBaseModule.GetModuleInstance(), NULL);
}

HRESULT CSearchBar::ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd)
{
    CComPtr<IOleWindow>                 pBrowserOleWnd;
    CMINVOKECOMMANDINFO                 cmi;
    HWND                                browserWnd;
    HRESULT                             hr;

    hr = IUnknown_QueryService(pSite, SID_SShellBrowser, IID_PPV_ARG(IOleWindow, &pBrowserOleWnd));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBrowserOleWnd->GetWindow(&browserWnd);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    ZeroMemory(&cmi, sizeof(cmi));
    cmi.cbSize = sizeof(cmi);
    cmi.lpVerb = MAKEINTRESOURCEA(nCmd);
    cmi.hwnd = browserWnd;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        cmi.fMask |= CMIC_MASK_CONTROL_DOWN;

    return menu->InvokeCommand(&cmi);
}


// *** ATL event handlers ***
LRESULT CSearchBar::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bFocused = TRUE;
    IUnknown_OnFocusChangeIS(pSite, reinterpret_cast<IUnknown*>(this), TRUE);
    bHandled = FALSE;
    return TRUE;
}

HRESULT CSearchBar::GetSearchResultsFolder(IShellBrowser **ppShellBrowser, HWND *pHwnd, IShellFolder **ppShellFolder)
{
    HRESULT hr;
    CComPtr<IShellBrowser> pShellBrowser;
    if (!ppShellBrowser)
        ppShellBrowser = &pShellBrowser;
    if (!*ppShellBrowser)
    {
        hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, ppShellBrowser));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    CComPtr<IShellView> pShellView;
    hr = (*ppShellBrowser)->QueryActiveShellView(&pShellView);
    if (FAILED(hr) || !pShellView)
        return hr;

    CComPtr<IFolderView> pFolderView;
    hr = pShellView->QueryInterface(IID_PPV_ARG(IFolderView, &pFolderView));
    if (FAILED(hr) || !pFolderView)
        return hr;

    CComPtr<IShellFolder> pShellFolder;
    if (!ppShellFolder)
        ppShellFolder = &pShellFolder;
    hr = pFolderView->GetFolder(IID_PPV_ARG(IShellFolder, ppShellFolder));
    if (FAILED(hr) || !pShellFolder)
        return hr;

    CLSID clsid;
    hr = IUnknown_GetClassID(*ppShellFolder, &clsid);
    if (FAILED(hr))
        return hr;
    if (clsid != CLSID_FindFolder)
        return E_FAIL;

    if (pHwnd)
    {
        hr = pShellView->GetWindow(pHwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

LRESULT CSearchBar::OnSearchButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CComPtr<IShellBrowser> pShellBrowser;
    HRESULT hr = IUnknown_QueryService(pSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &pShellBrowser));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    HWND hwnd;
    if (FAILED(GetSearchResultsFolder(&pShellBrowser, &hwnd, NULL)))
    {
        WCHAR szShellGuid[MAX_PATH];
        const WCHAR shellGuidPrefix[] = L"shell:::";
        memcpy(szShellGuid, shellGuidPrefix, sizeof(shellGuidPrefix));
        hr = StringFromGUID2(CLSID_FindFolder, szShellGuid + _countof(shellGuidPrefix) - 1,
                             _countof(szShellGuid) - _countof(shellGuidPrefix));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        CComHeapPtr<ITEMIDLIST> findFolderPidl;
        hr = SHParseDisplayName(szShellGuid, NULL, &findFolderPidl, 0, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pShellBrowser->BrowseObject(findFolderPidl, 0);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    GetSearchResultsFolder(&pShellBrowser, &hwnd, NULL);
    if (hwnd)
    ::PostMessageW(hwnd, WM_SEARCH_START, 0, (LPARAM) StrDupW(L"Starting search..."));

    return S_OK;
}

LRESULT CSearchBar::OnClicked(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HWND hwnd;
    HRESULT hr = GetSearchResultsFolder(NULL, &hwnd, NULL);
    if (SUCCEEDED(hr))
    {
        LPCWSTR path = L"C:\\readme.txt";
        ::PostMessageW(hwnd, WM_SEARCH_ADD_RESULT, 0, (LPARAM) StrDupW(path));
    }

    return 0;
}


// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetWindow(HWND *lphwnd)
{
    if (!lphwnd)
        return E_INVALIDARG;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::CloseDW(DWORD dwReserved)
{
    // We do nothing, we don't have anything to save yet
    TRACE("CloseDW called\n");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* Must return E_NOTIMPL according to MSDN */
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::ShowDW(BOOL fShow)
{
    fVisible = fShow;
    ShowWindow(fShow);
    return S_OK;
}


// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!pdbi)
    {
        return E_INVALIDARG;
    }

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 200;
        pdbi->ptMinSize.y = 30;
    }

    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.y = -1;
    }

    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.y = 1;
    }

    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 200;
        pdbi->ptActual.y = 30;
    }

    if (pdbi->dwMask & DBIM_TITLE)
    {
        if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_SEARCHLABEL, pdbi->wszTitle, _countof(pdbi->wszTitle)))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
    {
        pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
    }

    if (pdbi->dwMask & DBIM_BKCOLOR)
    {
        pdbi->dwMask &= ~DBIM_BKCOLOR;
    }
    return S_OK;
}

LRESULT CALLBACK MyWindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    return 0;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;
    HWND parentWnd;

    if (pUnkSite == pSite)
        return S_OK;

    TRACE("SetSite called \n");
    if (!pUnkSite)
    {
        DestroyWindow();
        m_hWnd = NULL;
    }

    if (pUnkSite != pSite)
    {
        pSite = NULL;
    }

    if(!pUnkSite)
        return S_OK;

    hr = IUnknown_GetWindow(pUnkSite, &parentWnd);
    if (!SUCCEEDED(hr))
    {
        ERR("Could not get parent's window ! Status: %08lx\n", hr);
        return E_INVALIDARG;
    }

    pSite = pUnkSite;

    if (m_hWnd)
    {
        // Change its parent
        SetParent(parentWnd);
    }
    else
    {
        CWindowImpl::Create(parentWnd);

        InitializeSearchBar();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    *ppvSite = pSite;
    return S_OK;
}


// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    /* FIXME: we probably want to handle more services here */
    return IUnknown_QueryService(pSite, SID_SShellBrowser, riid, ppvObject);
}


// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        //SetFocus();
        SetActiveWindow();
    }
    // TODO: handle message
    if(lpMsg)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::HasFocusIO()
{
    return bFocused ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CSearchBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (lpMsg->hwnd == m_hWnd)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }

    return S_FALSE;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    memcpy(pClassID, &CLSID_FileSearchBand, sizeof(CLSID));
    return S_OK;
}


// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // TODO: calculate max size
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSearchBar::IsWindowOwner(HWND hWnd)
{
    return (hWnd == m_hWnd) ? S_OK : S_FALSE;
}

// *** IBandNavigate methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::Select(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** INamespaceProxy ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetNavigateTarget(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Invoke(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::OnSelectionChanged(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::RefreshFlags(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::CacheItem(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CSearchBar::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSearchBar::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    TRACE("Unknown dispid requested: %08x\n", dispIdMember);
    return E_INVALIDARG;
}
