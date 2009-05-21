/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_depends.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


static HTREEITEM
AddItemToTreeView(HWND hTreeView,
                  HTREEITEM hRoot,
                  LPTSTR lpLabel,
                  ULONG serviceType)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = lstrlen(lpLabel);

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

static VOID
AddServiceDependency(PSERVICEPROPSHEET dlgInfo,
                     HWND hTreeView,
                     SC_HANDLE hSCManager,
                     LPTSTR lpServiceName,
                     HTREEITEM hParent,
                     HWND hwndDlg)
{
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    SC_HANDLE hService;
    HTREEITEM hChild;
    LPTSTR lpStr;
    LPTSTR lpNoDepends;

    hService = OpenService(hSCManager,
                           lpServiceName,
                           SERVICE_QUERY_CONFIG);
    if (hService)
    {

        lpStr = GetDependentServices(hService);
        if (lpStr)
        {
            while (*lpStr)
            {
                hChild = AddItemToTreeView(hTreeView,
                                           hParent,
                                           lpServiceConfig->lpDisplayName,
                                           lpServiceConfig->dwServiceType);


                AddServiceDependency(dlgInfo,
                                     hTreeView,
                                     hSCManager,
                                     lpStr,
                                     hChild,
                                     hwndDlg);

                while (*lpStr++)
                    ;
            }
        }
        else
        {
            if (TreeView_GetCount(hTreeView) == 0)
            {
                if (AllocAndLoadString(&lpNoDepends, hInstance, IDS_NO_DEPENDS))
                {
                    lpStr = lpNoDepends;
                }

                AddItemToTreeView(hTreeView,
                                  hParent,
                                  lpStr,
                                  0);

                HeapFree(ProcessHeap,
                         0,
                         lpNoDepends);

                EnableWindow(hTreeView, FALSE);
            }
        }


        HeapFree(ProcessHeap,
                 0,
                 lpStr);

        CloseServiceHandle(hService);
    }

}

static VOID
AddServiceDependent(HWND hTreeView,
                    HTREEITEM hParent,
                    SC_HANDLE hSCManager,
                    LPTSTR lpServiceName,
                    LPTSTR lpDisplayName,
                    DWORD dwServiceType)
{
    LPENUM_SERVICE_STATUS lpServiceStatus;
    SC_HANDLE hChildService;
    HTREEITEM hChildNode;
    DWORD count;
    INT i;


    hChildNode = AddItemToTreeView(hTreeView,
                                   hParent,
                                   lpDisplayName,
                                   dwServiceType);

    hChildService = OpenService(hSCManager,
                                lpServiceName,
                                SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
    if (hChildService)
    {
        lpServiceStatus = GetServiceDependents(hChildService, &count);
        if (lpServiceStatus)
        {
            for (i = 0; i < count; i++)
            {
                AddServiceDependent(hTreeView,
                                    hChildNode,
                                    hSCManager,
                                    lpServiceStatus[i].lpServiceName,
                                    lpServiceStatus[i].lpDisplayName,
                                    lpServiceStatus[i].ServiceStatus.dwServiceType);
            }

            HeapFree(ProcessHeap,
                     0,
                     lpServiceStatus);
        }

        CloseServiceHandle(hChildService);
    }
}

static VOID
SetServiceDependents(HWND hTreeView,
                     SC_HANDLE hSCManager,
                     SC_HANDLE hService)
{
    LPENUM_SERVICE_STATUS lpServiceStatus;
    LPTSTR lpNoDepends;
    DWORD count, i;

    lpServiceStatus = GetServiceDependents(hService, &count);
    if (lpServiceStatus)
    {
        for (i = 0; i < count; i++)
        {
            AddServiceDependent(hTreeView,
                                NULL,
                                hSCManager,
                                lpServiceStatus[i].lpServiceName,
                                lpServiceStatus[i].lpDisplayName,
                                lpServiceStatus[i].ServiceStatus.dwServiceType);
        }
    }
    else
    {
            AllocAndLoadString(&lpNoDepends, hInstance, IDS_NO_DEPENDS);

            AddItemToTreeView(hTreeView,
                              NULL,
                              lpNoDepends,
                              0);

            HeapFree(ProcessHeap,
                     0,
                     lpNoDepends);

            EnableWindow(hTreeView, FALSE);
    }
}

static VOID
SetDependentServices(SC_HANDLE hService)
{
}

static VOID
InitDependPage(PSERVICEPROPSHEET dlgInfo,
               HWND hwndDlg)
{
    HWND hTreeView1, hTreeView2;
    SC_HANDLE hSCManager;
    SC_HANDLE hService;

    dlgInfo->hDependsImageList = InitImageList(IDI_NODEPENDS,
                                               IDI_DRIVER,
                                               GetSystemMetrics(SM_CXSMICON),
                                               GetSystemMetrics(SM_CXSMICON),
                                               IMAGE_ICON);


    hTreeView1 = GetDlgItem(hwndDlg, IDC_DEPEND_TREE1);
    if (!hTreeView1)
        return;

    (void)TreeView_SetImageList(hTreeView1,
                                dlgInfo->hDependsImageList,
                                TVSIL_NORMAL);

    hTreeView2 = GetDlgItem(hwndDlg, IDC_DEPEND_TREE2);
    if (!hTreeView2)
        return;

    (void)TreeView_SetImageList(hTreeView2,
                                dlgInfo->hDependsImageList,
                                TVSIL_NORMAL);

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               dlgInfo->pService->lpServiceName,
                               SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_CONFIG);
        if (hService)
        {
            /* Set the first tree view */
            SetServiceDependents(hTreeView1,
                                 hSCManager,
                                 hService);

            /* Set the second tree view */
            SetDependentServices(hService);

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }

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
    PSERVICEPROPSHEET dlgInfo;

    /* Get the window context */
    dlgInfo = (PSERVICEPROPSHEET)GetWindowLongPtr(hwndDlg,
                                                  GWLP_USERDATA);
    if (dlgInfo == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            dlgInfo = (PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam);
            if (dlgInfo != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)dlgInfo);

                InitDependPage(dlgInfo, hwndDlg);
            }
        }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {

            }
            break;
    }

    return FALSE;
}
