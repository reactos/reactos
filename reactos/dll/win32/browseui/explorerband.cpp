/*
 * ReactOS Explorer
 *
 * Copyright 2016 Sylvain Deverre <deverre dot sylv at gmail dot com>
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

#include "precomp.h"
#include <commoncontrols.h>
#include <undocshell.h>

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

CExplorerBand::CExplorerBand() :
    pSite(NULL), fVisible(FALSE), bNavigating(FALSE), dwBandID(0)
{
}

CExplorerBand::~CExplorerBand()
{
}

void CExplorerBand::InitializeExplorerBand()
{
    // Init the treeview here
    HRESULT                             hr;
    LPITEMIDLIST                        pidl;
    CComPtr<IWebBrowser2>               browserService;
    SHChangeNotifyEntry                 shcne;

    hr = SHGetDesktopFolder(&pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = SHGetFolderLocation(m_hWnd, CSIDL_DESKTOP, NULL, 0, &pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    IImageList * piml;
    hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    TreeView_SetImageList(m_hWnd, (HIMAGELIST)piml, TVSIL_NORMAL);

    // Insert the root node
    hRoot = InsertItem(0, pDesktop, pidl, pidl, FALSE);
    if (!hRoot)
    {
        ERR("Failed to create root item\n");
        return;
    }

    NodeInfo* pNodeInfo = GetNodeInfo(hRoot);

    // Insert child nodes
    InsertSubitems(hRoot, pNodeInfo);
    TreeView_Expand(m_hWnd, hRoot, TVE_EXPAND);

    // Navigate to current folder position
    NavigateToCurrentFolder();

    // Register shell notification
    shcne.pidl = pidl;
    shcne.fRecursive = TRUE;
    shellRegID = SHChangeNotifyRegister(
        m_hWnd,
        SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_RecursiveInterrupt,
        SHCNE_DISKEVENTS | SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_MKDIR,
        WM_USER_SHELLEVENT,
        1,
        &shcne);
    if (!shellRegID)
    {
        ERR("Something went wrong, error %08x\n", GetLastError());
    }
    // Register browser connection endpoint
    hr = IUnknown_QueryService(pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlAdvise(browserService, dynamic_cast<IDispatch*>(this), DIID_DWebBrowserEvents, &adviseCookie);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    ILFree(pidl);
}

void CExplorerBand::DestroyExplorerBand()
{
    HRESULT hr;
    CComPtr <IWebBrowser2> browserService;

    TRACE("Cleaning up explorer band ...\n");

    hr = IUnknown_QueryService(pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlUnadvise(browserService, DIID_DWebBrowserEvents, adviseCookie);
    /* Remove all items of the treeview */
    RevokeDragDrop(m_hWnd);
    TreeView_DeleteAllItems(m_hWnd);
    pDesktop = NULL;
    hRoot = NULL;
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

HRESULT CExplorerBand::ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd)
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

HRESULT CExplorerBand::UpdateBrowser(LPITEMIDLIST pidlGoto)
{
    CComPtr<IShellBrowser>              pBrowserService;
    HRESULT                             hr;

    hr = IUnknown_QueryService(pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &pBrowserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBrowserService->BrowseObject(pidlGoto, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

// *** notifications handling ***
BOOL CExplorerBand::OnTreeItemExpanding(LPNMTREEVIEW pnmtv)
{
    NodeInfo *pNodeInfo;

    if (pnmtv->action == TVE_COLLAPSE) {
        if (pnmtv->itemNew.hItem == hRoot)
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

void CExplorerBand::OnSelectionChanged(LPNMTREEVIEW pnmtv)
{
    NodeInfo* pNodeInfo = GetNodeInfo(pnmtv->itemNew.hItem);

    /* Prevents navigation if selection is initiated inside the band */
    if (bNavigating)
        return;

    UpdateBrowser(pNodeInfo->absolutePidl);

    SetFocus();
    // Expand the node
    //TreeView_Expand(m_hWnd, pnmtv->itemNew.hItem, TVE_EXPAND);
}


// *** ATL event handlers ***
LRESULT CExplorerBand::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HTREEITEM                           item;
    NodeInfo                            *info;
    HMENU                               treeMenu;
    WORD                                x;
    WORD                                y;
    CComPtr<IShellFolder>               pFolder;
    CComPtr<IContextMenu>               contextMenu;
    HRESULT                             hr;
    UINT                                uCommand;
    LPITEMIDLIST                        pidlChild;

    treeMenu = NULL;
    item = TreeView_GetSelection(m_hWnd);
    bHandled = TRUE;
    if (!item)
    {
        goto Cleanup;
    }

    x = LOWORD(lParam);
    y = HIWORD(lParam);
    if (x == -1 && y == -1)
    {
        // TODO: grab position of tree item and position it correctly
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
    treeMenu = CreatePopupMenu();
    hr = contextMenu->QueryContextMenu(treeMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST,
        CMF_EXPLORE);
    if (!SUCCEEDED(hr))
    {
        WARN("Can't get context menu for item\n");
        DestroyMenu(treeMenu);
        goto Cleanup;
    }
    uCommand = TrackPopupMenu(treeMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        x, y, 0, m_hWnd, NULL);

    ExecuteCommand(contextMenu, uCommand);
Cleanup:
    if (treeMenu)
        DestroyMenu(treeMenu);
    bNavigating = TRUE;
    TreeView_SelectItem(m_hWnd, oldSelected);
    bNavigating = FALSE;
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
        oldSelected = TreeView_GetSelection(m_hWnd);

        // Move to the item selected by the treeview (don't change right pane)
        TreeView_HitTest(m_hWnd, &info);
        bNavigating = TRUE;
        TreeView_SelectItem(m_hWnd, info.hItem);
        bNavigating = FALSE;
    }
    return FALSE; /* let the wndproc process the message */
}
// *** Helper functions ***
HTREEITEM CExplorerBand::InsertItem(HTREEITEM hParent, IShellFolder *psfParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort)
{
    TV_INSERTSTRUCT                     tvInsert;
    HTREEITEM                           htiCreated;

    /* Get the attributes of the node */
    SFGAOF attrs = SFGAO_STREAM | SFGAO_HASSUBFOLDER;
    HRESULT hr = psfParent->GetAttributesOf(1, &pEltRelative, &attrs);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Ignore streams */
    if ((attrs & SFGAO_STREAM))
    {
        TRACE("Ignoring stream\n");
        return NULL;
    }

    /* Get the name of the node */
    WCHAR wszDisplayName[MAX_PATH];
    if (!ILGetDisplayNameEx(psfParent, pElt, wszDisplayName, ILGDN_INFOLDER))
    {
        ERR("Failed to get node name\n");
        return NULL;
    }

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

    return htiCreated;
}

/* This is the slow version of the above method */
HTREEITEM CExplorerBand::InsertItem(HTREEITEM hParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort)
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

    if (!pDesktop->CompareIDs(NULL, pidlSub, entry))
    {
        // We are the desktop, so use pDesktop as pFolder
        pFolder = pDesktop;
    }
    else
    {
        // Get an IShellFolder of our pidl
        hr = pDesktop->BindToObject(entry, NULL, IID_PPV_ARG(IShellFolder, &pFolder));
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
BOOL CExplorerBand::NavigateToPIDL(LPITEMIDLIST dest, HTREEITEM *item, BOOL bExpand, BOOL bInsert,
        BOOL bSelect)
{
    HTREEITEM                           current;
    HTREEITEM                           tmp;
    HTREEITEM                           parent;
    BOOL                                found;
    NodeInfo                            *nodeData;
    LPITEMIDLIST                        relativeChild;
    TVITEM                              tvItem;

    if (!item)
        return FALSE;

    found = FALSE;
    current = hRoot;
    parent = NULL;
    while(!found)
    {
        nodeData = GetNodeInfo(current);
        if (!nodeData)
        {
            ERR("Something has gone wrong, no data associated to node !\n");
            *item = NULL;
            return FALSE;
        }
        // If we found our node, give it back
        if (!pDesktop->CompareIDs(0, nodeData->absolutePidl, dest))
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
    return FALSE;
}

BOOL CExplorerBand::NavigateToCurrentFolder()
{
    LPITEMIDLIST                        explorerPidl;
    CComPtr<IBrowserService>            pBrowserService;
    HRESULT                             hr;
    HTREEITEM                           dummy;
    BOOL                                result; 
    explorerPidl = NULL;

    hr = IUnknown_QueryService(pSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &pBrowserService));
    if (!SUCCEEDED(hr))
    {
        ERR("Can't get IBrowserService !\n");
        return FALSE;
    }

    hr = pBrowserService->GetPidl(&explorerPidl);
    if (!SUCCEEDED(hr) || !explorerPidl)
    {
        ERR("Unable to get browser PIDL !\n");
        return FALSE;
    }
    bNavigating = TRUE;
    /* find PIDL into our explorer */
    result = NavigateToPIDL(explorerPidl, &dummy, TRUE, FALSE, TRUE);
    bNavigating = FALSE;
    return result;
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
    fVisible = fShow;
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
    this->dwBandID = dwBandID;

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

    if (pUnkSite == pSite)
        return S_OK;

    TRACE("SetSite called \n");
    if (!pUnkSite)
    {
        DestroyExplorerBand();
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
    *ppvSite = pSite;
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
    UNIMPLEMENTED;
    return E_NOTIMPL;
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
    return bFocused ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::TranslateAcceleratorIO(LPMSG lpMsg)
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
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    BOOL bHandled;
    if (uMsg == WM_NOTIFY)
    {
        NMHDR *pNotifyHeader = (NMHDR*)lParam;
        switch (pNotifyHeader->code)
        {
            case TVN_ITEMEXPANDING:
                *theResult = OnTreeItemExpanding((LPNMTREEVIEW)lParam);
                break;
            case TVN_SELCHANGED:
                OnSelectionChanged((LPNMTREEVIEW)lParam);
                break;
            case NM_RCLICK:
                OnContextMenu(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos(), bHandled);
                *theResult = 1;
                break;
            case TVN_BEGINLABELEDITW:
            {
                // TODO: put this in a function ? (mostly copypasta from CDefView)
                DWORD dwAttr = SFGAO_CANRENAME;
                LPNMTVDISPINFO dispInfo = (LPNMTVDISPINFO)lParam;
                CComPtr<IShellFolder> pParent;
                LPCITEMIDLIST pChild;
                HRESULT hr;

                *theResult = 1;
                NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
                if (!info)
                    return E_FAIL;
                hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pChild);
                if (!SUCCEEDED(hr) || !pParent.p)
                    return E_FAIL;

                hr = pParent->GetAttributesOf(1, &pChild, &dwAttr);
                if (SUCCEEDED(hr) && (dwAttr & SFGAO_CANRENAME))
                    *theResult = 0;
                return S_OK;
            }
            case TVN_ENDLABELEDITW:
            {
                LPNMTVDISPINFO dispInfo = (LPNMTVDISPINFO)lParam;
                NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
                HRESULT hr;

                *theResult = 0;
                if (dispInfo->item.pszText)
                {
                    LPITEMIDLIST pidlNew;
                    CComPtr<IShellFolder> pParent;
                    LPCITEMIDLIST pidlChild;

                    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pidlChild);
                if (!SUCCEEDED(hr) || !pParent.p)
                    return E_FAIL;

                    hr = pParent->SetNameOf(0, pidlChild, dispInfo->item.pszText, SHGDN_INFOLDER, &pidlNew);
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

                        // Navigate to our new location
                        UpdateBrowser(pidlNewAbs);

                        ILFree(pidlNewAbs);
                        ILFree(pidlNew);
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
HRESULT STDMETHODCALLTYPE CExplorerBand::GetNavigateTarget(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::OnSelectionChanged(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::RefreshFlags(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::CacheItem(long paramC)
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
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragLeave()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDropSource methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GiveFeedback(DWORD dwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
