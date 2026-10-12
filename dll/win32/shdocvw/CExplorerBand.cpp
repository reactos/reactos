/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Explorer bar
 * COPYRIGHT:   Copyright 2016 Sylvain Deverre <deverre.sylv@gmail.com>
 *              Copyright 2020-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"
#include <strsafe.h>

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
    // Windows 11 look: no lines, whole-row selection, hover highlight
    return WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_FULLROWSELECT | TVS_TRACKSELECT |
           TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_EDITLABELS | TVS_NOHSCROLL /* | TVS_SINGLEEXPAND*/;
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

    // Insert the top-level items, like the Windows 11 navigation pane: pinned
    // folders (flat), a separator, then the My Computer and My Network Places trees.
    // m_hRoot stays NULL: there is no single "Desktop" root any more.
    static const struct { INT csidl; LPCWSTR pszLabel; } Pinned[] =
    {
        { CSIDL_PROFILE, NULL },                // the user's folder, under its own name
        { CSIDL_DESKTOPDIRECTORY, L"Desktop" },
        { CSIDL_PERSONAL, L"Documents" },
        { -1 /* Downloads */, L"Downloads" },
        { CSIDL_MYPICTURES, L"Pictures" },
        { CSIDL_MYMUSIC, L"Music" },
        { CSIDL_MYVIDEO, L"Videos" },
    };
    for (SIZE_T i = 0; i < _countof(Pinned); i++)
    {
        LPITEMIDLIST pidl = NULL;
        if (Pinned[i].csidl >= 0)
        {
            SHGetSpecialFolderLocation(NULL, Pinned[i].csidl, &pidl);
        }
        else
        {
            WCHAR szPath[MAX_PATH];
            if (SHGetSpecialFolderPathW(NULL, szPath, CSIDL_PROFILE, FALSE) &&
                SUCCEEDED(StringCchCatW(szPath, _countof(szPath), L"\\Downloads")) &&
                PathIsDirectoryW(szPath))
            {
                pidl = ILCreateFromPathW(szPath);
            }
        }
        if (pidl)
        {
            HTREEITEM hItem = _InsertRoot(pidl, Pinned[i].pszLabel, TRUE);
            if (hItem)
                m_hLastPinned = hItem;
        }
    }

    static const INT Trees[] = { CSIDL_DRIVES, CSIDL_NETWORK };
    for (SIZE_T i = 0; i < _countof(Trees); i++)
    {
        LPITEMIDLIST pidl = NULL;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, Trees[i], &pidl)))
            _InsertRoot(pidl, NULL, FALSE);
    }

    if (!TreeView_GetRoot(m_hwndTreeView))
    {
        ERR("Failed to create root items\n");
        return E_FAIL;
    }
    HDC hdc = ::GetDC(m_hwndTreeView);
    TreeView_SetItemHeight(m_hwndTreeView, MulDiv(26, GetDeviceCaps(hdc, LOGPIXELSY), 96));
    ::ReleaseDC(m_hwndTreeView, hdc);

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

// Takes ownership of pidl
HTREEITEM CExplorerBand::_InsertRoot(_In_ LPITEMIDLIST pidl, _In_opt_ LPCWSTR pszLabel, _In_ BOOL bPinned)
{
    HTREEITEM hItem = _InsertItem(NULL, pidl, ILFindLastID(pidl), FALSE);
    ILFree(pidl);
    if (!hItem)
        return NULL;

    // pinned folders are flat shortcuts; the trees always get an expand button
    // (it disappears if there is nothing inside)
    TVITEMW tvi = { TVIF_HANDLE | TVIF_CHILDREN, hItem };
    tvi.cChildren = bPinned ? 0 : 1;
    if (pszLabel)
    {
        tvi.mask |= TVIF_TEXT;
        tvi.pszText = const_cast<LPWSTR>(pszLabel);
    }
    // navigating into a subfolder of a pinned folder selects it in the
    // My Computer tree instead (see _NavigateToPIDL)
    if (bPinned)
        m_PinnedItems.Add(hItem);
    TreeView_SetItem(m_hwndTreeView, &tvi);
    return hItem;
}

// The separator line between the pinned folders and the trees
LRESULT CExplorerBand::_OnTreeCustomDraw(_In_ LPNMTVCUSTOMDRAW pnmcd)
{
    switch (pnmcd->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
            return ((HTREEITEM)pnmcd->nmcd.dwItemSpec == m_hLastPinned) ? CDRF_NOTIFYPOSTPAINT : CDRF_DODEFAULT;
        case CDDS_ITEMPOSTPAINT:
        {
            RECT rc = pnmcd->nmcd.rc;
            rc.left += 8;
            rc.right -= 8;
            rc.top = rc.bottom - 1;
            // 15% of the text color over the background: works for light and dark themes
            COLORREF crBack = GetSysColor(COLOR_WINDOW), crText = GetSysColor(COLOR_WINDOWTEXT);
            HBRUSH hbr = CreateSolidBrush(RGB((GetRValue(crBack) * 85 + GetRValue(crText) * 15) / 100,
                                              (GetGValue(crBack) * 85 + GetGValue(crText) * 15) / 100,
                                              (GetBValue(crBack) * 85 + GetBValue(crText) * 15) / 100));
            FillRect(pnmcd->nmcd.hdc, &rc, hbr);
            DeleteObject(hbr);
            return CDRF_DODEFAULT;
        }
    }
    return CDRF_DODEFAULT;
}

BOOL CExplorerBand::_IsPinned(_In_ HTREEITEM hItem)
{
    return m_PinnedItems.Find(hItem) >= 0;
}

void CExplorerBand::_DestroyTreeView()
{
    m_PinnedItems.RemoveAll();
    m_hLastPinned = NULL;
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

    HTREEITEM hItem = TreeView_GetRoot(m_hwndTreeView);
    HTREEITEM hParent = NULL, tmp;
    while (TRUE)
    {
        CItemData *pItemData = _GetItemData(hItem);
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
        LPITEMIDLIST relativeChild = _IsPinned(hItem) ? NULL : ILFindChild(pItemData->absolutePidl, dest);
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
