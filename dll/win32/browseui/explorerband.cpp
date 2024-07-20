/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Explorer bar
 * COPYRIGHT:   Copyright 2016 Sylvain Deverre <deverre.sylv@gmail.com>
 *              Copyright 2020-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <commoncontrols.h>
#include <undocshell.h>
#include "utility.h"

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

/*
 * TODO:
 *  - Monitor correctly "external" shell interrupts (seems like we need to register/deregister them for each folder)
 *  - find and fix what cause explorer crashes sometimes (seems to be explorer that does more releases than addref)
 *  - TESTING
 */

typedef struct _PIDLDATA
{
    BYTE type;
    BYTE data[1];
} PIDLDATA, *LPPIDLDATA;

#define PT_GUID 0x1F
#define PT_SHELLEXT 0x2E
#define PT_YAGUID 0x70

static BOOL _ILIsSpecialFolder (LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = (LPPIDLDATA)&pidl->mkid.abID;

    return (pidl &&
        ((lpPData && (PT_GUID == lpPData->type || PT_SHELLEXT== lpPData->type ||
        PT_YAGUID == lpPData->type)) || (pidl && pidl->mkid.cb == 0x00)));
}

HRESULT GetDisplayName(LPCITEMIDLIST pidlDirectory,TCHAR *szDisplayName,UINT cchMax,DWORD uFlags)
{
    IShellFolder *pShellFolder = NULL;
    LPCITEMIDLIST pidlRelative = NULL;
    STRRET str;
    HRESULT hr;

    if (pidlDirectory == NULL || szDisplayName == NULL)
    {
        return E_FAIL;
    }

    hr = SHBindToParent(pidlDirectory, IID_PPV_ARG(IShellFolder, &pShellFolder), &pidlRelative);

    if (SUCCEEDED(hr))
    {
        hr = pShellFolder->GetDisplayNameOf(pidlRelative,uFlags,&str);
        if (SUCCEEDED(hr))
        {
            hr = StrRetToBuf(&str,pidlDirectory,szDisplayName,cchMax);
        }
        pShellFolder->Release();
    }
    return hr;
}

CExplorerBand::CExplorerBand()
    : m_pSite(NULL)
    , m_fVisible(FALSE)
    , m_mtxBlockNavigate(0)
    , m_dwBandID(0)
    , m_isEditing(FALSE)
    , m_pidlCurrent(NULL)
{
}

CExplorerBand::~CExplorerBand()
{
    if (m_pidlCurrent)
    {
        ILFree(m_pidlCurrent);
    }
}

void CExplorerBand::InitializeExplorerBand()
{
    // Init the treeview here
    HRESULT hr = SHGetDesktopFolder(&m_pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    LPITEMIDLIST pidl;
    hr = SHGetFolderLocation(m_hWnd, CSIDL_DESKTOP, NULL, 0, &pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    IImageList * piml;
    hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    TreeView_SetImageList(m_hWnd, (HIMAGELIST)piml, TVSIL_NORMAL);

    // Insert the root node
    m_hRoot = InsertItem(NULL, m_pDesktop, pidl, pidl, FALSE);
    if (!m_hRoot)
    {
        ERR("Failed to create root item\n");
        return;
    }

    NodeInfo* pNodeInfo = GetNodeInfo(m_hRoot);

    // Insert child nodes
    InsertSubitems(m_hRoot, pNodeInfo);
    TreeView_Expand(m_hWnd, m_hRoot, TVE_EXPAND);

    // Navigate to current folder position
    NavigateToCurrentFolder();

#define TARGET_EVENTS ( \
    SHCNE_DRIVEADD | SHCNE_MKDIR | SHCNE_CREATE | SHCNE_DRIVEREMOVED | SHCNE_RMDIR | \
    SHCNE_DELETE | SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM | SHCNE_UPDATEDIR | \
    SHCNE_UPDATEITEM | SHCNE_ASSOCCHANGED \
)
    // Register shell notification
    SHChangeNotifyEntry shcne = { pidl, TRUE };
    m_shellRegID = SHChangeNotifyRegister(m_hWnd,
                                          SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                                          TARGET_EVENTS,
                                          WM_USER_SHELLEVENT,
                                          1, &shcne);
    if (!m_shellRegID)
    {
        ERR("Something went wrong, error %08x\n", GetLastError());
    }

    // Register browser connection endpoint
    CComPtr<IWebBrowser2> browserService;
    hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlAdvise(browserService, dynamic_cast<IDispatch*>(this), DIID_DWebBrowserEvents, &m_adviseCookie);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    ILFree(pidl);
}

void CExplorerBand::DestroyExplorerBand()
{
    HRESULT hr;
    CComPtr <IWebBrowser2> browserService;

    TRACE("Cleaning up explorer band ...\n");

    hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlUnadvise(browserService, DIID_DWebBrowserEvents, m_adviseCookie);
    /* Remove all items of the treeview */
    RevokeDragDrop(m_hWnd);
    TreeView_DeleteAllItems(m_hWnd);
    m_pDesktop = NULL;
    m_hRoot = NULL;
    TRACE("Cleanup done !\n");
}

CExplorerBand::NodeInfo* CExplorerBand::GetNodeInfo(HTREEITEM hItem)
{
    TVITEM tvItem;

    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = hItem;

    if (!TreeView_GetItem(m_hWnd, &tvItem))
        return 0;

    return reinterpret_cast<NodeInfo*>(tvItem.lParam);
}

static HRESULT GetCurrentLocationFromView(IShellView &View, PIDLIST_ABSOLUTE &pidl)
{
    CComPtr<IFolderView> pfv;
    CComPtr<IShellFolder> psf;
    HRESULT hr = View.QueryInterface(IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr) && SUCCEEDED(hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf))))
        hr = SHELL_GetIDListFromObject(psf, &pidl);
    return hr;
}

HRESULT CExplorerBand::GetCurrentLocation(PIDLIST_ABSOLUTE &pidl)
{
    pidl = NULL;
    CComPtr<IShellBrowser> psb;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IBrowserService> pbs;
    if (SUCCEEDED(hr = psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        if (SUCCEEDED(hr = pbs->GetPidl(&pidl)) && pidl)
            return hr;

    CComPtr<IShellView> psv;
    if (!FAILED_UNEXPECTEDLY(hr = psb->QueryActiveShellView(&psv)))
        if (SUCCEEDED(hr = psv.p ? GetCurrentLocationFromView(*psv.p, pidl) : E_FAIL))
            return hr;
    return hr;
}

HRESULT CExplorerBand::IsCurrentLocation(PCIDLIST_ABSOLUTE pidl)
{
    if (!pidl)
        return E_INVALIDARG;
    HRESULT hr = E_FAIL;
    PIDLIST_ABSOLUTE location = m_pidlCurrent;
    if (location || SUCCEEDED(hr = GetCurrentLocation(location)))
        hr = SHELL_IsEqualAbsoluteID(location, pidl) ? S_OK : S_FALSE;
    if (location != m_pidlCurrent)
        ILFree(location);
    return hr;
}

HRESULT CExplorerBand::ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd)
{
    CComPtr<IOleWindow>                 pBrowserOleWnd;
    CMINVOKECOMMANDINFO                 cmi;
    HWND                                browserWnd;
    HRESULT                             hr;

    hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IOleWindow, &pBrowserOleWnd));
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

HRESULT CExplorerBand::UpdateBrowser(LPITEMIDLIST pidlGoto)
{
    CComPtr<IShellBrowser>              pBrowserService;
    HRESULT                             hr;

    hr = IUnknown_QueryService(m_pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &pBrowserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBrowserService->BrowseObject(pidlGoto, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    ILFree(m_pidlCurrent);
    return SHILClone(pidlGoto, &m_pidlCurrent);
}

// *** notifications handling ***
BOOL CExplorerBand::OnTreeItemExpanding(LPNMTREEVIEW pnmtv)
{
    NodeInfo *pNodeInfo;

    if (pnmtv->action == TVE_COLLAPSE) {
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
    if (pnmtv->action == TVE_EXPAND) {
        // Grab our directory PIDL
        pNodeInfo = GetNodeInfo(pnmtv->itemNew.hItem);
        // We have it, let's try
        if (pNodeInfo && !pNodeInfo->expanded)
            if (!InsertSubitems(pnmtv->itemNew.hItem, pNodeInfo)) {
                // remove subitem "+" since we failed to add subitems
                TV_ITEM tvItem;

                tvItem.mask = TVIF_CHILDREN;
                tvItem.hItem = pnmtv->itemNew.hItem;
                tvItem.cChildren = 0;

                TreeView_SetItem(m_hWnd, &tvItem);
            }
    }
    return FALSE;
}

BOOL CExplorerBand::OnTreeItemDeleted(LPNMTREEVIEW pnmtv)
{
    // Navigate to parent when deleting selected item
    HTREEITEM hItem = pnmtv->itemOld.hItem;
    HTREEITEM hParent = TreeView_GetParent(m_hWnd, hItem);
    if (hParent && TreeView_GetSelection(m_hWnd) == hItem)
        TreeView_SelectItem(m_hWnd, hParent);

    /* Destroy memory associated to our node */
    NodeInfo* pNode = GetNodeInfo(hItem);
    if (!pNode)
        return FALSE;

    ILFree(pNode->relativePidl);
    ILFree(pNode->absolutePidl);
    delete pNode;

    return TRUE;
}

void CExplorerBand::OnSelectionChanged(LPNMTREEVIEW pnmtv)
{
    NodeInfo* pNodeInfo = GetNodeInfo(pnmtv->itemNew.hItem);

    /* Prevents navigation if selection is initiated inside the band */
    if (m_mtxBlockNavigate)
        return;

    UpdateBrowser(pNodeInfo->absolutePidl);

    SetFocus();
    // Expand the node
    //TreeView_Expand(m_hWnd, pnmtv->itemNew.hItem, TVE_EXPAND);
}

void CExplorerBand::OnTreeItemDragging(LPNMTREEVIEW pnmtv, BOOL isRightClick)
{
    if (!pnmtv->itemNew.lParam)
        return;

    NodeInfo* pNodeInfo = GetNodeInfo(pnmtv->itemNew.hItem);

    HRESULT hr;
    CComPtr<IShellFolder> pSrcFolder;
    LPCITEMIDLIST pLast;
    hr = SHBindToParent(pNodeInfo->absolutePidl, IID_PPV_ARG(IShellFolder, &pSrcFolder), &pLast);
    if (!SUCCEEDED(hr))
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
    if (!SUCCEEDED(hr))
        return;

    DoDragDrop(pObj, this, dwEffect, &dwEffect);
}

// *** ATL event handlers ***
LRESULT CExplorerBand::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HTREEITEM                           item;
    NodeInfo                            *info;
    HMENU                               treeMenu;
    POINT                               pt;
    CComPtr<IShellFolder>               pFolder;
    CComPtr<IContextMenu>               contextMenu;
    HRESULT                             hr;
    UINT                                uCommand;
    LPITEMIDLIST                        pidlChild;
    UINT                                cmdBase = max(FCIDM_SHVIEWFIRST, 1);
    UINT                                cmf = CMF_EXPLORE;
    SFGAOF                              attr = SFGAO_CANRENAME;
    BOOL                                startedRename = FALSE;

    treeMenu = NULL;
    item = TreeView_GetSelection(m_hWnd);
    bHandled = TRUE;
    if (!item)
    {
        goto Cleanup;
    }

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    if ((UINT)lParam == (UINT)-1)
    {
        RECT r;
        if (TreeView_GetItemRect(m_hWnd, item, &r, TRUE))
        {
            pt.x = (r.left + r.right) / 2; // Center of
            pt.y = (r.top + r.bottom) / 2; // item rectangle
        }
        ClientToScreen(&pt);
    }

    info = GetNodeInfo(item);
    if (!info)
    {
        ERR("No node data, something has gone wrong !\n");
        goto Cleanup;
    }
    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pFolder),
        (LPCITEMIDLIST*)&pidlChild);
    if (!SUCCEEDED(hr))
    {
        ERR("Can't bind to folder!\n");
        goto Cleanup;
    }
    hr = pFolder->GetUIObjectOf(m_hWnd, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu,
        NULL, reinterpret_cast<void**>(&contextMenu));
    if (!SUCCEEDED(hr))
    {
        ERR("Can't get IContextMenu interface\n");
        goto Cleanup;
    }

    IUnknown_SetSite(contextMenu, (IDeskBand *)this);

    if (SUCCEEDED(pFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlChild, &attr)) && (attr & SFGAO_CANRENAME))
        cmf |= CMF_CANRENAME;

    treeMenu = CreatePopupMenu();
    hr = contextMenu->QueryContextMenu(treeMenu, 0, cmdBase, FCIDM_SHVIEWLAST, cmf);
    if (!SUCCEEDED(hr))
    {
        WARN("Can't get context menu for item\n");
        DestroyMenu(treeMenu);
        goto Cleanup;
    }

    uCommand = TrackPopupMenu(treeMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, m_hWnd, NULL);
    if (uCommand)
    {
        uCommand -= cmdBase;

        // Do DFM_CMD_RENAME in the treeview
        if ((cmf & CMF_CANRENAME) && SHELL_IsVerb(contextMenu, uCommand, L"rename"))
        {
            HTREEITEM oldSelected = m_oldSelected;
            SetFocus();
            startedRename = TreeView_EditLabel(m_hWnd, item) != NULL;
            m_oldSelected = oldSelected; // Restore after TVN_BEGINLABELEDIT
            goto Cleanup;
        }

        hr = ExecuteCommand(contextMenu, uCommand);
    }

Cleanup:
    if (contextMenu)
        IUnknown_SetSite(contextMenu, NULL);
    if (treeMenu)
        DestroyMenu(treeMenu);
    if (startedRename)
    {
        // The treeview disables drawing of the edited item so we must make sure
        // the correct item is selected (on right-click -> rename on not-current folder).
        // TVN_ENDLABELEDIT becomes responsible for restoring the selection.
    }
    else
    {
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hWnd, m_oldSelected);
        --m_mtxBlockNavigate;
    }
    return TRUE;
}

LRESULT CExplorerBand::ContextMenuHack(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;
    if (uMsg == WM_RBUTTONDOWN)
    {
        TVHITTESTINFO info;
        info.pt.x = LOWORD(lParam);
        info.pt.y = HIWORD(lParam);
        info.flags = TVHT_ONITEM;
        info.hItem = NULL;

        // Save the current location
        m_oldSelected = TreeView_GetSelection(m_hWnd);

        // Move to the item selected by the treeview (don't change right pane)
        TreeView_HitTest(m_hWnd, &info);
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hWnd, info.hItem);
        --m_mtxBlockNavigate;
    }
    return FALSE; /* let the wndproc process the message */
}

// WM_USER_SHELLEVENT
LRESULT CExplorerBand::OnShellEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // We use SHCNRF_NewDelivery method
    HANDLE hChange = (HANDLE)wParam;
    DWORD dwProcID = (DWORD)lParam;

    PIDLIST_ABSOLUTE *ppidl = NULL;
    LONG lEvent;
    HANDLE hLock = SHChangeNotification_Lock(hChange, dwProcID, &ppidl, &lEvent);
    if (hLock == NULL)
    {
        ERR("hLock == NULL\n");
        return 0;
    }

    OnChangeNotify(ppidl[0], ppidl[1], (lEvent & ~SHCNE_INTERRUPT));

    SHChangeNotification_Unlock(hLock);
    return 0;
}

BOOL
CExplorerBand::IsTreeItemInEnum(
    _In_ HTREEITEM hItem,
    _In_ IEnumIDList *pEnum)
{
    NodeInfo* pNodeInfo = GetNodeInfo(hItem);
    if (!pNodeInfo)
        return FALSE;

    pEnum->Reset();

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
    {
        if (ILIsEqual(pidlTemp, pNodeInfo->relativePidl))
            return TRUE;

        pidlTemp.Free();
    }

    return FALSE;
}

BOOL
CExplorerBand::TreeItemHasThisChild(
    _In_ HTREEITEM hItem,
    _In_ PCITEMID_CHILD pidlChild)
{
    for (hItem = TreeView_GetChild(m_hWnd, hItem); hItem;
         hItem = TreeView_GetNextSibling(m_hWnd, hItem))
    {
        NodeInfo* pNodeInfo = GetNodeInfo(hItem);
        if (ILIsEqual(pNodeInfo->relativePidl, pidlChild))
            return TRUE;
    }

    return FALSE;
}

HRESULT
CExplorerBand::GetItemEnum(
    _Out_ CComPtr<IEnumIDList>& pEnum,
    _In_ HTREEITEM hItem)
{
    NodeInfo* pNodeInfo = GetNodeInfo(hItem);

    CComPtr<IShellFolder> psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> pFolder;
    hr = psfDesktop->BindToObject(pNodeInfo->absolutePidl, NULL, IID_PPV_ARG(IShellFolder, &pFolder));
    if (FAILED(hr))
        return hr;

    return pFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &pEnum);
}

BOOL CExplorerBand::ItemHasAnyChild(_In_ HTREEITEM hItem)
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hr = GetItemEnum(pEnum, hItem);
    if (FAILED(hr))
        return FALSE;

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    hr = pEnum->Next(1, &pidlTemp, NULL);
    return SUCCEEDED(hr);
}

void CExplorerBand::RefreshRecurse(_In_ HTREEITEM hTarget)
{
    NodeInfo* pNodeInfo = GetNodeInfo(hTarget);

    CComPtr<IEnumIDList> pEnum;
    HRESULT hrEnum = GetItemEnum(pEnum, hTarget);

    // Delete zombie items
    HTREEITEM hItem, hNextItem;
    for (hItem = TreeView_GetChild(m_hWnd, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(m_hWnd, hItem);

        if (SUCCEEDED(hrEnum) && !IsTreeItemInEnum(hItem, pEnum))
            TreeView_DeleteItem(m_hWnd, hItem);
    }

    pEnum = NULL;
    hrEnum = GetItemEnum(pEnum, hTarget);

    // Insert new items and update items
    if (SUCCEEDED(hrEnum))
    {
        CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
        while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
        {
            if (!TreeItemHasThisChild(hTarget, pidlTemp))
            {
                CComHeapPtr<ITEMIDLIST> pidlAbsolute(ILCombine(pNodeInfo->absolutePidl, pidlTemp));
                InsertItem(hTarget, pidlAbsolute, pidlTemp, TRUE);
            }
            pidlTemp.Free();
        }
    }

    // Update children and recurse
    for (hItem = TreeView_GetChild(m_hWnd, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(m_hWnd, hItem);

        TV_ITEMW item = { TVIF_HANDLE | TVIF_CHILDREN };
        item.hItem = hItem;
        item.cChildren = ItemHasAnyChild(hItem);
        TreeView_SetItem(m_hWnd, &item);

        if (TreeView_GetItemState(m_hWnd, hItem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
            RefreshRecurse(hItem);
    }
}

void CExplorerBand::Refresh()
{
    SendMessage(WM_SETREDRAW, FALSE, 0);
    RefreshRecurse(m_hRoot);
    SendMessage(WM_SETREDRAW, TRUE, 0);
    InvalidateRect(NULL, TRUE);
}

#define TIMER_ID_REFRESH 9999

LRESULT CExplorerBand::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam != TIMER_ID_REFRESH)
        return 0;

    KillTimer(TIMER_ID_REFRESH);

    // FIXME: Avoid full refresh and optimize for speed
    Refresh();

    return 0;
}

void
CExplorerBand::OnChangeNotify(
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

LRESULT CExplorerBand::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_bFocused = TRUE;
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), TRUE);
    bHandled = FALSE;
    return TRUE;
}

LRESULT CExplorerBand::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), FALSE);
    bHandled = FALSE;
    return TRUE;
}

// *** Helper functions ***
HTREEITEM
CExplorerBand::InsertItem(
    _In_opt_ HTREEITEM hParent,
    _Inout_ IShellFolder *psfParent,
    _In_ LPCITEMIDLIST pElt,
    _In_ LPCITEMIDLIST pEltRelative,
    _In_ BOOL bSort)
{
    TV_INSERTSTRUCT                     tvInsert;
    HTREEITEM                           htiCreated;

    /* Get the attributes of the node */
    SFGAOF attrs = SFGAO_STREAM | SFGAO_HASSUBFOLDER;
    HRESULT hr = psfParent->GetAttributesOf(1, &pEltRelative, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Ignore streams */
    if (attrs & SFGAO_STREAM)
    {
        TRACE("Ignoring stream\n");
        return NULL;
    }

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

    NodeInfo* pChildInfo = new NodeInfo;
    if (!pChildInfo)
    {
        ERR("Failed to allocate NodeInfo\n");
        return FALSE;
    }

    // Store our node info
    pChildInfo->absolutePidl = ILClone(pElt);
    pChildInfo->relativePidl = ILClone(pEltRelative);
    pChildInfo->expanded = FALSE;

    // Set up our treeview template
    tvInsert.hParent = hParent;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
    tvInsert.item.cchTextMax = MAX_PATH;
    tvInsert.item.pszText = wszDisplayName;
    tvInsert.item.iImage = tvInsert.item.iSelectedImage = iIcon;
    tvInsert.item.cChildren = (attrs & SFGAO_HASSUBFOLDER) ? 1 : 0;
    tvInsert.item.lParam = (LPARAM)pChildInfo;

    htiCreated = TreeView_InsertItem(m_hWnd, &tvInsert);

    if (bSort)
    {
        TVSORTCB sortCallback;
        sortCallback.hParent = hParent;
        sortCallback.lpfnCompare = CompareTreeItems;
        sortCallback.lParam = (LPARAM)this;
        SendMessage(TVM_SORTCHILDRENCB, 0, (LPARAM)&sortCallback);
    }

    return htiCreated;
}

/* This is the slow version of the above method */
HTREEITEM
CExplorerBand::InsertItem(
    _In_opt_ HTREEITEM hParent,
    _In_ LPCITEMIDLIST pElt,
    _In_ LPCITEMIDLIST pEltRelative,
    _In_ BOOL bSort)
{
    CComPtr<IShellFolder> psfFolder;
    HRESULT hr = SHBindToParent(pElt, IID_PPV_ARG(IShellFolder, &psfFolder), NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    return InsertItem(hParent, psfFolder, pElt, pEltRelative, bSort);
}

BOOL CExplorerBand::InsertSubitems(HTREEITEM hItem, NodeInfo *pNodeInfo)
{
    CComPtr<IEnumIDList>                pEnumIDList;
    LPITEMIDLIST                        pidlSub;
    LPITEMIDLIST                        entry;
    SHCONTF                             EnumFlags;
    HRESULT                             hr;
    ULONG                               fetched;
    ULONG                               uItemCount;
    CComPtr<IShellFolder>               pFolder;
    TVSORTCB                            sortCallback;

    entry = pNodeInfo->absolutePidl;
    fetched = 1;
    uItemCount = 0;
    EnumFlags = SHCONTF_FOLDERS;

    hr = SHGetFolderLocation(m_hWnd, CSIDL_DESKTOP, NULL, 0, &pidlSub);
    if (!SUCCEEDED(hr))
    {
        ERR("Can't get desktop PIDL !\n");
        return FALSE;
    }

    if (!m_pDesktop->CompareIDs(NULL, pidlSub, entry))
    {
        // We are the desktop, so use pDesktop as pFolder
        pFolder = m_pDesktop;
    }
    else
    {
        // Get an IShellFolder of our pidl
        hr = m_pDesktop->BindToObject(entry, NULL, IID_PPV_ARG(IShellFolder, &pFolder));
        if (!SUCCEEDED(hr))
        {
            ILFree(pidlSub);
            ERR("Can't bind folder to desktop !\n");
            return FALSE;
        }
    }
    ILFree(pidlSub);

    // TODO: handle hidden folders according to settings !
    EnumFlags |= SHCONTF_INCLUDEHIDDEN;

    // Enum through objects
    hr = pFolder->EnumObjects(NULL,EnumFlags,&pEnumIDList);

    // avoid broken IShellFolder implementations that return null pointer with success
    if (!SUCCEEDED(hr) || !pEnumIDList)
    {
        ERR("Can't enum the folder !\n");
        return FALSE;
    }

    /* Don't redraw while we add stuff into the tree */
    SendMessage(WM_SETREDRAW, FALSE, 0);
    while(SUCCEEDED(pEnumIDList->Next(1, &pidlSub, &fetched)) && pidlSub && fetched)
    {
        LPITEMIDLIST pidlSubComplete;
        pidlSubComplete = ILCombine(entry, pidlSub);

        if (InsertItem(hItem, pFolder, pidlSubComplete, pidlSub, FALSE))
            uItemCount++;
        ILFree(pidlSubComplete);
        ILFree(pidlSub);
    }
    pNodeInfo->expanded = TRUE;
    /* Let's do sorting */
    sortCallback.hParent = hItem;
    sortCallback.lpfnCompare = CompareTreeItems;
    sortCallback.lParam = (LPARAM)this;
    SendMessage(TVM_SORTCHILDRENCB, 0, (LPARAM)&sortCallback);

    /* Now we can redraw */
    SendMessage(WM_SETREDRAW, TRUE, 0);

    return (uItemCount > 0) ? TRUE : FALSE;
}

/**
 * Navigate to a given PIDL in the treeview, and return matching tree item handle
 *  - dest: The absolute PIDL we should navigate in the treeview
 *  - item: Handle of the tree item matching the PIDL
 *  - bExpand: expand collapsed nodes in order to find the right element
 *  - bInsert: insert the element at the right place if we don't find it
 *  - bSelect: select the item after we found it
 */
BOOL CExplorerBand::NavigateToPIDL(LPCITEMIDLIST dest, HTREEITEM *item, BOOL bExpand, BOOL bInsert,
        BOOL bSelect)
{
    HTREEITEM                           current;
    HTREEITEM                           tmp;
    HTREEITEM                           parent;
    NodeInfo                            *nodeData;
    LPITEMIDLIST                        relativeChild;
    TVITEM                              tvItem;

    if (!item)
        return FALSE;

    current = m_hRoot;
    parent = NULL;
    while (TRUE)
    {
        nodeData = GetNodeInfo(current);
        if (!nodeData)
        {
            ERR("Something has gone wrong, no data associated to node !\n");
            *item = NULL;
            return FALSE;
        }
        // If we found our node, give it back
        if (!m_pDesktop->CompareIDs(0, nodeData->absolutePidl, dest))
        {
            if (bSelect)
                TreeView_SelectItem(m_hWnd, current);
            *item = current;
            return TRUE;
        }

        // Check if we are a parent of the requested item
        relativeChild = ILFindChild(nodeData->absolutePidl, dest);
        if (relativeChild != 0)
        {
            // Notify treeview we have children
            tvItem.mask = TVIF_CHILDREN;
            tvItem.hItem = current;
            tvItem.cChildren = 1;
            TreeView_SetItem(m_hWnd, &tvItem);

            // If we can expand and the node isn't expanded yet, do it
            if (bExpand)
            {
                if (!nodeData->expanded)
                    InsertSubitems(current, nodeData);
                TreeView_Expand(m_hWnd, current, TVE_EXPAND);
            }

            // Try to get a child
            tmp = TreeView_GetChild(m_hWnd, current);
            if (tmp)
            {
                // We have a child, let's continue with it
                parent = current;
                current = tmp;
                continue;
            }

            if (bInsert && nodeData->expanded)
            {
                // Happens when we have to create a subchild inside a child
                current = InsertItem(current, dest, relativeChild, TRUE);
            }
            // We end up here, without any children, so we found nothing
            // Tell the parent node it has children
            ZeroMemory(&tvItem, sizeof(tvItem));
            *item = NULL;
            return FALSE;
        }

        // Find sibling
        tmp = TreeView_GetNextSibling(m_hWnd, current);
        if (tmp)
        {
            current = tmp;
            continue;
        }
        if (bInsert)
        {
            current = InsertItem(parent, dest, ILFindLastID(dest), TRUE);
            *item = current;
            return TRUE;
        }
        *item = NULL;
        return FALSE;
    }
    UNREACHABLE;
}

BOOL CExplorerBand::NavigateToCurrentFolder()
{
    LPITEMIDLIST                        explorerPidl;
    HTREEITEM                           dummy;
    BOOL                                result;

    HRESULT hr = GetCurrentLocation(explorerPidl);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("Unable to get browser PIDL !\n");
        return FALSE;
    }
    ++m_mtxBlockNavigate;
    /* find PIDL into our explorer */
    result = NavigateToPIDL(explorerPidl, &dummy, TRUE, FALSE, TRUE);
    --m_mtxBlockNavigate;
    ILFree(explorerPidl);
    return result;
}

// *** Tree item sorting callback ***
int CALLBACK CExplorerBand::CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3)
{
    /*
     * We first sort drive letters (Path root), then PIDLs and then regular folder
     * display name.
     * This is not how Windows sorts item, but it gives decent results.
     */
    NodeInfo                            *info1;
    NodeInfo                            *info2;
    CExplorerBand                       *pThis;
    WCHAR                               wszFolder1[MAX_PATH];
    WCHAR                               wszFolder2[MAX_PATH];

    info1 = (NodeInfo*)p1;
    info2 = (NodeInfo*)p2;
    pThis = (CExplorerBand*)p3;

    GetDisplayName(info1->absolutePidl, wszFolder1, MAX_PATH, SHGDN_FORPARSING);
    GetDisplayName(info2->absolutePidl, wszFolder2, MAX_PATH, SHGDN_FORPARSING);
    if (PathIsRoot(wszFolder1) && PathIsRoot(wszFolder2))
    {
        return lstrcmpiW(wszFolder1,wszFolder2);
    }
    if (PathIsRoot(wszFolder1) && !PathIsRoot(wszFolder2))
    {
        return -1;
    }
    if (!PathIsRoot(wszFolder1) && PathIsRoot(wszFolder2))
    {
        return 1;
    }
    // Now, we compare non-root folders, grab display name
    GetDisplayName(info1->absolutePidl, wszFolder1, MAX_PATH, SHGDN_INFOLDER);
    GetDisplayName(info2->absolutePidl, wszFolder2, MAX_PATH, SHGDN_INFOLDER);

    if (_ILIsSpecialFolder(info1->relativePidl) && !_ILIsSpecialFolder(info2->relativePidl))
    {
        return -1;
    }
    if (!_ILIsSpecialFolder(info1->relativePidl) && _ILIsSpecialFolder(info2->relativePidl))
    {
        return 1;
    }
    if (_ILIsSpecialFolder(info1->relativePidl) && !_ILIsSpecialFolder(info2->relativePidl))
    {
        HRESULT hr;
        hr = pThis->m_pDesktop->CompareIDs(0, info1->absolutePidl, info2->absolutePidl);
        if (!hr) return 0;
        return (hr > 0) ? -1 : 1;
    }
    return StrCmpLogicalW(wszFolder1, wszFolder2);
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetWindow(HWND *lphwnd)
{
    if (!lphwnd)
        return E_INVALIDARG;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::CloseDW(DWORD dwReserved)
{
    // We do nothing, we don't have anything to save yet
    TRACE("CloseDW called\n");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* Must return E_NOTIMPL according to MSDN */
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ShowDW(BOOL fShow)
{
    m_fVisible = fShow;
    ShowWindow(fShow);
    return S_OK;
}


// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!pdbi)
    {
        return E_INVALIDARG;
    }
    this->m_dwBandID = dwBandID;

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
        if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_FOLDERSLABEL, pdbi->wszTitle, _countof(pdbi->wszTitle)))
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


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;
    HWND parentWnd;

    if (pUnkSite == m_pSite)
        return S_OK;

    TRACE("SetSite called \n");
    if (!pUnkSite)
    {
        DestroyExplorerBand();
        DestroyWindow();
        m_hWnd = NULL;
    }

    if (pUnkSite != m_pSite)
    {
        m_pSite = NULL;
    }

    if(!pUnkSite)
        return S_OK;

    hr = IUnknown_GetWindow(pUnkSite, &parentWnd);
    if (!SUCCEEDED(hr))
    {
        ERR("Could not get parent's window ! Status: %08lx\n", hr);
        return E_INVALIDARG;
    }

    m_pSite = pUnkSite;

    if (m_hWnd)
    {
        // Change its parent
        SetParent(parentWnd);
    }
    else
    {
        HWND wnd = CreateWindow(WC_TREEVIEW, NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS /* | TVS_SINGLEEXPAND*/ , // remove TVS_SINGLEEXPAND for now since it has strange behaviour
            0, 0, 0, 0, parentWnd, NULL, _AtlBaseModule.GetModuleInstance(), NULL);

        // Subclass the window
        SubclassWindow(wnd);

        // Initialize our treeview now
        InitializeExplorerBand();
        RegisterDragDrop(m_hWnd, dynamic_cast<IDropTarget*>(this));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    *ppvSite = m_pSite;
    return S_OK;
}


// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    /* FIXME: we probably want to handle more services here */
    return IUnknown_QueryService(m_pSite, SID_SShellBrowser, riid, ppvObject);
}


// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
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

HRESULT STDMETHODCALLTYPE CExplorerBand::HasFocusIO()
{
    return m_bFocused ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::TranslateAcceleratorIO(LPMSG lpMsg)
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

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    memcpy(pClassID, &CLSID_ExplorerBand, sizeof(CLSID));
    return S_OK;
}


// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // TODO: calculate max size
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    BOOL bHandled;
    LRESULT result;

    if (uMsg == WM_NOTIFY)
    {
        NMHDR *pNotifyHeader = (NMHDR*)lParam;
        switch (pNotifyHeader->code)
        {
            case TVN_ITEMEXPANDING:
                result = OnTreeItemExpanding((LPNMTREEVIEW)lParam);
                if (theResult)
                    *theResult = result;
                break;
            case TVN_SELCHANGED:
                OnSelectionChanged((LPNMTREEVIEW)lParam);
                break;
            case TVN_DELETEITEM:
                OnTreeItemDeleted((LPNMTREEVIEW)lParam);
                break;
            case NM_RCLICK:
                OnContextMenu(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos(), bHandled);
                if (theResult)
                    *theResult = 1;
                break;
            case TVN_BEGINDRAG:
            case TVN_BEGINRDRAG:
                OnTreeItemDragging((LPNMTREEVIEW)lParam, pNotifyHeader->code == TVN_BEGINRDRAG);
                break;
            case TVN_BEGINLABELEDITW:
            {
                // TODO: put this in a function ? (mostly copypasta from CDefView)
                DWORD dwAttr = SFGAO_CANRENAME;
                LPNMTVDISPINFO dispInfo = (LPNMTVDISPINFO)lParam;
                CComPtr<IShellFolder> pParent;
                LPCITEMIDLIST pChild;
                HRESULT hr;

                if (theResult)
                    *theResult = 1;
                NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
                if (!info)
                    return E_FAIL;
                hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pChild);
                if (!SUCCEEDED(hr) || !pParent.p)
                    return E_FAIL;

                hr = pParent->GetAttributesOf(1, &pChild, &dwAttr);
                if (SUCCEEDED(hr) && (dwAttr & SFGAO_CANRENAME))
                {
                    if (theResult)
                        *theResult = 0;
                    m_isEditing = TRUE;
                    m_oldSelected = NULL;
                }
                return S_OK;
            }
            case TVN_ENDLABELEDITW:
            {
                LPNMTVDISPINFO dispInfo = (LPNMTVDISPINFO)lParam;
                NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
                HRESULT hr;

                m_isEditing = FALSE;
                if (m_oldSelected)
                {
                    ++m_mtxBlockNavigate;
                    TreeView_SelectItem(m_hWnd, m_oldSelected);
                    --m_mtxBlockNavigate;
                }

                if (theResult)
                    *theResult = 0;
                if (dispInfo->item.pszText)
                {
                    LPITEMIDLIST pidlNew;
                    CComPtr<IShellFolder> pParent;
                    LPCITEMIDLIST pidlChild;
                    BOOL RenamedCurrent = IsCurrentLocation(info->absolutePidl) == S_OK;

                    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pidlChild);
                    if (!SUCCEEDED(hr) || !pParent.p)
                        return E_FAIL;

                    hr = pParent->SetNameOf(m_hWnd, pidlChild, dispInfo->item.pszText, SHGDN_INFOLDER, &pidlNew);
                    if(SUCCEEDED(hr) && pidlNew)
                    {
                        CComPtr<IPersistFolder2> pPersist;
                        LPITEMIDLIST pidlParent, pidlNewAbs;

                        hr = pParent->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pPersist));
                        if(!SUCCEEDED(hr))
                            return E_FAIL;

                        hr = pPersist->GetCurFolder(&pidlParent);
                        if(!SUCCEEDED(hr))
                            return E_FAIL;
                        pidlNewAbs = ILCombine(pidlParent, pidlNew);

                        if (RenamedCurrent)
                        {
                            // Navigate to our new location
                            UpdateBrowser(pidlNewAbs);
                        }
                        else
                        {
                            // Tell everyone in case SetNameOf forgot, this causes IShellView to update itself when we renamed a child
                            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_IDLIST, info->absolutePidl, pidlNewAbs);
                        }

                        ILFree(pidlParent);
                        ILFree(pidlNewAbs);
                        ILFree(pidlNew);
                        if (theResult)
                            *theResult = 1;
                    }
                    return S_OK;
                }
            }
            default:
                break;
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::IsWindowOwner(HWND hWnd)
{
    return (hWnd == m_hWnd) ? S_OK : S_FALSE;
}

// *** IBandNavigate methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::Select(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** INamespaceProxy ***

/// Returns the ITEMIDLIST that should be navigated when an item is invoked.
STDMETHODIMP CExplorerBand::GetNavigateTarget(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _Out_ PIDLIST_ABSOLUTE ppidlTarget,
    _Out_ ULONG *pulAttrib)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Handles a user action on an item.
STDMETHODIMP CExplorerBand::Invoke(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Called when the user has selected an item.
STDMETHODIMP CExplorerBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Returns flags used to update the tree control.
STDMETHODIMP CExplorerBand::RefreshFlags(
    _Out_ DWORD *pdwStyle,
    _Out_ DWORD *pdwExStyle,
    _Out_ DWORD *dwEnum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CExplorerBand::CacheItem(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch (dispIdMember)
    {
        case DISPID_DOWNLOADCOMPLETE:
        case DISPID_NAVIGATECOMPLETE2:
           TRACE("DISPID_NAVIGATECOMPLETE2 received\n");
           NavigateToCurrentFolder();
           return S_OK;
    }
    TRACE("Unknown dispid requested: %08x\n", dispIdMember);
    return E_INVALIDARG;
}

// *** IDropTarget methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ERR("Entering drag\n");
    m_pCurObject = pObj;
    m_oldSelected = TreeView_GetSelection(m_hWnd);
    return DragOver(glfKeyState, pt, pdwEffect);
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    TVHITTESTINFO                           info;
    CComPtr<IShellFolder>                   pShellFldr;
    NodeInfo                                *nodeInfo;
    //LPCITEMIDLIST                         pChild;
    HRESULT                                 hr;

    info.pt.x = pt.x;
    info.pt.y = pt.y;
    info.flags = TVHT_ONITEM;
    info.hItem = NULL;
    ScreenToClient(&info.pt);

    // Move to the item selected by the treeview (don't change right pane)
    TreeView_HitTest(m_hWnd, &info);

    if (info.hItem)
    {
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hWnd, info.hItem);
        --m_mtxBlockNavigate;
        // Delegate to shell folder
        if (m_pDropTarget && info.hItem != m_childTargetNode)
        {
            m_pDropTarget = NULL;
        }
        if (info.hItem != m_childTargetNode)
        {
            nodeInfo = GetNodeInfo(info.hItem);
            if (!nodeInfo)
                return E_FAIL;
#if 0
            hr = SHBindToParent(nodeInfo->absolutePidl, IID_PPV_ARG(IShellFolder, &pShellFldr), &pChild);
            if (!SUCCEEDED(hr))
                return E_FAIL;
            hr = pShellFldr->GetUIObjectOf(m_hWnd, 1, &pChild, IID_IDropTarget, NULL, reinterpret_cast<void**>(&pDropTarget));
            if (!SUCCEEDED(hr))
                return E_FAIL;
#endif
            if(_ILIsDesktop(nodeInfo->absolutePidl))
                pShellFldr = m_pDesktop;
            else
            {
                hr = m_pDesktop->BindToObject(nodeInfo->absolutePidl, 0, IID_PPV_ARG(IShellFolder, &pShellFldr));
                if (!SUCCEEDED(hr))
                {
                    /* Don't allow dnd since we couldn't get our folder object */
                    ERR("Can't bind to folder object\n");
                    *pdwEffect = DROPEFFECT_NONE;
                    return E_FAIL;
                }
            }
            hr = pShellFldr->CreateViewObject(m_hWnd, IID_PPV_ARG(IDropTarget, &m_pDropTarget));
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
        {
            hr = m_pDropTarget->DragOver(glfKeyState, pt, pdwEffect);
        }
    }
    else
    {
        m_childTargetNode = NULL;
        m_pDropTarget = NULL;
        *pdwEffect = DROPEFFECT_NONE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragLeave()
{
    ++m_mtxBlockNavigate;
    TreeView_SelectItem(m_hWnd, m_oldSelected);
    --m_mtxBlockNavigate;
    m_childTargetNode = NULL;
    if (m_pCurObject)
    {
        m_pCurObject = NULL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (!m_pDropTarget)
        return E_FAIL;
    m_pDropTarget->Drop(pObj, glfKeyState, pt, pdwEffect);
    DragLeave();
    return S_OK;
}

// *** IDropSource methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;
    if ((grfKeyState & MK_LBUTTON) || (grfKeyState & MK_RBUTTON))
        return S_OK;
    return DRAGDROP_S_DROP;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
