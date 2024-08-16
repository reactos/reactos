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

HRESULT CExplorerBand::_CreateTreeView(HWND hwndParent)
{
    HRESULT hr = CNSCBand::_CreateTreeView(hwndParent);
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

    // Communicate via IDispatch
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

// *** IDispatch methods ***

STDMETHODIMP CExplorerBand::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP
CExplorerBand::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    switch (dispIdMember)
    {
        case DISPID_DOWNLOADCOMPLETE:
        case DISPID_NAVIGATECOMPLETE2:
        {
            TRACE("dispId %d received\n", dispIdMember);
            _NavigateToCurrentFolder();
            return S_OK;
        }
    }
    TRACE("Unknown dispid requested: %08x\n", dispIdMember);
    return E_INVALIDARG;
}

BOOL CExplorerBand::_NavigateToCurrentFolder()
{
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hr = _GetCurrentLocation(&pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    // Find PIDL into our explorer
    ++m_mtxBlockNavigate;
    HTREEITEM hItem;
    BOOL result = _NavigateToPIDL(pidl, &hItem, TRUE, FALSE, TRUE);
    --m_mtxBlockNavigate;

    return result;
}

/**
 * Navigate to a given PIDL in the treeview, and return matching tree item handle
 *  - dest: The absolute PIDL we should navigate in the treeview
 *  - item: Handle of the tree item matching the PIDL
 *  - bExpand: expand collapsed nodes in order to find the right element
 *  - bInsert: insert the element at the right place if we don't find it
 *  - bSelect: select the item after we found it
 */
BOOL
CExplorerBand::_NavigateToPIDL(
    _In_ LPCITEMIDLIST dest,
    _Out_ HTREEITEM *phItem,
    _In_ BOOL bExpand,
    _In_ BOOL bInsert,
    _In_ BOOL bSelect)
{
    if (!phItem)
        return FALSE;

    *phItem = NULL;

    HTREEITEM hItem = TreeView_GetFirstVisible(m_hwndTreeView);
    HTREEITEM hParent = NULL, tmp;
    while (TRUE)
    {
        CItemData *pItemData = GetItemData(hItem);
        if (!pItemData)
        {
            ERR("Something has gone wrong, no data associated to node\n");
            return FALSE;
        }

        // If we found our node, give it back
        if (!m_pDesktop->CompareIDs(0, pItemData->absolutePidl, dest))
        {
            if (bSelect)
                TreeView_SelectItem(m_hwndTreeView, hItem);
            *phItem = hItem;
            return TRUE;
        }

        // Check if we are a parent of the requested item
        TVITEMW tvItem;
        LPITEMIDLIST relativeChild = ILFindChild(pItemData->absolutePidl, dest);
        if (relativeChild)
        {
            // Notify treeview we have children
            tvItem.mask = TVIF_CHILDREN;
            tvItem.hItem = hItem;
            tvItem.cChildren = 1;
            TreeView_SetItem(m_hwndTreeView, &tvItem);

            // If we can expand and the node isn't expanded yet, do it
            if (bExpand)
            {
                if (!pItemData->expanded)
                {
                    _InsertSubitems(hItem, pItemData->absolutePidl);
                    pItemData->expanded = TRUE;
                }
                TreeView_Expand(m_hwndTreeView, hItem, TVE_EXPAND);
            }

            // Try to get a child
            tmp = TreeView_GetChild(m_hwndTreeView, hItem);
            if (tmp)
            {
                // We have a child, let's continue with it
                hParent = hItem;
                hItem = tmp;
                continue;
            }

            if (bInsert && pItemData->expanded)
            {
                // Happens when we have to create a subchild inside a child
                hItem = _InsertItem(hItem, dest, relativeChild, TRUE);
            }

            // We end up here, without any children, so we found nothing
            // Tell the parent node it has children
            ZeroMemory(&tvItem, sizeof(tvItem));
            return FALSE;
        }

        // Find sibling
        tmp = TreeView_GetNextSibling(m_hwndTreeView, hItem);
        if (tmp)
        {
            hItem = tmp;
            continue;
        }

        if (bInsert)
        {
            *phItem = hItem = _InsertItem(hParent, dest, ILFindLastID(dest), TRUE);
            return TRUE;
        }

        return FALSE;
    }

    UNREACHABLE;
}
