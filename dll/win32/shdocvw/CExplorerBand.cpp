/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Explorer bar
 * COPYRIGHT:   Copyright 2016 Sylvain Deverre <deverre.sylv@gmail.com>
 *              Copyright 2020-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/*
 * TODO:
 *  - Monitor correctly "external" shell interrupts (seems like we need to register/deregister them for each folder)
 *  - find and fix what cause explorer crashes sometimes (seems to be explorer that does more releases than addref)
 *  - TESTING
 */

CExplorerBand::CExplorerBand()
{
}

CExplorerBand::~CExplorerBand()
{
}

STDMETHODIMP CExplorerBand::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_ExplorerBand;
    return S_OK;
}

INT CExplorerBand::_GetRootCsidl()
{
    return CSIDL_DESKTOP;
}

DWORD CExplorerBand::_GetTVStyle()
{
    // Remove TVS_SINGLEEXPAND for now since it has strange behaviour
    return WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASLINES |
           TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS /* | TVS_SINGLEEXPAND*/;
}

DWORD CExplorerBand::_GetTVExStyle()
{
    return 0;
}

DWORD CExplorerBand::_GetEnumFlags()
{
    return SHCONTF_FOLDERS;
}

BOOL CExplorerBand::_GetTitle(LPWSTR pszTitle, INT cchTitle)
{
    return ::LoadStringW(instance, IDS_FOLDERSLABEL, pszTitle, cchTitle);
}

BOOL CExplorerBand::_WantsRootItem()
{
    return TRUE;
}

// Called when the user has selected an item.
STDMETHODIMP CExplorerBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    return Invoke(pidl);
}

// Handles a user action on an item.
STDMETHODIMP CExplorerBand::Invoke(_In_ PCIDLIST_ABSOLUTE pidl)
{
    /* Prevents navigation if selection is initiated inside the band */
    if (m_mtxBlockNavigate)
        return S_OK;

    _UpdateBrowser(pidl);
    m_hwndTreeView.SetFocus();
    return S_OK;
}

void CExplorerBand::_SortItems(HTREEITEM hParent)
{
    TVSORTCB sortCallback;
    sortCallback.hParent = hParent;
    sortCallback.lpfnCompare = _CompareTreeItems;
    sortCallback.lParam = (LPARAM)(PVOID)m_pDesktop; // m_pDesktop is not a pointer
    TreeView_SortChildrenCB(m_hwndTreeView, &sortCallback, 0);
}

INT CALLBACK CExplorerBand::_CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3)
{
    CItemData *info1 = (CItemData*)p1;
    CItemData *info2 = (CItemData*)p2;
    IShellFolder *pDesktop = (IShellFolder *)p3;
    HRESULT hr = pDesktop->CompareIDs(0, info1->absolutePidl, info2->absolutePidl);
    if (FAILED(hr))
        return 0;
    return (SHORT)HRESULT_CODE(hr);
}

HRESULT CExplorerBand::_CreateTreeView()
{
    HRESULT hr = CNSCBand::_CreateTreeView();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Insert the root node
    m_hRoot = _InsertItem(NULL, m_pDesktop, m_pidlRoot, m_pidlRoot, FALSE);
    if (!m_hRoot)
    {
        ERR("Failed to create root item\n");
        return E_FAIL;
    }
    TreeView_Expand(m_hwndTreeView, m_hRoot, TVE_EXPAND);

    // Navigate to current folder position
    _NavigateToCurrentFolder();

    // Register browser connection endpoint
    CComPtr<IWebBrowser2> browserService;
    hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = AtlAdvise(browserService, dynamic_cast<IDispatch*>(this), DIID_DWebBrowserEvents, &m_adviseCookie);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

void CExplorerBand::_DestroyTreeView()
{
    CComPtr<IWebBrowser2> browserService;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp,
                                       IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    AtlUnadvise(browserService, DIID_DWebBrowserEvents, m_adviseCookie);

    CNSCBand::_DestroyTreeView();
}
