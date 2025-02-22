/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Favorites bar
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

CFavBand::CFavBand()
{
}

CFavBand::~CFavBand()
{
}

STDMETHODIMP CFavBand::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_SH_FavBand;
    return S_OK;
}

INT CFavBand::_GetRootCsidl()
{
    return CSIDL_FAVORITES;
}

DWORD CFavBand::_GetTVStyle()
{
    // Remove TVS_SINGLEEXPAND for now since it has strange behaviour
    return TVS_NOHSCROLL | TVS_NONEVENHEIGHT | TVS_FULLROWSELECT | TVS_INFOTIP |
           /*TVS_SINGLEEXPAND | TVS_TRACKSELECT |*/ TVS_SHOWSELALWAYS | TVS_EDITLABELS |
           WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
}

DWORD CFavBand::_GetTVExStyle()
{
    return WS_EX_CLIENTEDGE;
}

DWORD CFavBand::_GetEnumFlags()
{
    return SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
}

BOOL CFavBand::_GetTitle(LPWSTR pszTitle, INT cchTitle)
{
#define IDS_FAVORITES 47 // Borrowed from shell32.dll
    HINSTANCE hShell32 = ::LoadLibraryExW(L"shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hShell32)
    {
        ::LoadStringW(hShell32, IDS_FAVORITES, pszTitle, cchTitle);
        ::FreeLibrary(hShell32);
        return TRUE;
    }
    return FALSE;
#undef IDS_FAVORITES
}

BOOL CFavBand::_WantsRootItem()
{
    return FALSE;
}

HRESULT CFavBand::_CreateToolbar(HWND hwndParent)
{
#define IDB_SHELL_EXPLORER_SM 216 // Borrowed from browseui.dll
    HINSTANCE hinstBrowseUI = LoadLibraryExW(L"browseui.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ATLASSERT(hinstBrowseUI);
    HBITMAP hbmToolbar = NULL;
    if (hinstBrowseUI)
    {
        hbmToolbar = LoadBitmapW(hinstBrowseUI, MAKEINTRESOURCEW(IDB_SHELL_EXPLORER_SM));
        FreeLibrary(hinstBrowseUI);
    }
#undef IDB_SHELL_EXPLORER_SM
    ATLASSERT(hbmToolbar);
    if (!hbmToolbar)
        return E_FAIL;

    m_hToolbarImageList = ImageList_Create(16, 16, ILC_COLOR32, 0, 8);
    ATLASSERT(m_hToolbarImageList);
    if (!m_hToolbarImageList)
        return E_FAIL;

    ImageList_Add(m_hToolbarImageList, hbmToolbar, NULL);
    DeleteObject(hbmToolbar);

    DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER |
                  TBSTYLE_WRAPABLE;
    HWND hwndTB = ::CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style, 0, 0, 0, 0, hwndParent,
                                    (HMENU)UlongToHandle(IDW_TOOLBAR), instance, NULL);
    ATLASSERT(hwndTB);
    if (!hwndTB)
        return E_FAIL;

    m_hwndToolbar.Attach(hwndTB);
    m_hwndToolbar.SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    m_hwndToolbar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)m_hToolbarImageList);
    m_hwndToolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    WCHAR szzAdd[MAX_PATH], szzOrganize[MAX_PATH];
    ZeroMemory(szzAdd, sizeof(szzAdd));
    ZeroMemory(szzOrganize, sizeof(szzOrganize));
    LoadStringW(instance, IDS_ADD, szzAdd, _countof(szzAdd));
    LoadStringW(instance, IDS_ORGANIZE, szzOrganize, _countof(szzOrganize));

    TBBUTTON tbb[2] = { { 0 } };
    INT iButton = 0;
    tbb[iButton].iBitmap = 3;
    tbb[iButton].idCommand = ID_ADD;
    tbb[iButton].fsState = TBSTATE_ENABLED;
    tbb[iButton].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[iButton].iString = (INT)m_hwndToolbar.SendMessage(TB_ADDSTRING, 0, (LPARAM)szzAdd);
    ++iButton;
    tbb[iButton].iBitmap = 42;
    tbb[iButton].idCommand = ID_ORGANIZE;
    tbb[iButton].fsState = TBSTATE_ENABLED;
    tbb[iButton].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[iButton].iString = (INT)m_hwndToolbar.SendMessage(TB_ADDSTRING, 0, (LPARAM)szzOrganize);
    ++iButton;
    ATLASSERT(iButton == _countof(tbb));
    m_hwndToolbar.SendMessage(TB_ADDBUTTONS, iButton, (LPARAM)&tbb);

    return S_OK;
}

// Called when the user has selected an item.
STDMETHODIMP CFavBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    CComHeapPtr<ITEMIDLIST> pidlTarget;
    DWORD attrs = SFGAO_FOLDER | SFGAO_LINK;
    HRESULT hr = GetNavigateTarget(pidl, &pidlTarget, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if ((attrs & (SFGAO_FOLDER | SFGAO_LINK)) == (SFGAO_FOLDER | SFGAO_LINK))
        return _UpdateBrowser(pidlTarget);

    if (attrs & SFGAO_FOLDER)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_hwndTreeView);
        CItemData *pItemData = GetItemData(hItem);
        if (pItemData && !pItemData->expanded)
        {
            _InsertSubitems(hItem, pItemData->absolutePidl);
            pItemData->expanded = TRUE;
        }
        TreeView_Expand(m_hwndTreeView, hItem, TVE_EXPAND);
        return S_OK;
    }

    SHELLEXECUTEINFOW info = { sizeof(info) };
    info.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_IDLIST;
    info.hwnd = m_hWnd;
    info.nShow = SW_SHOWNORMAL;
    info.lpIDList = pidlTarget;
    ShellExecuteExW(&info);
    return hr;
}

void CFavBand::_SortItems(HTREEITEM hParent)
{
    TreeView_SortChildren(m_hwndTreeView, hParent, 0); // Sort by name
}

HRESULT CFavBand::_CreateTreeView(HWND hwndParent)
{
    HRESULT hr = CNSCBand::_CreateTreeView(hwndParent);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    TreeView_SetItemHeight(m_hwndTreeView, 24);
    _InsertSubitems(TVI_ROOT, m_pidlRoot);
    return hr;
}
