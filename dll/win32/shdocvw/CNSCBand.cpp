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
#define TIMER_ID_SELECT_ITEM 888

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

HRESULT SHELL_GetPathOfShortcut(HWND hWnd, LPCWSTR pszLnkFile, LPWSTR pszPath)
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

HRESULT SHELL_CreateShortcut(
    LPCWSTR pszLnkFileName, 
    PCIDLIST_ABSOLUTE pidlTarget,
    LPCWSTR pszDescription)
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
    if(FAILED_UNEXPECTEDLY(hr))
        return hr;

    return ppf->Save(pszLnkFileName, TRUE);
}

CNSCBand::CNSCBand()
{
    SHDOCVW_LockModule();
}

CNSCBand::~CNSCBand()
{
    if (m_hToolbarImageList)
    {
        ImageList_Destroy(m_hToolbarImageList);
        m_hToolbarImageList = NULL;
    }
    if (m_hTreeViewImageList)
    {
        ImageList_Destroy(m_hTreeViewImageList);
        m_hTreeViewImageList = NULL;
    }
    SHDOCVW_UnlockModule();
}

VOID CNSCBand::OnFinalMessage(HWND)
{
    // The message loop is finished, now we can safely destruct!
    static_cast<IDeskBand *>(this)->Release();
}

// *** helper methods ***

CNSCBand::NodeInfo* CNSCBand::GetNodeInfo(HTREEITEM hItem)
{
    if (hItem == TVI_ROOT)
        return NULL;

    TVITEM tvItem = { TVIF_PARAM, hItem };
    if (!TreeView_GetItem(m_hwndTreeView, &tvItem))
        return NULL;

    return reinterpret_cast<NodeInfo*>(tvItem.lParam);
}

static HRESULT GetCurrentLocationFromView(IShellView &View, PIDLIST_ABSOLUTE& pidl)
{
    CComPtr<IFolderView> pfv;
    CComPtr<IShellFolder> psf;
    HRESULT hr = View.QueryInterface(IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr) && SUCCEEDED(hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf))))
        hr = SHELL_GetIDListFromObject(psf, &pidl);
    return hr;
}

HRESULT CNSCBand::GetCurrentLocation(PIDLIST_ABSOLUTE *ppidl)
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
        if (SUCCEEDED(hr = psv.p ? GetCurrentLocationFromView(*psv.p, *ppidl) : E_FAIL))
            return hr;
    return hr;
}

HRESULT CNSCBand::IsCurrentLocation(PCIDLIST_ABSOLUTE pidl)
{
    if (!pidl)
        return E_INVALIDARG;
    HRESULT hr = E_FAIL;
    PIDLIST_ABSOLUTE location = NULL;
    hr = GetCurrentLocation(&location);
    if (SUCCEEDED(hr))
        hr = SHELL_IsEqualAbsoluteID(location, pidl) ? S_OK : S_FALSE;
    ILFree(location);
    return hr;
}

HRESULT CNSCBand::ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd)
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
    if (GetKeyState(VK_SHIFT) < 0)
        cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) < 0)
        cmi.fMask |= CMIC_MASK_CONTROL_DOWN;

    return menu->InvokeCommand(&cmi);
}

void CNSCBand::_Init()
{
    // Init the treeview here
    HRESULT hr = SHGetDesktopFolder(&m_pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    m_pidlRoot.Free();
    hr = SHGetFolderLocation(m_hWnd, _GetRootCsidl(), NULL, 0, &m_pidlRoot);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    IImageList *piml;
    hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    TreeView_SetImageList(m_hwndTreeView, (HIMAGELIST)piml, TVSIL_NORMAL);

    if (_WantsRootItem())
    {
        // Insert the root node
        m_hRoot = InsertItem(NULL, m_pDesktop, m_pidlRoot, m_pidlRoot, FALSE);
        if (!m_hRoot)
        {
            ERR("Failed to create root item\n");
            return;
        }
        TreeView_Expand(m_hwndTreeView, m_hRoot, TVE_EXPAND);

        // Navigate to current folder position
        NavigateToCurrentFolder();
    }
    else
    {
        InsertSubitems(TVI_ROOT, m_pidlRoot);
    }

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

    // Register browser connection endpoint
    CComPtr<IWebBrowser2> browserService;
    hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlAdvise(browserService, dynamic_cast<IDispatch*>(this), DIID_DWebBrowserEvents, &m_adviseCookie);
    if (FAILED_UNEXPECTEDLY(hr))
        return;
}

void CNSCBand::_UnInit()
{
    HRESULT hr;
    CComPtr <IWebBrowser2> browserService;

    TRACE("Cleaning up explorer band ...\n");

    hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &browserService));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    hr = AtlUnadvise(browserService, DIID_DWebBrowserEvents, m_adviseCookie);
    /* Remove all items of the treeview */
    RevokeDragDrop(m_hwndTreeView);
    TreeView_DeleteAllItems(m_hwndTreeView);
    m_pDesktop = NULL;
    m_hRoot = NULL;
    TRACE("Cleanup done !\n");
}

HRESULT CNSCBand::_CreateTreeView()
{
    m_hTreeViewImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 64, 0);
    ATLASSERT(m_hTreeViewImageList);
    if (!m_hTreeViewImageList)
        return E_FAIL;

    RefreshFlags(&m_dwTVStyle, &m_dwTVExStyle, &m_dwEnumFlags);
    HWND hwndTV = ::CreateWindowExW(m_dwTVExStyle, WC_TREEVIEWW, NULL, m_dwTVStyle, 0, 0, 0, 0,
                                    m_hWnd, (HMENU)(ULONG_PTR)IDW_TREEVIEW, instance, NULL);
    ATLASSERT(hwndTV);
    if (!hwndTV)
        return E_FAIL;

    m_hwndTreeView.Attach(hwndTV);
    TreeView_SetImageList(m_hwndTreeView, m_hTreeViewImageList, TVSIL_NORMAL);
    ::RegisterDragDrop(m_hwndTreeView, dynamic_cast<IDropTarget*>(this));

    return S_OK;
}

DWORD CNSCBand::_GetEnumFlags()
{
    return SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
}

BOOL
CNSCBand::IsTreeItemInEnum(
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
CNSCBand::TreeItemHasThisChild(
    _In_ HTREEITEM hItem,
    _In_ PCITEMID_CHILD pidlChild)
{
    for (hItem = TreeView_GetChild(m_hwndTreeView, hItem); hItem;
         hItem = TreeView_GetNextSibling(m_hwndTreeView, hItem))
    {
        NodeInfo* pNodeInfo = GetNodeInfo(hItem);
        if (ILIsEqual(pNodeInfo->relativePidl, pidlChild))
            return TRUE;
    }

    return FALSE;
}

HRESULT
CNSCBand::GetItemEnum(
    _Out_ CComPtr<IEnumIDList>& pEnum,
    _In_ HTREEITEM hItem,
    _Out_opt_ IShellFolder **ppFolder)
{
    CComPtr<IShellFolder> psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
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
        NodeInfo* pNodeInfo = GetNodeInfo(hItem);
        if (!pNodeInfo && hItem == TVI_ROOT && !_WantsRootItem())
            hr = psfDesktop->BindToObject(m_pidlRoot, NULL, IID_PPV_ARG(IShellFolder, ppFolder));
        else
            hr = psfDesktop->BindToObject(pNodeInfo->absolutePidl, NULL, IID_PPV_ARG(IShellFolder, ppFolder));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return (*ppFolder)->EnumObjects(NULL, _GetEnumFlags(), &pEnum);
}

BOOL CNSCBand::ItemHasAnyChild(_In_ HTREEITEM hItem)
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hr = GetItemEnum(pEnum, hItem);
    if (FAILED(hr))
        return FALSE;

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    hr = pEnum->Next(1, &pidlTemp, NULL);
    return SUCCEEDED(hr);
}

void CNSCBand::RefreshRecurse(_In_ HTREEITEM hTarget)
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hrEnum = GetItemEnum(pEnum, hTarget);

    // Delete zombie items
    HTREEITEM hItem, hNextItem;
    for (hItem = TreeView_GetChild(m_hwndTreeView, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(m_hwndTreeView, hItem);

        if (SUCCEEDED(hrEnum) && !IsTreeItemInEnum(hItem, pEnum))
            TreeView_DeleteItem(m_hwndTreeView, hItem);
    }

    pEnum = NULL;
    hrEnum = GetItemEnum(pEnum, hTarget);

    NodeInfo* pNodeInfo = ((hTarget == TVI_ROOT) ? NULL : GetNodeInfo(hTarget));

    // Insert new items and update items
    if (SUCCEEDED(hrEnum))
    {
        CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
        while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
        {
            if (!TreeItemHasThisChild(hTarget, pidlTemp))
            {
                if (pNodeInfo)
                {
                    CComHeapPtr<ITEMIDLIST> pidlAbsolute(ILCombine(pNodeInfo->absolutePidl, pidlTemp));
                    InsertItem(hTarget, pidlAbsolute, pidlTemp, TRUE);
                }
                else
                {
                    CComHeapPtr<ITEMIDLIST> pidlAbsolute(ILCombine(m_pidlRoot, pidlTemp));
                    InsertItem(hTarget, pidlAbsolute, pidlTemp, TRUE);
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
        item.cChildren = ItemHasAnyChild(hItem);
        TreeView_SetItem(m_hwndTreeView, &item);

        if (TreeView_GetItemState(m_hwndTreeView, hItem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
            RefreshRecurse(hItem);
    }
}

void CNSCBand::Refresh()
{
    m_hwndTreeView.SendMessage(WM_SETREDRAW, FALSE, 0);
    RefreshRecurse(_WantsRootItem() ? m_hRoot : TVI_ROOT);
    m_hwndTreeView.SendMessage(WM_SETREDRAW, TRUE, 0);
    m_hwndTreeView.InvalidateRect(NULL, TRUE);
}

LRESULT CNSCBand::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    KillTimer(wParam);

    switch (wParam)
    {
        case TIMER_ID_REFRESH:
            Refresh();
            break;
        case TIMER_ID_SELECT_ITEM:
            _OnSelectItem();
            break;
    }

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
CNSCBand::InsertItem(
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

    NodeInfo* pChildInfo = new NodeInfo;
    if (!pChildInfo)
    {
        ERR("Failed to allocate NodeInfo\n");
        return NULL;
    }

    // Store our node info
    pChildInfo->absolutePidl = ILClone(pElt);
    pChildInfo->relativePidl = ILClone(pEltRelative);
    pChildInfo->expanded = FALSE;

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
    {
        TVSORTCB sortCallback;
        sortCallback.hParent = hParent;
        sortCallback.lpfnCompare = CompareTreeItems;
        sortCallback.lParam = (LPARAM)(PVOID)m_pDesktop;
        TreeView_SortChildrenCB(m_hwndTreeView, &sortCallback, 0);
    }

    return htiCreated;
}

/* This is the slow version of the above method */
HTREEITEM
CNSCBand::InsertItem(
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

BOOL CNSCBand::InsertSubitems(HTREEITEM hItem, LPCITEMIDLIST entry)
{
    ULONG fetched = 1, uItemCount = 0;

    CComPtr<IEnumIDList> pEnum;
    CComPtr<IShellFolder> pFolder;
    HRESULT hr = GetItemEnum(pEnum, hItem, &pFolder);

    // avoid broken IShellFolder implementations that return null pointer with success
    if (!SUCCEEDED(hr) || !pEnum)
    {
        ERR("Can't enum the folder! %p\n", hItem);
        return FALSE;
    }

    /* Don't redraw while we add stuff into the tree */
    m_hwndTreeView.SendMessage(WM_SETREDRAW, FALSE, 0);

    LPITEMIDLIST pidlSub;
    while (SUCCEEDED(pEnum->Next(1, &pidlSub, &fetched)) && pidlSub && fetched)
    {
        LPITEMIDLIST pidlSubComplete;
        pidlSubComplete = ILCombine(entry, pidlSub);

        if (InsertItem(hItem, pFolder, pidlSubComplete, pidlSub, FALSE))
        {
            uItemCount++;
        }

        ILFree(pidlSubComplete);
        ILFree(pidlSub);
    }

    /* Let's do sorting */
    TVSORTCB sortCallback;
    sortCallback.hParent = hItem;
    sortCallback.lpfnCompare = CompareTreeItems;
    sortCallback.lParam = (LPARAM)(PVOID)m_pDesktop;
    TreeView_SortChildrenCB(m_hwndTreeView, &sortCallback, 0);

    /* Now we can redraw */
    m_hwndTreeView.SendMessage(WM_SETREDRAW, TRUE, 0);

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
BOOL CNSCBand::NavigateToPIDL(LPCITEMIDLIST dest, HTREEITEM *item, BOOL bExpand, BOOL bInsert,
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

    current = TreeView_GetFirstVisible(m_hwndTreeView);
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
                TreeView_SelectItem(m_hwndTreeView, current);
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
            TreeView_SetItem(m_hwndTreeView, &tvItem);

            // If we can expand and the node isn't expanded yet, do it
            if (bExpand)
            {
                if (!nodeData->expanded)
                {
                    InsertSubitems(current, nodeData->absolutePidl);
                    nodeData->expanded = TRUE;
                }
                TreeView_Expand(m_hwndTreeView, current, TVE_EXPAND);
            }

            // Try to get a child
            tmp = TreeView_GetChild(m_hwndTreeView, current);
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
        tmp = TreeView_GetNextSibling(m_hwndTreeView, current);
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

BOOL CNSCBand::NavigateToCurrentFolder()
{
    CComHeapPtr<ITEMIDLIST> explorerPidl;
    HRESULT hr = GetCurrentLocation(&explorerPidl);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("Unable to get browser PIDL !\n");
        return FALSE;
    }

    ++m_mtxBlockNavigate;
    /* find PIDL into our explorer */
    HTREEITEM hItem;
    BOOL result = NavigateToPIDL(explorerPidl, &hItem, TRUE, FALSE, TRUE);
    --m_mtxBlockNavigate;

    return result;
}

// *** Tree item sorting callback ***
INT CALLBACK CNSCBand::CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3)
{
    NodeInfo *info1 = (NodeInfo*)p1;
    NodeInfo *info2 = (NodeInfo*)p2;
    IShellFolder *pDesktop = (IShellFolder *)p3;

    HRESULT hr = pDesktop->CompareIDs(0, info1->absolutePidl, info2->absolutePidl);
    if (FAILED(hr))
        return 0;

    return (SHORT)HRESULT_CODE(hr);
}

// *** message handlers ***

LRESULT CNSCBand::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INITCOMMONCONTROLSEX iccx = { sizeof(iccx), ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES };
    if (!::InitCommonControlsEx(&iccx))
        return -1;
    if (FAILED(_CreateToolbar()) || FAILED(_CreateTreeView()))
        return -1;
    return 0;
}

LRESULT CNSCBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hwndTreeView.DestroyWindow();
    m_hwndToolbar.DestroyWindow();
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
    GetCurrentLocation(&pidlCurrent);

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

    return SHELL_CreateShortcut(szPath, pidlCurrent, NULL);
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
            SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_IDLIST };
            sei.hwnd = m_hWnd;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpIDList = m_pidlRoot;
            ::ShellExecuteExW(&sei);
            break;
        }
    }
    return 0;
}

BOOL CNSCBand::OnTreeItemExpanding(LPNMTREEVIEW pnmtv)
{
    NodeInfo *pNodeInfo;

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
        pNodeInfo = GetNodeInfo(pnmtv->itemNew.hItem);
        // We have it, let's try
        if (pNodeInfo && !pNodeInfo->expanded)
        {
            if (InsertSubitems(pnmtv->itemNew.hItem, pNodeInfo->absolutePidl))
            {
                pNodeInfo->expanded = TRUE;
            }
            else
            {
                // remove subitem "+" since we failed to add subitems
                TV_ITEM tvItem;

                tvItem.mask = TVIF_CHILDREN;
                tvItem.hItem = pnmtv->itemNew.hItem;
                tvItem.cChildren = 0;

                TreeView_SetItem(m_hwndTreeView, &tvItem);
            }
        }
    }
    return FALSE;
}

BOOL CNSCBand::OnTreeItemDeleted(LPNMTREEVIEW pnmtv)
{
    // Navigate to parent when deleting selected item
    HTREEITEM hItem = pnmtv->itemOld.hItem;
    HTREEITEM hParent = TreeView_GetParent(m_hwndTreeView, hItem);
    if (hParent && TreeView_GetSelection(m_hwndTreeView) == hItem)
        TreeView_SelectItem(m_hwndTreeView, hParent);

    /* Destroy memory associated to our node */
    NodeInfo* pNode = GetNodeInfo(hItem);
    if (!pNode)
        return FALSE;

    ILFree(pNode->relativePidl);
    ILFree(pNode->absolutePidl);
    delete pNode;

    return TRUE;
}

void CNSCBand::_OnSelectItem()
{
    KillTimer(TIMER_ID_SELECT_ITEM);
    HTREEITEM hItem = TreeView_GetSelection(m_hwndTreeView);
    NodeInfo* pNodeInfo = GetNodeInfo(hItem);
    if (pNodeInfo)
    {
        KillTimer(TIMER_ID_SELECT_ITEM);
        OnSelectionChanged(pNodeInfo->absolutePidl);
    }
}

void CNSCBand::OnSelectionChanged(LPNMTREEVIEW pnmtv)
{
    HTREEITEM hItem = pnmtv->itemNew.hItem;
    NodeInfo* pNodeInfo = GetNodeInfo(hItem);
    if (pNodeInfo)
    {
        KillTimer(TIMER_ID_SELECT_ITEM);
        OnSelectionChanged(pNodeInfo->absolutePidl);
    }
}

void CNSCBand::OnTreeItemDragging(LPNMTREEVIEW pnmtv, BOOL isRightClick)
{
    KillTimer(TIMER_ID_SELECT_ITEM);

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

LRESULT CNSCBand::OnBeginLabelEdit(LPNMTVDISPINFO dispInfo)
{
    // TODO: put this in a function ? (mostly copypasta from CDefView)
    DWORD dwAttr = SFGAO_CANRENAME;
    CComPtr<IShellFolder> pParent;
    LPCITEMIDLIST pChild;
    HRESULT hr;

    NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
    if (!info)
        return FALSE;

    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pChild);
    if (!SUCCEEDED(hr) || !pParent.p)
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

HRESULT CNSCBand::UpdateBrowser(LPCITEMIDLIST pidlGoto)
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

LRESULT CNSCBand::OnEndLabelEdit(LPNMTVDISPINFO dispInfo)
{
    NodeInfo *info = GetNodeInfo(dispInfo->item.hItem);
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

    LPITEMIDLIST pidlNew;
    CComPtr<IShellFolder> pParent;
    LPCITEMIDLIST pidlChild;
    BOOL RenamedCurrent = IsCurrentLocation(info->absolutePidl) == S_OK;

    hr = SHBindToParent(info->absolutePidl, IID_PPV_ARG(IShellFolder, &pParent), &pidlChild);
    if (!SUCCEEDED(hr) || !pParent.p)
        return FALSE;

    hr = pParent->SetNameOf(m_hWnd, pidlChild, dispInfo->item.pszText, SHGDN_INFOLDER, &pidlNew);
    if (SUCCEEDED(hr) && pidlNew)
    {
        CComPtr<IPersistFolder2> pPersist;
        LPITEMIDLIST pidlParent, pidlNewAbs;

        hr = pParent->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pPersist));
        if (!SUCCEEDED(hr))
            return FALSE;

        hr = pPersist->GetCurFolder(&pidlParent);
        if (!SUCCEEDED(hr))
            return FALSE;

        pidlNewAbs = ILCombine(pidlParent, pidlNew);

        if (RenamedCurrent)
        {
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
            {
                KillTimer(TIMER_ID_SELECT_ITEM);
                OnSelectionChanged((LPNMTREEVIEW)lParam);
            }
            break;
        case TVN_DELETEITEM:
            OnTreeItemDeleted((LPNMTREEVIEW)lParam);
            break;
        case NM_CLICK:
        case NM_DBLCLK:
            if (pnmhdr->hwndFrom == m_hwndTreeView)
            {
                TV_HITTESTINFO HitTest;
                ::GetCursorPos(&HitTest.pt);
                TreeView_HitTest(m_hwndTreeView, &HitTest);
                if (HitTest.flags & (VHT_ABOVE | TVHT_BELOW | TVHT_NOWHERE))
                    break;
                KillTimer(TIMER_ID_SELECT_ITEM);
                SetTimer(TIMER_ID_SELECT_ITEM, ::GetDoubleClickTime(), NULL);
            }
            break;
        case NM_RCLICK:
            OnContextMenu(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos(), bHandled);
            return 1;
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

// *** ATL event handlers ***
LRESULT CNSCBand::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
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
    item = TreeView_GetSelection(m_hwndTreeView);
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
        if (TreeView_GetItemRect(m_hwndTreeView, item, &r, TRUE))
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
            startedRename = TreeView_EditLabel(m_hwndTreeView, item) != NULL;
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
        TreeView_SelectItem(m_hwndTreeView, m_oldSelected);
        --m_mtxBlockNavigate;
    }
    return TRUE;
}

LRESULT CNSCBand::ContextMenuHack(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
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
        m_oldSelected = TreeView_GetSelection(m_hwndTreeView);

        // Move to the item selected by the treeview (don't change right pane)
        TreeView_HitTest(m_hwndTreeView, &info);
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hwndTreeView, info.hItem);
        --m_mtxBlockNavigate;
    }
    return FALSE; /* let the wndproc process the message */
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
    if (hLock == NULL)
    {
        ERR("hLock == NULL\n");
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
    if (!SUCCEEDED(hr))
    {
        ERR("Could not get parent's window! 0x%08lX\n", hr);
        return E_INVALIDARG;
    }

    m_pSite = pUnkSite;

    if (m_hWnd)
    {
        SetParent(hwndParent); // Change its parent
    }
    else
    {
        this->Create(hwndParent, NULL, NULL, WS_CHILD | WS_VISIBLE, 0, (UINT)0, NULL);
    }

    _Init();

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

// *** IServiceProvider ***

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
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_SH_FavBand;
    return S_OK;
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
    // TODO: calculate max size
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
    return (hWnd == m_hWnd) ? S_OK : S_FALSE;
}

// *** IBandNavigate ***

STDMETHODIMP CNSCBand::Select(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

// *** INamespaceProxy ***

/// Returns the ITEMIDLIST that should be navigated when an item is invoked.
STDMETHODIMP CNSCBand::GetNavigateTarget(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _Out_ PIDLIST_ABSOLUTE *ppidlTarget,
    _Out_ ULONG *pulAttrib)
{
    *pulAttrib = 0;
    WCHAR szPath[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, szPath))
    {
        if (lstrcmpiW(PathFindExtensionW(szPath), L".lnk") == 0) // shortcut file?
        {
            WCHAR szTarget[MAX_PATH];
            HRESULT hr = SHELL_GetPathOfShortcut(m_hWnd, szPath, szTarget);
            if (SUCCEEDED(hr))
            {
                lstrcpynW(szPath, szTarget, _countof(szPath));
            }
            *pulAttrib |= SFGAO_LINK;
        }
        if (PathIsDirectoryW(szPath))
        {
            *pulAttrib |= SFGAO_FOLDER;
        }
        *ppidlTarget = ILCreateFromPathW(szPath);
        return S_OK;
    }
    return E_FAIL;
}

/// Handles a user action on an item.
STDMETHODIMP CNSCBand::Invoke(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Called when the user has selected an item.
STDMETHODIMP CNSCBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

/// Returns flags used to update the tree control.
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

// *** IDispatch methods ***
STDMETHODIMP CNSCBand::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CNSCBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    switch (dispIdMember)
    {
        case DISPID_DOWNLOADCOMPLETE:
        case DISPID_NAVIGATECOMPLETE2:
        {
            TRACE("dispId %d received\n", dispIdMember);
            if (_WantsRootItem())
                NavigateToCurrentFolder();
            return S_OK;
        }
    }
    TRACE("Unknown dispid requested: %08x\n", dispIdMember);
    return E_INVALIDARG;
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
    TreeView_HitTest(m_hwndTreeView, &info);

    if (info.hItem)
    {
        ++m_mtxBlockNavigate;
        TreeView_SelectItem(m_hwndTreeView, info.hItem);
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
            if (_ILIsDesktop(nodeInfo->absolutePidl))
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
