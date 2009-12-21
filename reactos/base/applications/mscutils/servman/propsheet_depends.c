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
                  LPTSTR lpDisplayName,
                  LPTSTR lpServiceName,
                  ULONG ServiceType,
                  BOOL bHasChildren)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_CHILDREN;
    tvi.pszText = lpDisplayName;
    tvi.cchTextMax = _tcslen(lpDisplayName);
    tvi.cChildren = bHasChildren; //I_CHILDRENCALLBACK;

    switch (ServiceType)
    {
        case SERVICE_WIN32_OWN_PROCESS:
        case SERVICE_WIN32_SHARE_PROCESS:
            tvi.iImage = 1;
            tvi.iSelectedImage = 1;
            break;

        case SERVICE_KERNEL_DRIVER:
        case SERVICE_FILE_SYSTEM_DRIVER:
            tvi.iImage = 2;
            tvi.iSelectedImage = 2;
            break;

        default:
            tvi.iImage = 0;
            tvi.iSelectedImage = 0;
            break;
    }

    /* Attach the service name */
    tvi.lParam = (LPARAM)(LPTSTR)HeapAlloc(GetProcessHeap(),
                                           0,
                                           (_tcslen(lpServiceName) + 1) * sizeof(TCHAR));
    if (tvi.lParam)
    {
        _tcscpy((LPTSTR)tvi.lParam, lpServiceName);
    }

    tvins.item = tvi;
    tvins.hParent = hParent;

    return TreeView_InsertItem(hTreeView, &tvins);
}

static VOID
DestroyTreeView(HWND hTreeView)
{
    //FIXME: traverse the nodes and free the strings
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


static LPARAM
TreeView_GetItemParam(HWND hTreeView,
                      HTREEITEM hItem)
{
    LPARAM lParam = 0;
    TVITEM tv = {0};

    tv.mask = TVIF_PARAM | TVIF_HANDLE;
    tv.hItem = hItem;

    if (TreeView_GetItem(hTreeView, &tv))
    {
        lParam = tv.lParam;
    }

    return lParam;
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
                            TV1_AddDependantsToTree(pDlgInfo, lpnmtv->itemNew.hItem, (LPTSTR)lpnmtv->itemNew.lParam);
                        }
                        else if (lpnmtv->hdr.idFrom == IDC_DEPEND_TREE2)
                        {
                            TV2_AddDependantsToTree(pDlgInfo, lpnmtv->itemNew.hItem, (LPTSTR)lpnmtv->itemNew.lParam);
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
