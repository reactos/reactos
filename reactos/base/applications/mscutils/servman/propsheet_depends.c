/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_depends.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

HTREEITEM
AddItemToTreeView(HWND hTreeView,
                  HTREEITEM hParent,
                  LPWSTR lpDisplayName,
                  LPWSTR lpServiceName,
                  ULONG ServiceType,
                  BOOL bHasChildren)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;
    LPWSTR lpName;
    DWORD dwSize;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_CHILDREN;
    tvi.pszText = lpDisplayName;
    tvi.cchTextMax = wcslen(lpDisplayName);
    tvi.cChildren = bHasChildren;

    /* Select the image for this service */
    switch (ServiceType)
    {
        case SERVICE_WIN32_OWN_PROCESS:
        case SERVICE_WIN32_SHARE_PROCESS:
            tvi.iImage = IMAGE_SERVICE;
            tvi.iSelectedImage = IMAGE_SERVICE;
            break;

        case SERVICE_KERNEL_DRIVER:
        case SERVICE_FILE_SYSTEM_DRIVER:
            tvi.iImage = IMAGE_DRIVER;
            tvi.iSelectedImage = IMAGE_DRIVER;
            break;

        default:
            tvi.iImage = IMAGE_UNKNOWN;
            tvi.iSelectedImage = IMAGE_UNKNOWN;
            break;
    }

    if (lpServiceName)
    {
        dwSize = wcslen(lpServiceName) + 1;
        /* Attach the service name */
        lpName = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                   0,
                                   dwSize * sizeof(WCHAR));
        if (lpName)
        {
            StringCchCopyW(lpName, dwSize, lpServiceName);
            tvi.lParam = (LPARAM)lpName;
        }
    }

    tvins.item = tvi;
    tvins.hParent = hParent;

    return TreeView_InsertItem(hTreeView, &tvins);
}

static LPARAM
TreeView_GetItemParam(HWND hTreeView,
                      HTREEITEM hItem)
{
    LPARAM lParam = 0;
    TVITEMW tv = {0};

    tv.mask = TVIF_PARAM | TVIF_HANDLE;
    tv.hItem = hItem;

    if (TreeView_GetItem(hTreeView, &tv))
    {
        lParam = tv.lParam;
    }

    return lParam;
}

static VOID
DestroyItem(HWND hTreeView,
            HTREEITEM hItem)
{
    HTREEITEM hChildItem;
    LPWSTR lpServiceName;

    /* Does this item have any children */
    hChildItem = TreeView_GetChild(hTreeView, hItem);
    if (hChildItem)
    {
        /* It does, recurse to that one */
        DestroyItem(hTreeView, hChildItem);
    }

    /* Get the string and free it */
    lpServiceName = (LPWSTR)TreeView_GetItemParam(hTreeView, hItem);
    if (lpServiceName)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpServiceName);
    }
}

static VOID
DestroyTreeView(HWND hTreeView)
{
    HTREEITEM hItem;

    /* Get the first item in the top level */
    hItem = TreeView_GetFirstVisible(hTreeView);
    if (hItem)
    {
        /* Kill it and all children */
        DestroyItem(hTreeView, hItem);

        /* Kill all remaining top level items */
        while (hItem)
        {
            /* Are there any more items at the top level */
            hItem = TreeView_GetNextSibling(hTreeView, hItem);
            if (hItem)
            {
                /*  Kill it and all children */
                DestroyItem(hTreeView, hItem);
            }
        }
    }
}

/*
static BOOL
TreeView_GetItemText(HWND hTreeView,
                     HTREEITEM hItem,
                     LPTSTR lpBuffer,
                     DWORD cbBuffer)
{
    TVITEM tv = {0};

    tv.mask = TVIF_TEXT | TVIF_HANDLE;
    tv.hItem = hItem;
    tv.pszText = lpBuffer;
    tv.cchTextMax = (int)cbBuffer;

    return TreeView_GetItem(hTreeView, &tv);
}
*/

static VOID
InitDependPage(PSERVICEPROPSHEET pDlgInfo)
{
    /* Initialize the image list */
    pDlgInfo->hDependsImageList = InitImageList(IDI_NODEPENDS,
                                                IDI_DRIVER,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CXSMICON),
                                                IMAGE_ICON);

    /* Set the first tree view */
    TV1_Initialize(pDlgInfo, pDlgInfo->pService->lpServiceName);

    /* Set the second tree view */
    TV2_Initialize(pDlgInfo, pDlgInfo->pService->lpServiceName);
}

/*
 * Dependancies Property dialog callback.
 * Controls messages to the Dependancies dialog
 */
INT_PTR CALLBACK
DependenciesPageProc(HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    PSERVICEPROPSHEET pDlgInfo;

    /* Get the window context */
    pDlgInfo = (PSERVICEPROPSHEET)GetWindowLongPtr(hwndDlg,
                                                   GWLP_USERDATA);
    if (pDlgInfo == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDlgInfo = (PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam);
            if (pDlgInfo != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)pDlgInfo);

                pDlgInfo->hDependsWnd = hwndDlg;

                InitDependPage(pDlgInfo);
            }
        }
        break;

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case TVN_ITEMEXPANDING:
                {
                    LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lParam;

                    if (lpnmtv->action == TVE_EXPAND)
                    {
                        if (lpnmtv->hdr.idFrom == IDC_DEPEND_TREE1)
                        {
                            /* Has this node been expanded before */
                            if (!TreeView_GetChild(pDlgInfo->hDependsTreeView1, lpnmtv->itemNew.hItem))
                            {
                                /* It's not, add the children */
                                TV1_AddDependantsToTree(pDlgInfo, lpnmtv->itemNew.hItem, (LPWSTR)lpnmtv->itemNew.lParam);
                            }
                        }
                        else if (lpnmtv->hdr.idFrom == IDC_DEPEND_TREE2)
                        {
                            /* Has this node been expanded before */
                            if (!TreeView_GetChild(pDlgInfo->hDependsTreeView2, lpnmtv->itemNew.hItem))
                            {
                                /* It's not, add the children */
                                TV2_AddDependantsToTree(pDlgInfo, lpnmtv->itemNew.hItem, (LPWSTR)lpnmtv->itemNew.lParam);
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {

            }
            break;

        case WM_DESTROY:
            DestroyTreeView(pDlgInfo->hDependsTreeView1);
            DestroyTreeView(pDlgInfo->hDependsTreeView2);

            if (pDlgInfo->hDependsImageList)
                ImageList_Destroy(pDlgInfo->hDependsImageList);
    }

    return FALSE;
}
