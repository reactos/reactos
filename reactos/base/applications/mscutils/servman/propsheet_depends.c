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
                  HTREEITEM hRoot,
                  LPTSTR lpDisplayName,
                  LPTSTR lpServiceName,
                  ULONG serviceType,
                  BOOL bHasChildren)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_CHILDREN;
    tvi.pszText = lpDisplayName;
    tvi.cchTextMax = _tcslen(lpDisplayName);
    tvi.cChildren = bHasChildren; //I_CHILDRENCALLBACK;

    if (serviceType == SERVICE_WIN32_OWN_PROCESS ||
        serviceType == SERVICE_WIN32_SHARE_PROCESS)
    {
        tvi.iImage = 1;
        tvi.iSelectedImage = 1;
    }
    else if (serviceType == SERVICE_KERNEL_DRIVER ||
             serviceType == SERVICE_FILE_SYSTEM_DRIVER)
    {
        tvi.iImage = 2;
        tvi.iSelectedImage = 2;
    }
    else
    {
        tvi.iImage = 0;
        tvi.iSelectedImage = 0;
    }

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTreeView, &tvins);
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

static BOOL
InitDependPage(PSERVICEPROPSHEET pDlgInfo)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    BOOL bRet = FALSE;

    /* Initialize the image list */
    pDlgInfo->hDependsImageList = InitImageList(IDI_NODEPENDS,
                                                IDI_DRIVER,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CXSMICON),
                                                IMAGE_ICON);

    /* Set the first items in each tree view */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               pDlgInfo->pService->lpServiceName,
                               SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_CONFIG);
        if (hService)
        {
            /* Set the first tree view */
            TV1_Initialize(pDlgInfo, hService);

            /* Set the second tree view */
            TV2_Initialize(pDlgInfo, hService);

            bRet = TRUE;

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }

    return bRet;
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
            if (pDlgInfo->hDependsImageList)
                ImageList_Destroy(pDlgInfo->hDependsImageList);
    }

    return FALSE;
}
