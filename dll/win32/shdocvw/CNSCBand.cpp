/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     NameSpace Control Band
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"
#include <shlobj.h>
#include <commoncontrols.h>
#include <undocshell.h>

#define TIMER_ID_REFRESH 9999

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

HRESULT
SHDOCVW_GetPathOfShortcut(
    _In_opt_ HWND hWnd,
    _In_ LPCWSTR pszLnkFile,
    _Out_ LPWSTR pszPath)
{
    *pszPath = UNICODE_NULL;
    CComPtr<IShellLink> pShellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL,  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IShellLink, &pShellLink));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IPersistFile> pPersistFile;
    hr = pShellLink->QueryInterface(IID_PPV_ARG(IPersistFile, &pPersistFile));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pPersistFile->Load(pszLnkFile, STGM_READ);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WIN32_FIND_DATA find;
    hr = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT
SHDOCVW_CreateShortcut(
    _In_ LPCWSTR pszLnkFileName, 
    _In_ PCIDLIST_ABSOLUTE pidlTarget,
    _In_opt_ LPCWSTR pszDescription)
{
    HRESULT hr;

    CComPtr<IShellLink> psl;
    hr = CoCreateInstance(CLSID_ShellLink, NULL,  CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLink, &psl));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    psl->SetIDList(pidlTarget);

    if (pszDescription)
        psl->SetDescription(pszDescription);

    CComPtr<IPersistFile> ppf;
    hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return ppf->Save(pszLnkFileName, TRUE);
}

CNSCBand::CNSCBand()
{
    SHDOCVW_LockModule();

    INITCOMMONCONTROLSEX iccx = { sizeof(iccx), ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES };
    ::InitCommonControlsEx(&iccx);
}

CNSCBand::~CNSCBand()
{
    if (m_hToolbarImageList)
    {
        ImageList_Destroy(m_hToolbarImageList);
        m_hToolbarImageList = NULL;
    }
    SHDOCVW_UnlockModule();
}

VOID CNSCBand::OnFinalMessage(HWND)
{
    // The message loop is finished, now we can safely destruct!
    static_cast<IDeskBand *>(this)->Release();
}

// *** helper methods ***

CNSCBand::CItemData* CNSCBand::GetItemData(_In_ HTREEITEM hItem)
{
    if (hItem == TVI_ROOT)
        return NULL;

    TVITEMW tvItem = { TVIF_PARAM, hItem };
    if (!TreeView_GetItem(m_hwndTreeView, &tvItem))
        return NULL;

    return reinterpret_cast<CItemData*>(tvItem.lParam);
}

static HRESULT
SHDOCVW_GetCurrentLocationFromView(_In_ IShellView& View, _In_ PIDLIST_ABSOLUTE *ppidl)
{
    CComPtr<IFolderView> pfv;
    CComPtr<IShellFolder> psf;
    HRESULT hr = View.QueryInterface(IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr) && SUCCEEDED(hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf))))
        hr = SHELL_GetIDListFromObject(psf, ppidl);
    return hr;
}

HRESULT CNSCBand::_GetCurrentLocation(_Out_ PIDLIST_ABSOLUTE *ppidl)
{
    *ppidl = NULL;
    CComPtr<IShellBrowser> psb;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IBrowserService> pbs;
    if (SUCCEEDED(hr = psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        if (SUCCEEDED(hr = pbs->GetPidl(ppidl)) && *ppidl)
            return hr;

    CComPtr<IShellView> psv;
    if (!FAILED_UNEXPECTEDLY(hr = psb->QueryActiveShellView(&psv)))
        if (SUCCEEDED(hr = psv.p ? SHDOCVW_GetCurrentLocationFromView(*psv.p, ppidl) : E_FAIL))
            return hr;
    return hr;
}

HRESULT CNSCBand::_IsCurrentLocation(_In_ PCIDLIST_ABSOLUTE pidl)
{
    if (!pidl)
        return E_INVALIDARG;
    HRESULT hr = E_FAIL;
    PIDLIST_ABSOLUTE location = NULL;
    hr = _GetCurrentLocation(&location);
    if (SUCCEEDED(hr))
        hr = SHELL_IsEqualAbsoluteID(location, pidl) ? S_OK : S_FALSE;
    ILFree(location);
    return hr;
}

HRESULT CNSCBand::_ExecuteCommand(_In_ CComPtr<IContextMenu>& menu, _In_ UINT nCmd)
{
    CComPtr<IOleWindow> pBrowserOleWnd;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser,
                                       IID_PPV_ARG(IOleWindow, &pBrowserOleWnd));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    HWND browserWnd;
    hr = pBrowserOleWnd->GetWindow(&browserWnd);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CMINVOKECOMMANDINFO cmi = { sizeof(cmi) };
    cmi.lpVerb = MAKEINTRESOURCEA(nCmd);
    cmi.hwnd = browserWnd;
    cmi.nShow = SW_SHOW;
    if (::GetKeyState(VK_SHIFT) < 0)
        cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
    if (::GetKeyState(VK_CONTROL) < 0)
        cmi.fMask |= CMIC_MASK_CONTROL_DOWN;

    return menu->InvokeCommand(&cmi);
}

void CNSCBand::_RegisterChangeNotify()
{
#define TARGET_EVENTS ( \
    SHCNE_DRIVEADD | SHCNE_MKDIR | SHCNE_CREATE | SHCNE_DRIVEREMOVED | SHCNE_RMDIR | \
    SHCNE_DELETE | SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM | SHCNE_UPDATEDIR | \
    SHCNE_UPDATEITEM | SHCNE_ASSOCCHANGED \
)
    // Register shell notification
    SHChangeNotifyEntry shcne = { m_pidlRoot, TRUE };
    m_shellRegID = SHChangeNotifyRegister(m_hWnd,
                                          SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                                          TARGET_EVENTS,
                                          WM_USER_SHELLEVENT,
                                          1, &shcne);
    if (!m_shellRegID)
    {
        ERR("Something went wrong, error %08x\n", GetLastError());
    }
}

void CNSCBand::_UnregisterChangeNotify()
{
    SHChangeNotifyDeregister(m_shellRegID);
    m_shellRegID = 0;
}

void CNSCBand::_DestroyTreeView()
{
    TRACE("Cleaning up treeview...\n");
    /* Remove all items of the treeview */
    ::RevokeDragDrop(m_hwndTreeView);
    TreeView_DeleteAllItems(m_hwndTreeView);
    m_hwndTreeView.DestroyWindow();
    m_pDesktop = NULL;
    m_hRoot = NULL;
    TRACE("Cleanup ok\n");
}

void CNSCBand::_DestroyToolbar()
{
    m_hwndToolbar.DestroyWindow();
}

HRESULT CNSCBand::_CreateTreeView(HWND hwndParent)
{
    RefreshFlags(&m_dwTVStyle, &m_dwTVExStyle, &m_dwEnumFlags);
    HWND hwndTV = ::CreateWindowExW(m_dwTVExStyle, WC_TREEVIEWW, NULL, m_dwTVStyle, 0, 0, 0, 0,
                                    hwndParent, (HMENU)UlongToHandle(IDW_TREEVIEW), instance, NULL);
    ATLASSERT(hwndTV);
    if (!hwndTV)
        return E_FAIL;

    m_hwndTreeView.Attach(hwndTV);
    ::RegisterDragDrop(m_hwndTreeView, dynamic_cast<IDropTarget*>(this));

    // Init the treeview here
    HRESULT hr = SHGetDesktopFolder(&m_pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_pidlRoot.Free();
    hr = SHGetFolderLocation(m_hWnd, _GetRootCsidl(), NULL, 0, &m_pidlRoot);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Create image list and set
    IImageList *piml;
    hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    TreeView_SetImageList(m_hwndTreeView, (HIMAGELIST)piml, TVSIL_NORMAL);
    return S_OK;
}

BOOL
CNSCBand::_IsTreeItemInEnum(
    _In_ HTREEITEM hItem,
    _In_ IEnumIDList *pEnum)
{
    CItemData* pItemData = GetItemData(hItem);
    if (!pItemData)
        return FALSE;

    pEnum->Reset();

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
    {
        if (ILIsEqual(pidlTemp, pItemData->relativePidl))
            return TRUE;

        pidlTemp.Free();
    }

    return FALSE;
}

BOOL
CNSCBand::_TreeItemHasThisChild(
    _In_ HTREEITEM hItem,
    _In_ PCITEMID_CHILD pidlChild)
{
    for (hItem = TreeView_GetChild(m_hwndTreeView, hItem); hItem;
         hItem = TreeView_GetNextSibling(m_hwndTreeView, hItem))
    {
        CItemData* pItemData = GetItemData(hItem);
        if (ILIsEqual(pItemData->relativePidl, pidlChild))
            return TRUE;
    }

    return FALSE;
}

HRESULT
CNSCBand::_GetItemEnum(
    _Out_ CComPtr<IEnumIDList>& pEnum,
    _In_ HTREEITEM hItem,
    _Out_opt_ IShellFolder **ppFolder)
{
    CComPtr<IShellFolder> psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder> pFolder;
    if (!ppFolder)
        ppFolder = &pFolder;

    if (hItem == m_hRoot && hItem)
    {
        *ppFolder = psfDesktop;
        (*ppFolder)->AddRef();
    }
    else
    {
        CItemData* pItemData = GetItemData(hItem);
        if (!pItemData && hItem == TVI_ROOT && !_WantsRootItem())
            hr = psfDesktop->BindToObject(m_pidlRoot, NULL, IID_PPV_ARG(IShellFolder, ppFolder));
        else
            hr = psfDesktop->BindToObject(pItemData->absolutePidl, NULL, IID_PPV_ARG(IShellFolder, ppFolder));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return (*ppFolder)->EnumObjects(NULL, _GetEnumFlags(), &pEnum);
}

BOOL CNSCBand::_ItemHasAnyChild(_In_ HTREEITEM hItem)
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hr = _GetItemEnum(pEnum, hItem);
    if (FAILED(hr))
        return FALSE;

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    hr = pEnum->Next(1, &pidlTemp, NULL);
    return SUCCEEDED(hr);
}

void CNSCBand::_RefreshRecurse(_In_ HTREEITEM hTarget)
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hrEnum = _GetItemEnum(pEnum, hTarget);

    // Delete zombie items
    HTREEITEM hItem, hNextItem;
    for (hItem = TreeView_GetChild(m_hwndTreeView, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(m_hwndTreeView, hItem);

        if (SUCCEEDED(hrEnum) && !_IsTreeItemInEnum(hItem, pEnum))
            TreeView_DeleteItem(m_hwndTreeView, hItem);
    }

    pEnum = NULL;
    hrEnum = _GetItemEnum(pEnum, hTarget);

    CItemData* pItemData = ((hTarget == TVI_ROOT) ? NULL : GetItemData(hTarget));

    // Insert new items and update items
    if (SUCCEEDED(hrEnum))
    {
        CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
        while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
        {
            if (!_TreeItemHasThisChild(hTarget, pidlTemp))
            {
                if (pItemData)
                {
                    CComHeapPtr<ITEMIDLIST> pidlAbsolute(ILCombine(pItemData->absolutePidl, pidlTemp));
                    _InsertItem(hTarget, pidlAbsolute, pidlTemp, TRUE);
                }
                else
                {
                    CComHeapPtr<ITEMIDLIST> pidlAbsolute(ILCombine(m_pidlRoot, pidlTemp));
                    _InsertItem(hTarget, pidlAbsolute, pidlTemp, TRUE);
                }
            }
            pidlTemp.Free();
        }
    }

    // Update children and recurse
    for (hItem = TreeView_GetChild(m_hwndTreeView, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(m_hwndTreeView, hItem);

        TV_ITEMW item = { TVIF_HANDLE | TVIF_CHILDREN };
        item.hItem = hItem;
        item.cChildren = _ItemHasAnyChild(hItem);
        TreeView_SetItem(m_hwndTreeView, &item);

        if (TreeView_GetItemState(m_hwndTreeView, hItem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
            _RefreshRecurse(hItem);
    }
}

void CNSCBand::_Refresh()
{
    m_hwndTreeView.SendMessage(WM_SETREDRAW, FALSE, 0);
    _RefreshRecurse(_WantsRootItem() ? m_hRoot : TVI_ROOT);
    m_hwndTreeView.SendMessage(WM_SETREDRAW, TRUE, 0);
}

LRESULT CNSCBand::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    KillTimer(wParam);

    if (wParam == TIMER_ID_REFRESH)
        _Refresh();

    return 0;
}

void
CNSCBand::OnChangeNotify(
    _In_opt_ LPCITEMIDLIST pidl0,
    _In_opt_ LPCITEMIDLIST pidl1,
    _In_ LONG lEvent)
{
    switch (lEvent)
    {
        case SHCNE_DRIVEADD:
        case SHCNE_MKDIR:
        case SHCNE_CREATE:
        case SHCNE_DRIVEREMOVED:
        case SHCNE_RMDIR:
        case SHCNE_DELETE:
        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
        case SHCNE_UPDATEDIR:
        case SHCNE_UPDATEITEM:
        case SHCNE_ASSOCCHANGED:
        {
            KillTimer(TIMER_ID_REFRESH);
            SetTimer(TIMER_ID_REFRESH, 500, NULL);
            break;
        }
        default:
        {
            TRACE("lEvent: 0x%08lX\n", lEvent);
            break;
        }
    }
}

HTREEITEM
CNSCBand::_InsertItem(
    _In_opt_ HTREEITEM hParent,
    _Inout_ IShellFolder *psfParent,
    _In_ LPCITEMIDLIST pElt,
    _In_ LPCITEMIDLIST pEltRelative,
    _In_ BOOL bSort)
{
    /* Get the attributes of the node */
    SFGAOF attrs = SFGAO_STREAM | SFGAO_HASSUBFOLDER;
    HRESULT hr = psfParent->GetAttributesOf(1, &pEltRelative, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Get the name of the node */
    WCHAR wszDisplayName[MAX_PATH];
    STRRET strret;
    hr = psfParent->GetDisplayNameOf(pEltRelative, SHGDN_INFOLDER, &strret);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    hr = StrRetToBufW(&strret, pEltRelative, wszDisplayName, MAX_PATH);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Get the icon of the node */
    INT iIcon = SHMapPIDLToSystemImageListIndex(psfParent, pEltRelative, NULL);

    CItemData* pChildInfo = new CItemData;
    if (!pChildInfo)
    {
        ERR("Failed to allocate CItemData\n");
        return NULL;
    }
    pChildInfo->absolutePidl.Attach(ILClone(pElt));
    pChildInfo->relativePidl.Attach(ILClone(pEltRelative));

    // Set up our treeview template
    TV_INSERTSTRUCT tvInsert = { hParent, TVI_LAST };
    tvInsert.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
    tvInsert.item.cchTextMax = MAX_PATH;
    tvInsert.item.pszText = wszDisplayName;
    tvInsert.item.iImage = tvInsert.item.iSelectedImage = iIcon;
    tvInsert.item.lParam = (LPARAM)pChildInfo;

    if (!(attrs & SFGAO_STREAM) && (attrs & SFGAO_HASSUBFOLDER))
        tvInsert.item.cChildren = 1;

    HTREEITEM htiCreated = TreeView_InsertItem(m_hwndTreeView, &tvInsert);

    if (bSort)
        _SortItems(hParent);

    return htiCreated;
}

/* This is the slow version of the above method */
HTREEITEM
CNSCBand::_InsertItem(
    _In_opt_ HTREEITEM hParent,
    _In_ LPCITEMIDLIST pElt,
    _In_ LPCITEMIDLIST pEltRelative,
    _In_ BOOL bSort)
{
    CComPtr<IShellFolder> psfFolder;
    HRESULT hr = SHBindToParent(pElt, IID_PPV_ARG(IShellFolder, &psfFolder), NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    return _InsertItem(hParent, psfFolder, pElt, pEltRelative, bSort);
}

BOOL CNSCBand::_InsertSubitems(HTREEITEM hItem, LPCITEMIDLIST entry)
{
    ULONG fetched = 1, uItemCount = 0;

    CComPtr<IEnumIDList> pEnum;
    CComPtr<IShellFolder> pFolder;
    HRESULT hr = _GetItemEnum(pEnum, hItem, &pFolder);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    /* Don't redraw while we add stuff into the tree */
    m_hwndTreeView.SendMessage(WM_SETREDRAW, FALSE, 0);

    LPITEMIDLIST pidlSub;
    while (SUCCEEDED(pEnum->Next(1, &pidlSub, &fetched)) && pidlSub && fetched)
    {
        LPITEMIDLIST pidlSubComplete;
        pidlSubComplete = ILCombine(entry, pidlSub);

        if (_InsertItem(hItem, pFolder, pidlSubComplete, pidlSub, FALSE))
            ++uItemCount;

        ILFree(pidlSubComplete);
        ILFree(pidlSub);
    }

    /* Let's do sorting */
    _SortItems(hItem);

    /* Now we can redraw */
    m_hwndTreeView.SendMessage(WM_SETREDRAW, TRUE, 0);

    return (uItemCount > 0);
}

// *** message handlers ***

LRESULT CNSCBand::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (FAILED_UNEXPECTEDLY(_CreateToolbar(m_hWnd)))
        return -1;
    if (FAILED_UNEXPECTEDLY(_CreateTreeView(m_hWnd)))
        return -1;
    return 0;
}

LRESULT CNSCBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _DestroyTreeView();
    _DestroyToolbar();
    _UnregisterChangeNotify();
    return 0;
}

LRESULT CNSCBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_hwndTreeView)
        return 0;

    RECT rc;
    GetClientRect(&rc);
    LONG cx = rc.right, cy = rc.bottom;

    RECT rcTB;
    LONG cyTB = 0;
    if (m_hwndToolbar)
    {
        m_hwndToolbar.SendMessage(TB_AUTOSIZE, 0, 0);
        m_hwndToolbar.GetWindowRect(&rcTB);
        cyTB = rcTB.bottom - rcTB.top;
    }

    m_hwndTreeView.MoveWindow(0, cyTB, cx, cy - cyTB);
    return 0;
}

LRESULT CNSCBand::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_bFocused = TRUE;
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), TRUE);
    bHandled = FALSE;
    return 0;
}

LRESULT CNSCBand::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), FALSE);
    m_bFocused = FALSE;
    return 0;
}

HRESULT CNSCBand::_AddFavorite()
{
    CComHeapPtr<ITEMIDLIST> pidlCurrent;
    _GetCurrentLocation(&pidlCurrent);

    WCHAR szCurDir[MAX_PATH];
    if (!ILGetDisplayName(pidlCurrent, szCurDir))
    {
        FIXME("\n");
        return E_FAIL;
    }

    WCHAR szPath[MAX_PATH], szSuffix[32];
    SHGetSpecialFolderPathW(m_hWnd, szPath, CSIDL_FAVORITES, TRUE);
    PathAppendW(szPath, PathFindFileNameW(szCurDir));

    const INT ich = lstrlenW(szPath);
    for (INT iTry = 2; iTry <= 9999; ++iTry)
    {
        PathAddExtensionW(szPath, L".lnk");
        if (!PathFileExistsW(szPath))
            break;
        szPath[ich] = UNICODE_NULL;
        wsprintfW(szSuffix, L" (%d)", iTry);
        lstrcatW(szPath, szSuffix);
    }

    TRACE("%S, %S\n", szCurDir, szPath);

    return SHDOCVW_CreateShortcut(szPath, pidlCurrent, NULL);
}

LRESULT CNSCBand::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    switch (LOWORD(wParam))
    {
        case ID_ADD:
        {
            _AddFavorite();
            break;
        }
        case ID_ORGANIZE:
        {
            SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_INVOKEIDLIST };
            sei.hwnd = m_hWnd;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpIDList = m_pidlRoot;
            ::ShellExecuteExW(&sei);
            break;
        }
    }
    return 0;
}

BOOL CNSCBand::OnTreeItemExpanding(_In_ LPNMTREEVIEW pnmtv)
{
    CItemData *pItemData;

    if (pnmtv->action == TVE_COLLAPSE)
    {
        if (pnmtv->itemNew.hItem == m_hRoot)
        {
            // Prenvent root from collapsing
            pnmtv->itemNew.mask |= TVIF_STATE;
            pnmtv->itemNew.stateMask |= TVIS_EXPANDED;
            pnmtv->itemNew.state &= ~TVIS_EXPANDED;
            pnmtv->action = TVE_EXPAND;
            return TRUE;
        }
    }

    if (pnmtv->action == TVE_EXPAND)
    {
        // Grab our directory PIDL
        pItemData = GetItemData(pnmtv->itemNew.hItem);
        // We have it, let's try
        if (pItemData && !pItemData->expanded)
        {
            if (_InsertSubitems(pnmtv->itemNew.hItem, pItemData->absolutePidl))
            {
                pItemData->expanded = TRUE;
            }
            else
            {
                // remove subitem "+" since we failed to add subitems
                TVITEMW tvItem = { TVIF_CHILDREN, pnmtv->itemNew.hItem };
                tvItem.cChildren = 0;
                TreeView_SetItem(m_hwndTreeView, &tvItem);
            }
        }
    }
    return FALSE;
}

BOOL CNSCBand::OnTreeItemDeleted(_In_ LPNMTREEVIEW pnmtv)
{
    // Navigate to parent when deleting selected item
    HTREEITEM hItem = pnmtv->itemOld.hItem;
    HTREEITEM hParent = TreeView_GetParent(m_hwndTreeView, hItem);
    if (hParent && TreeView_GetSelection(m_hwndTreeView) == hItem)
        TreeView_SelectItem(m_hwndTreeView, hParent);

    /* Destroy memory associated to our node */
    CItemData* pItemData = GetItemData(hItem);
    if (!pItemData)
        return FALSE;

    delete pItemData;

    return TRUE;
}

void CNSCBand::_OnSelectionChanged(_In_ LPNMTREEVIEW pnmtv)
{
    HTREEITEM hItem = pnmtv->itemNew.hItem;
    if (!hItem)
        return;
    CItemData* pItemData = GetItemData(hItem);
    if (pItemData)
        OnSelectionChanged(pItemData->absolutePidl);
}

void CNSCBand::OnTreeItemDragging(_In_ LPNMTREEVIEW pnmtv, _In_ BOOL isRightClick)
{
    CItemData* pItemData = GetItemData(pnmtv->itemNew.hItem);
    if (!pItemData)
        return;

    HRESULT hr;
    CComPtr<IShellFolder> pSrcFolder;
    LPCITEMIDLIST pLast;
    hr = SHBindToParent(pItemData->absolutePidl, IID_PPV_ARG(IShellFolder, &pSrcFolder), &pLast);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    SFGAOF attrs = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK;
    pSrcFolder->GetAttributesOf(1, &pLast, &attrs);

    DWORD dwEffect = 0;
    if (attrs & SFGAO_CANCOPY)
        dwEffect |= DROPEFFECT_COPY;
    if (attrs & SFGAO_CANMOVE)
        dwEffect |= DROPEFFECT_MOVE;
    if (attrs & SFGAO_CANLINK)
        dwEffect |= DROPEFFECT_LINK;

    CComPtr<IDataObject> pObj;
    hr = pSrcFolder->GetUIObjectOf(m_hWnd, 1, &pLast, IID_IDataObject, 0, (LPVOID*)&pObj);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    DoDragDrop(pObj, this, dwEffect, &dwEffect);
}

LRESULT CNSCBand::OnBeginLabelEdit(_In_ LPNMTVDISPINFO dispInfo)
{
    // TODO: put this in a function ? (mostly copypasta from CDefView)
    DWORD dwAttr = SFGAO_CANRENAME;
    CComPtr<IShellFolder> pParent;
    LPCITEMIDLIST pChild;
    HRESULT hr;

    CItemData *info = GetItemData(dispInfo->item.hItem);
    if (!info)
        return FALSE;

    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    hr = pParent->GetAttributesOf(1, &pChild, &dwAttr);
    if (SUCCEEDED(hr) && (dwAttr & SFGAO_CANRENAME))
    {
        m_isEditing = TRUE;
        m_oldSelected = NULL;
        return FALSE;
    }

    return TRUE;
}

HRESULT CNSCBand::_UpdateBrowser(LPCITEMIDLIST pidlGoto)
{
    CComPtr<IShellBrowser> pBrowserService;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_STopLevelBrowser,
                                       IID_PPV_ARG(IShellBrowser, &pBrowserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBrowserService->BrowseObject(pidlGoto, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

LRESULT CNSCBand::OnEndLabelEdit(_In_ LPNMTVDISPINFO dispInfo)
{
    CItemData *info = GetItemData(dispInfo->item.hItem);
    HRESULT hr;

    m_isEditing = FALSE;
    if (m_oldSelected)
    {
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hwndTreeView, m_oldSelected);
        --m_mtxBlockNavigate;
    }

    if (!dispInfo->item.pszText)
        return FALSE;

    CComPtr<IShellFolder> pParent;
    LPCITEMIDLIST pidlChild;
    BOOL RenamedCurrent = _IsCurrentLocation(info->absolutePidl) == S_OK;

    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pidlChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    CComHeapPtr<ITEMIDLIST> pidlNew;
    hr = pParent->SetNameOf(m_hWnd, pidlChild, dispInfo->item.pszText, SHGDN_INFOLDER, &pidlNew);
    if (SUCCEEDED(hr) && pidlNew)
    {
        CComPtr<IPersistFolder2> pPersist;
        hr = pParent->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pPersist));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        CComHeapPtr<ITEMIDLIST> pidlParent;
        hr = pPersist->GetCurFolder(&pidlParent);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        CComHeapPtr<ITEMIDLIST> pidlNewAbs(ILCombine(pidlParent, pidlNew));
        if (RenamedCurrent)
        {
            _UpdateBrowser(pidlNewAbs);
        }
        else
        {
            // Tell everyone if SetNameOf forgot, this causes IShellView to update itself when we rename a child
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_IDLIST, info->absolutePidl, pidlNewAbs);
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT CNSCBand::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    NMHDR *pnmhdr = (NMHDR*)lParam;
    switch (pnmhdr->code)
    {
        case TVN_ITEMEXPANDING:
            return OnTreeItemExpanding((LPNMTREEVIEW)lParam);
        //case TVN_SINGLEEXPAND:
        case TVN_SELCHANGED:
            if (pnmhdr->hwndFrom == m_hwndTreeView)
                _OnSelectionChanged((LPNMTREEVIEW)lParam);
            break;
        case TVN_DELETEITEM:
            OnTreeItemDeleted((LPNMTREEVIEW)lParam);
            break;
        case NM_CLICK:
        case NM_RCLICK:
            if (pnmhdr->hwndFrom == m_hwndTreeView)
            {
                TVHITTESTINFO HitTest;
                ::GetCursorPos(&HitTest.pt);
                ::ScreenToClient(m_hwndTreeView, &HitTest.pt);
                TreeView_HitTest(m_hwndTreeView, &HitTest);

                if (HitTest.flags & (TVHT_ABOVE | TVHT_BELOW | TVHT_NOWHERE))
                    return TRUE; // Prevents click processing

                if (HitTest.flags & TVHT_ONITEMBUTTON) // [+] / [-]
                    break; // Do default processing

                // Generate selection notification even if same item
                m_hwndTreeView.SendMessage(WM_SETREDRAW, FALSE, 0);
                TreeView_SelectItem(m_hwndTreeView, NULL);
                TreeView_SelectItem(m_hwndTreeView, HitTest.hItem);
                m_hwndTreeView.SendMessage(WM_SETREDRAW, TRUE, 0);

                if (pnmhdr->code == NM_CLICK)
                    return TRUE; // Prevents click processing
            }
            break;
        case TVN_BEGINDRAG:
        case TVN_BEGINRDRAG:
            OnTreeItemDragging((LPNMTREEVIEW)lParam, pnmhdr->code == TVN_BEGINRDRAG);
            break;
        case TVN_BEGINLABELEDITW:
            return OnBeginLabelEdit((LPNMTVDISPINFO)lParam);
        case TVN_ENDLABELEDITW:
            return OnEndLabelEdit((LPNMTVDISPINFO)lParam);
        default:
            break;
    }

    return 0;
}

// Temporary menu
struct CMenuTemp
{
    HMENU m_hMenu = NULL;
    CMenuTemp(HMENU hMenu) : m_hMenu(hMenu)
    {
    }
    ~CMenuTemp()
    {
        if (m_hMenu)
            ::DestroyMenu(m_hMenu);
    }
    operator HMENU() const
    {
        return m_hMenu;
    }
};

// *** ATL event handlers ***
LRESULT CNSCBand::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HWND hwndTarget = reinterpret_cast<HWND>(wParam);
    if (hwndTarget && (hwndTarget == m_hwndToolbar || hwndTarget == m_hWnd))
    {
        FIXME("Show 'Close Toolbar' menu\n");
        return 0;
    }

    HTREEITEM hItem = TreeView_GetSelection(m_hwndTreeView);
    if (!hItem)
        return 0;

    POINT pt = { (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
    if ((UINT)lParam == (UINT)-1)
    {
        RECT rc;
        if (TreeView_GetItemRect(m_hwndTreeView, hItem, &rc, TRUE))
        {
            // Center of item rectangle
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
        }
        ClientToScreen(&pt);
    }

    CItemData *info = GetItemData(hItem);
    if (!info)
    {
        ERR("No node data, something has gone wrong\n");
        return 0;
    }

    CComPtr<IShellFolder> pFolder;
    LPCITEMIDLIST pidlChild;
    HRESULT hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pFolder), &pidlChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    CComPtr<IContextMenu> contextMenu;
    hr = pFolder->GetUIObjectOf(m_hWnd, 1, &pidlChild, IID_NULL_PPV_ARG(IContextMenu, &contextMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    IUnknown_SetSite(contextMenu, (IDeskBand *)this);

    UINT cmf = CMF_EXPLORE;
    SFGAOF attr = SFGAO_CANRENAME;
    hr = pFolder->GetAttributesOf(1, &pidlChild, &attr);
    if (SUCCEEDED(hr) && (attr & SFGAO_CANRENAME))
        cmf |= CMF_CANRENAME;

    CMenuTemp menuTemp(::CreatePopupMenu());
    UINT idCmdFirst = max(FCIDM_SHVIEWFIRST, 1);
    hr = contextMenu->QueryContextMenu(menuTemp, 0, idCmdFirst, FCIDM_SHVIEWLAST, cmf);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    enum { flags = TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON };
    UINT uCommand = ::TrackPopupMenu(menuTemp, flags, pt.x, pt.y, 0, m_hWnd, NULL);
    if (uCommand)
    {
        uCommand -= idCmdFirst;

        // Do DFM_CMD_RENAME in the treeview
        if ((cmf & CMF_CANRENAME) && SHELL_IsVerb(contextMenu, uCommand, L"rename"))
        {
            m_hwndTreeView.SetFocus();
            if (TreeView_EditLabel(m_hwndTreeView, hItem))
                m_oldSelected = hItem;
            return 0;
        }

        hr = _ExecuteCommand(contextMenu, uCommand);
    }

    return TRUE;
}

// WM_USER_SHELLEVENT
LRESULT CNSCBand::OnShellEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // We use SHCNRF_NewDelivery method
    HANDLE hChange = (HANDLE)wParam;
    DWORD dwProcID = (DWORD)lParam;

    PIDLIST_ABSOLUTE *ppidl = NULL;
    LONG lEvent;
    HANDLE hLock = SHChangeNotification_Lock(hChange, dwProcID, &ppidl, &lEvent);
    if (!hLock)
    {
        ERR("!hLock\n");
        return 0;
    }

    OnChangeNotify(ppidl[0], ppidl[1], (lEvent & ~SHCNE_INTERRUPT));

    SHChangeNotification_Unlock(hLock);
    return 0;
}

// *** IOleWindow ***

STDMETHODIMP CNSCBand::GetWindow(HWND *lphwnd)
{
    if (!lphwnd)
        return E_INVALIDARG;
    *lphwnd = m_hWnd;
    return S_OK;
}

STDMETHODIMP CNSCBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDockingWindow ***

STDMETHODIMP CNSCBand::CloseDW(DWORD dwReserved)
{
    // We do nothing, we don't have anything to save yet
    TRACE("CloseDW called\n");
    return S_OK;
}

STDMETHODIMP CNSCBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* Must return E_NOTIMPL according to MSDN */
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::ShowDW(BOOL fShow)
{
    m_fVisible = fShow;
    ShowWindow(fShow ? SW_SHOW : SW_HIDE);
    return S_OK;
}

// *** IDeskBand ***

STDMETHODIMP CNSCBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!pdbi)
        return E_INVALIDARG;

    m_dwBandID = dwBandID;

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 200;
        pdbi->ptMinSize.y = 30;
    }

    if (pdbi->dwMask & DBIM_MAXSIZE)
        pdbi->ptMaxSize.y = -1;

    if (pdbi->dwMask & DBIM_INTEGRAL)
        pdbi->ptIntegral.y = 1;

    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 200;
        pdbi->ptActual.y = 30;
    }

    if (pdbi->dwMask & DBIM_TITLE)
    {
        _GetTitle(pdbi->wszTitle, _countof(pdbi->wszTitle));
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;

    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->dwMask &= ~DBIM_BKCOLOR;

    return S_OK;
}

// *** IObjectWithSite ***

STDMETHODIMP CNSCBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;

    if (pUnkSite == m_pSite)
        return S_OK;

    TRACE("SetSite called\n");

    if (!pUnkSite)
    {
        DestroyWindow();
        m_hWnd = NULL;
    }

    if (pUnkSite != m_pSite)
        m_pSite = NULL;

    if (!pUnkSite)
        return S_OK;

    HWND hwndParent;
    hr = IUnknown_GetWindow(pUnkSite, &hwndParent);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_INVALIDARG;

    m_pSite = pUnkSite;

    if (m_hWnd)
    {
        SetParent(hwndParent); // Change its parent
    }
    else
    {
        enum { style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN };
        this->Create(hwndParent, NULL, NULL, style, 0, 0U, NULL);
    }

    _RegisterChangeNotify();

    return S_OK;
}

STDMETHODIMP CNSCBand::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    *ppvSite = m_pSite;
    return S_OK;
}

// *** IOleCommandTarget ***

STDMETHODIMP CNSCBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IServiceProvider ***

STDMETHODIMP CNSCBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    return IUnknown_QueryService(m_pSite, guidService, riid, ppvObject);
}

// *** IContextMenu ***

STDMETHODIMP CNSCBand::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::GetCommandString(
    UINT_PTR idCmd,
    UINT uType,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IInputObject ***

STDMETHODIMP CNSCBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        m_hwndTreeView.SetFocus();
    }

    if (lpMsg)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
    }

    return S_OK;
}

STDMETHODIMP CNSCBand::HasFocusIO()
{
    return m_bFocused ? S_OK : S_FALSE;
}

STDMETHODIMP CNSCBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (lpMsg->hwnd == m_hWnd ||
        (m_isEditing && IsChild(lpMsg->hwnd)))
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }

    return S_FALSE;
}

// *** IPersist ***

STDMETHODIMP CNSCBand::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

// *** IPersistStream ***

STDMETHODIMP CNSCBand::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IWinEventHandler ***

STDMETHODIMP CNSCBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    return S_OK;
}

STDMETHODIMP CNSCBand::IsWindowOwner(HWND hWnd)
{
    return SHIsChildOrSelf(m_hWnd, hWnd);
}

// *** IBandNavigate ***

STDMETHODIMP CNSCBand::Select(LPCITEMIDLIST pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** INamespaceProxy ***

// Returns the ITEMIDLIST that should be navigated when an item is invoked.
STDMETHODIMP CNSCBand::GetNavigateTarget(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _Out_ PIDLIST_ABSOLUTE *ppidlTarget,
    _Out_ ULONG *pulAttrib)
{
    *pulAttrib = 0;
    WCHAR szPath[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, szPath))
        return E_FAIL;

    if (lstrcmpiW(PathFindExtensionW(szPath), L".lnk") == 0) // shortcut file?
    {
        WCHAR szTarget[MAX_PATH];
        HRESULT hr = SHDOCVW_GetPathOfShortcut(m_hWnd, szPath, szTarget);
        if (SUCCEEDED(hr))
        {
            lstrcpynW(szPath, szTarget, _countof(szPath));
            *pulAttrib |= SFGAO_LINK;
        }
    }

    if (PathIsDirectoryW(szPath) || PathIsRootW(szPath))
        *pulAttrib |= SFGAO_FOLDER;

    *ppidlTarget = ILCreateFromPathW(szPath);
    return S_OK;
}

// Handles a user action on an item.
STDMETHODIMP CNSCBand::Invoke(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// Called when the user has selected an item.
STDMETHODIMP CNSCBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

// Returns flags used to update the tree control.
STDMETHODIMP CNSCBand::RefreshFlags(
    _Out_ DWORD *pdwStyle,
    _Out_ DWORD *pdwExStyle,
    _Out_ DWORD *dwEnum)
{
    *pdwStyle = _GetTVStyle();
    *pdwExStyle = _GetTVExStyle();
    *dwEnum = _GetEnumFlags();
    return S_OK;
}

STDMETHODIMP CNSCBand::CacheItem(
    _In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDropTarget methods ***
STDMETHODIMP CNSCBand::DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ERR("Entering drag\n");
    m_pCurObject = pObj;
    m_oldSelected = TreeView_GetSelection(m_hwndTreeView);
    return DragOver(glfKeyState, pt, pdwEffect);
}

STDMETHODIMP CNSCBand::DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    TVHITTESTINFO info;
    info.pt.x = pt.x;
    info.pt.y = pt.y;
    info.flags = TVHT_ONITEM;
    info.hItem = NULL;
    ScreenToClient(&info.pt);

    // Move to the item selected by the treeview (don't change right pane)
    TreeView_HitTest(m_hwndTreeView, &info);

    HRESULT hr;
    if (!info.hItem)
    {
        m_childTargetNode = NULL;
        m_pDropTarget = NULL;
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    ++m_mtxBlockNavigate;
    TreeView_SelectItem(m_hwndTreeView, info.hItem);
    --m_mtxBlockNavigate;

    // Delegate to shell folder
    if (m_pDropTarget && info.hItem != m_childTargetNode)
        m_pDropTarget = NULL;

    if (info.hItem != m_childTargetNode)
    {
        CItemData *pItemData = GetItemData(info.hItem);
        if (!pItemData)
            return E_FAIL;

        CComPtr<IShellFolder> pFolder;
        if (_ILIsDesktop(pItemData->absolutePidl))
        {
            pFolder = m_pDesktop;
        }
        else
        {
            hr = m_pDesktop->BindToObject(pItemData->absolutePidl, 0, IID_PPV_ARG(IShellFolder, &pFolder));
            if (!SUCCEEDED(hr))
            {
                /* Don't allow dnd since we couldn't get our folder object */
                ERR("Can't bind to folder object\n");
                *pdwEffect = DROPEFFECT_NONE;
                return E_FAIL;
            }
        }

        hr = pFolder->CreateViewObject(m_hWnd, IID_PPV_ARG(IDropTarget, &m_pDropTarget));
        if (!SUCCEEDED(hr))
        {
            /* Don't allow dnd since we couldn't get our drop target */
            ERR("Can't get drop target for folder object\n");
            *pdwEffect = DROPEFFECT_NONE;
            return E_FAIL;
        }

        hr = m_pDropTarget->DragEnter(m_pCurObject, glfKeyState, pt, pdwEffect);
        m_childTargetNode = info.hItem;
    }

    if (m_pDropTarget)
        hr = m_pDropTarget->DragOver(glfKeyState, pt, pdwEffect);

    return S_OK;
}

STDMETHODIMP CNSCBand::DragLeave()
{
    ++m_mtxBlockNavigate;
    TreeView_SelectItem(m_hwndTreeView, m_oldSelected);
    --m_mtxBlockNavigate;
    m_childTargetNode = NULL;
    if (m_pCurObject)
        m_pCurObject = NULL;
    return S_OK;
}

STDMETHODIMP CNSCBand::Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (!m_pDropTarget)
        return E_FAIL;
    m_pDropTarget->Drop(pObj, glfKeyState, pt, pdwEffect);
    DragLeave();
    return S_OK;
}

// *** IDropSource methods ***

STDMETHODIMP CNSCBand::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;
    if ((grfKeyState & MK_LBUTTON) || (grfKeyState & MK_RBUTTON))
        return S_OK;
    return DRAGDROP_S_DROP;
}

STDMETHODIMP CNSCBand::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
