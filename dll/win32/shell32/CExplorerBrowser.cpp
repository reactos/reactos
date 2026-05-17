/*
 * IExplorerBrowser implementation
 *
 * Copyright 2010 David Hedberg (Wine)
 * Copyright 2024 ReactOS Contributors
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CExplorerBrowser::CExplorerBrowser() :
    m_hwnd(NULL),
    m_hwndSV(NULL),
    m_pidl(NULL),
    m_dwOptions((EXPLORER_BROWSER_OPTIONS)0),
    m_dwEventsCookie(0)
{
    ZeroMemory(&m_fs, sizeof(m_fs));
    m_fs.ViewMode = FVM_DETAILS;
}

CExplorerBrowser::~CExplorerBrowser()
{
    Destroy();
    ILFree(m_pidl);
}

/* Navigate to the given absolute PIDL, creating/replacing the shell view */
HRESULT CExplorerBrowser::Navigate(PCIDLIST_ABSOLUTE pidl)
{
    HRESULT hr;
    CComPtr<IShellFolder> psf;
    CComPtr<IShellView> psvNew;
    HWND hwndSVNew = NULL;
    RECT rc = {0};

    if (!m_hwnd)
        return E_FAIL;

    /* Notify pending navigation — handler may cancel by returning failure */
    if (m_pEvents)
    {
        hr = m_pEvents->OnNavigationPending(pidl);
        if (FAILED(hr))
            return hr;
    }

    /* Bind to the shell folder for this PIDL */
    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    if (_ILIsDesktop(pidl))
    {
        psf = psfDesktop;
    }
    else
    {
        hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED(hr))
        {
            if (m_pEvents)
                m_pEvents->OnNavigationFailed(pidl);
            return hr;
        }
    }

    /* Create new shell view */
    hr = psf->CreateViewObject(m_hwnd, IID_PPV_ARG(IShellView, &psvNew));
    if (FAILED(hr))
    {
        if (m_pEvents)
            m_pEvents->OnNavigationFailed(pidl);
        return hr;
    }

    /* Notify view created */
    if (m_pEvents)
        m_pEvents->OnViewCreated(psvNew);

    /* Get rect for view window */
    GetClientRect(m_hwnd, &rc);

    hr = psvNew->CreateViewWindow(m_psv, &m_fs, static_cast<IShellBrowser*>(this), &rc, &hwndSVNew);
    if (FAILED(hr) || !hwndSVNew)
    {
        if (m_pEvents)
            m_pEvents->OnNavigationFailed(pidl);
        return FAILED(hr) ? hr : E_FAIL;
    }

    /* Destroy old view */
    if (m_psv)
    {
        m_psv->UIActivate(SVUIA_DEACTIVATE);
        m_psv->DestroyViewWindow();
        m_psv.Release();
        m_hwndSV = NULL;
    }

    /* Activate new view */
    m_psv = psvNew;
    m_hwndSV = hwndSVNew;

    ShowWindow(m_hwndSV, SW_SHOW);
    m_psv->UIActivate(SVUIA_ACTIVATE_NOFOCUS);

    /* Update stored PIDL */
    ILFree(m_pidl);
    m_pidl = ILClone(pidl);

    /* Notify complete */
    if (m_pEvents)
        m_pEvents->OnNavigationComplete(pidl);

    return S_OK;
}

/*************************************************************************
 * IExplorerBrowser
 */

STDMETHODIMP CExplorerBrowser::Initialize(HWND hwndParent, const RECT *prc, const FOLDERSETTINGS *pfs)
{
    TRACE("(%p, %p, %p, %p)\n", this, hwndParent, prc, pfs);

    if (m_hwnd)
        return E_UNEXPECTED;

    if (!hwndParent)
        return E_INVALIDARG;

    if (pfs)
        m_fs = *pfs;

    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    if (m_dwOptions & EBO_NOBORDER)
        dwStyle &= ~WS_BORDER;

    m_hwnd = CreateWindowExW(
        0,
        WC_STATIC,
        NULL,
        dwStyle,
        prc ? prc->left   : 0,
        prc ? prc->top    : 0,
        prc ? (prc->right - prc->left) : 0,
        prc ? (prc->bottom - prc->top) : 0,
        hwndParent,
        NULL,
        shell32_hInstance,
        NULL);

    if (!m_hwnd)
    {
        ERR("Failed to create browser window\n");
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CExplorerBrowser::Destroy()
{
    TRACE("(%p)\n", this);

    if (m_psv)
    {
        m_psv->UIActivate(SVUIA_DEACTIVATE);
        m_psv->DestroyViewWindow();
        m_psv.Release();
        m_hwndSV = NULL;
    }

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetRect(HDWP *phdwp, RECT rcBrowser)
{
    TRACE("(%p, %p, ...)\n", this, phdwp);

    if (!m_hwnd)
        return E_FAIL;

    if (phdwp)
    {
        *phdwp = DeferWindowPos(*phdwp, m_hwnd, NULL,
            rcBrowser.left, rcBrowser.top,
            rcBrowser.right - rcBrowser.left,
            rcBrowser.bottom - rcBrowser.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else
    {
        SetWindowPos(m_hwnd, NULL,
            rcBrowser.left, rcBrowser.top,
            rcBrowser.right - rcBrowser.left,
            rcBrowser.bottom - rcBrowser.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (m_hwndSV)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        SetWindowPos(m_hwndSV, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetPropertyBag(LPCWSTR pszPropertyBag)
{
    FIXME("(%p, %s) stub\n", this, debugstr_w(pszPropertyBag));
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetEmptyText(LPCWSTR pszEmptyText)
{
    FIXME("(%p, %s) stub\n", this, debugstr_w(pszEmptyText));
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetFolderSettings(const FOLDERSETTINGS *pfs)
{
    TRACE("(%p, %p)\n", this, pfs);
    if (!pfs)
        return E_INVALIDARG;
    m_fs = *pfs;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::Advise(IExplorerBrowserEvents *psbe, DWORD *pdwCookie)
{
    TRACE("(%p, %p, %p)\n", this, psbe, pdwCookie);
    if (!psbe || !pdwCookie)
        return E_INVALIDARG;

    m_pEvents = psbe;
    *pdwCookie = ++m_dwEventsCookie;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::Unadvise(DWORD dwCookie)
{
    TRACE("(%p, %u)\n", this, dwCookie);
    if (dwCookie == m_dwEventsCookie)
        m_pEvents.Release();
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetOptions(EXPLORER_BROWSER_OPTIONS dwFlag)
{
    TRACE("(%p, 0x%x)\n", this, dwFlag);
    m_dwOptions = dwFlag;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::GetOptions(EXPLORER_BROWSER_OPTIONS *pdwFlag)
{
    TRACE("(%p, %p)\n", this, pdwFlag);
    if (!pdwFlag)
        return E_INVALIDARG;
    *pdwFlag = m_dwOptions;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::BrowseToIDList(PCUIDLIST_RELATIVE pidl, UINT uFlags)
{
    TRACE("(%p, %p, 0x%x)\n", this, pidl, uFlags);

    /* Back/forward navigation requires a history stack we don't implement */
    if (uFlags & (SBSP_NAVIGATEBACK | SBSP_NAVIGATEFORWARD))
        return S_OK;

    if (!m_pidl && !pidl)
        return E_INVALIDARG;

    LPITEMIDLIST pidlAbs;
    if (!pidl)
        return Navigate(m_pidl);

    if (m_pidl)
        pidlAbs = ILCombine(m_pidl, pidl);
    else
        pidlAbs = ILClone((PCIDLIST_ABSOLUTE)pidl);

    if (!pidlAbs)
        return E_OUTOFMEMORY;

    HRESULT hr = Navigate(pidlAbs);
    ILFree(pidlAbs);
    return hr;
}

STDMETHODIMP CExplorerBrowser::BrowseToObject(IUnknown *punk, UINT uFlags)
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;

    TRACE("(%p, %p, 0x%x)\n", this, punk, uFlags);

    if (!punk)
        return E_INVALIDARG;

    /* Try IPersistIDList first (IShellItem) */
    CComPtr<IPersistIDList> ppidl;
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistIDList, &ppidl))))
    {
        hr = ppidl->GetIDList(&pidl);
    }
    else
    {
        /* Try IPersistFolder2 (IShellFolder) */
        CComPtr<IPersistFolder2> ppf2;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2))))
        {
            hr = ppf2->GetCurFolder(&pidl);
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    if (FAILED(hr))
        return hr;

    hr = Navigate(pidl);
    ILFree(pidl);
    return hr;
}

STDMETHODIMP CExplorerBrowser::FillFromObject(IUnknown *punk, EXPLORER_BROWSER_FILL_FLAGS dwFlags)
{
    FIXME("(%p, %p, 0x%x) stub\n", this, punk, dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::RemoveAll()
{
    FIXME("(%p) stub\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::GetCurrentView(REFIID riid, void **ppv)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (!m_psv)
        return E_FAIL;

    /* Try to get IShellItemArray from the current view's selection */
    if (IsEqualIID(riid, IID_IShellItemArray))
    {
        CComPtr<IDataObject> pdo;
        HRESULT hr = m_psv->GetItemObject(SVGIO_SELECTION, IID_PPV_ARG(IDataObject, &pdo));
        if (SUCCEEDED(hr) && pdo)
            return SHCreateShellItemArrayFromDataObject(pdo, riid, ppv);
        /* No selection — return empty array via current folder */
        if (m_pidl)
        {
            PCUITEMID_CHILD empty = NULL;
            return SHCreateShellItemArray(m_pidl, NULL, 0, &empty, (IShellItemArray**)ppv);
        }
        return E_FAIL;
    }

    /* Try to QI the shell view */
    return m_psv->QueryInterface(riid, ppv);
}

/*************************************************************************
 * IShellBrowser (used by IShellView to call back into the host)
 */

STDMETHODIMP CExplorerBrowser::GetWindow(HWND *phwnd)
{
    TRACE("(%p, %p)\n", this, phwnd);
    if (!phwnd)
        return E_INVALIDARG;
    *phwnd = m_hwnd;
    return m_hwnd ? S_OK : E_FAIL;
}

STDMETHODIMP CExplorerBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::RemoveMenusSB(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::SetStatusTextSB(LPCWSTR pszStatusText)
{
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::EnableModelessSB(BOOL fEnable)
{
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::TranslateAcceleratorSB(MSG *pmsg, WORD wID)
{
    return S_FALSE;
}

STDMETHODIMP CExplorerBrowser::BrowseObject(PCUIDLIST_RELATIVE pidl, UINT wFlags)
{
    TRACE("(%p, %p, 0x%x)\n", this, pidl, wFlags);
    return BrowseToIDList(pidl, wFlags);
}

STDMETHODIMP CExplorerBrowser::GetViewStateStream(DWORD grfMode, IStream **ppStrm)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::GetControlWindow(UINT id, HWND *lphwnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBrowser::QueryActiveShellView(IShellView **ppshv)
{
    TRACE("(%p, %p)\n", this, ppshv);
    if (!ppshv)
        return E_INVALIDARG;
    if (!m_psv)
        return E_FAIL;
    *ppshv = m_psv;
    (*ppshv)->AddRef();
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::OnViewWindowActive(IShellView *ppshv)
{
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::SetToolbarItems(LPTBBUTTONSB lpButtons, UINT nButtons, UINT uFlags)
{
    return E_NOTIMPL;
}

/*************************************************************************
 * IObjectWithSite
 */

STDMETHODIMP CExplorerBrowser::SetSite(IUnknown *pUnkSite)
{
    TRACE("(%p, %p)\n", this, pUnkSite);
    m_pSite = pUnkSite;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::GetSite(REFIID riid, void **ppvSite)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvSite);
    if (!ppvSite)
        return E_INVALIDARG;
    *ppvSite = NULL;
    if (!m_pSite)
        return E_FAIL;
    return m_pSite->QueryInterface(riid, ppvSite);
}

/*************************************************************************
 * ICommDlgBrowser3 — forwards to the site's ICommDlgBrowser3
 * (itemdlg.c sets the site to FileDialogImpl which implements ICommDlgBrowser3)
 */

static HRESULT GetSiteCommDlgBrowser3(IUnknown *pSite, ICommDlgBrowser3 **ppcdb3)
{
    *ppcdb3 = NULL;
    if (!pSite)
        return E_FAIL;
    return pSite->QueryInterface(IID_PPV_ARG(ICommDlgBrowser3, ppcdb3));
}

STDMETHODIMP CExplorerBrowser::OnDefaultCommand(IShellView *ppshv)
{
    TRACE("(%p, %p)\n", this, ppshv);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->OnDefaultCommand(ppshv);
    return S_FALSE;
}

STDMETHODIMP CExplorerBrowser::OnStateChange(IShellView *ppshv, ULONG uChange)
{
    TRACE("(%p, %p, %u)\n", this, ppshv, uChange);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->OnStateChange(ppshv, uChange);
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::IncludeObject(IShellView *ppshv, PCUITEMID_CHILD pidl)
{
    TRACE("(%p, %p, %p)\n", this, ppshv, pidl);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->IncludeObject(ppshv, pidl);
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::Notify(IShellView *ppshv, DWORD dwNotifyType)
{
    TRACE("(%p, %p, %u)\n", this, ppshv, dwNotifyType);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->Notify(ppshv, dwNotifyType);
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::GetDefaultMenuText(IShellView *ppshv, WCHAR *pszText, INT cchMax)
{
    TRACE("(%p, %p, %p, %d)\n", this, ppshv, pszText, cchMax);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->GetDefaultMenuText(ppshv, pszText, cchMax);
    return S_FALSE;
}

STDMETHODIMP CExplorerBrowser::GetViewFlags(DWORD *pdwFlags)
{
    TRACE("(%p, %p)\n", this, pdwFlags);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->GetViewFlags(pdwFlags);
    if (pdwFlags) *pdwFlags = 0;
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::OnColumnClicked(IShellView *ppshv, int iColumn)
{
    TRACE("(%p, %p, %d)\n", this, ppshv, iColumn);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->OnColumnClicked(ppshv, iColumn);
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::GetCurrentFilter(LPWSTR pszFileSpec, int cchFileSpec)
{
    TRACE("(%p, %p, %d)\n", this, pszFileSpec, cchFileSpec);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->GetCurrentFilter(pszFileSpec, cchFileSpec);
    if (pszFileSpec && cchFileSpec > 0) pszFileSpec[0] = L'\0';
    return S_OK;
}

STDMETHODIMP CExplorerBrowser::OnPreviewCreated(IShellView *ppshv)
{
    TRACE("(%p, %p)\n", this, ppshv);
    CComPtr<ICommDlgBrowser3> pcdb3;
    if (SUCCEEDED(GetSiteCommDlgBrowser3(m_pSite, &pcdb3)))
        return pcdb3->OnPreviewCreated(ppshv);
    return S_OK;
}

/*************************************************************************
 * IServiceProvider
 */

STDMETHODIMP CExplorerBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    TRACE("(%p, %s, %s, %p)\n", this, debugstr_guid(&guidService), debugstr_guid(&riid), ppv);

    if (IsEqualGUID(guidService, SID_SShellBrowser))
        return QueryInterface(riid, ppv);

    if (IsEqualGUID(guidService, SID_STopLevelBrowser))
        return QueryInterface(riid, ppv);

    if (m_pSite)
    {
        CComPtr<IServiceProvider> psp;
        if (SUCCEEDED(m_pSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &psp))))
            return psp->QueryService(guidService, riid, ppv);
    }

    return E_NOINTERFACE;
}
