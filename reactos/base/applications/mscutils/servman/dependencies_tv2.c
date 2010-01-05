/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/tv2_dependencies.c
 * PURPOSE:     Helper functions for service dependents
 * COPYRIGHT:   Copyright 2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


BOOL
TV2_HasDependantServices(LPWSTR lpServiceName)
{
    HANDLE hSCManager;
    HANDLE hService;
    DWORD dwBytesNeeded, dwCount;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               lpServiceName,
                               SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
        if (hService)
        {
            /* Does this have any dependencies? */
            if (!EnumDependentServices(hService,
                                       SERVICE_STATE_ALL,
                                       NULL,
                                       0,
                                       &dwBytesNeeded,
                                       &dwCount))
            {
                 if (GetLastError() == ERROR_MORE_DATA)
                 {
                     /* It does, return TRUE */
                     bRet = TRUE;
                 }
            }
        }

        CloseServiceHandle(hSCManager);
    }

    return bRet;
}


static LPENUM_SERVICE_STATUS
TV2_GetDependants(SC_HANDLE hService,
                  LPDWORD lpdwCount)
{
    LPENUM_SERVICE_STATUS lpDependencies;
    DWORD dwBytesNeeded;
    DWORD dwCount;

    /* Does this have any dependencies? */
    if (EnumDependentServices(hService,
                              SERVICE_STATE_ALL,
                              NULL,
                              0,
                              &dwBytesNeeded,
                              &dwCount))
    {
        /* There are no dependent services */
         return NULL;
    }
    else
    {
        if (GetLastError() != ERROR_MORE_DATA)
            return NULL; /* Unexpected error */

        lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(GetProcessHeap(),
                                                          0,
                                                          dwBytesNeeded);
        if (lpDependencies)
        {
            /* Get the list of dependents */
            if (EnumDependentServices(hService,
                                      SERVICE_STATE_ALL,
                                      lpDependencies,
                                      dwBytesNeeded,
                                      &dwBytesNeeded,
                                      &dwCount))
            {
                /* Set the count */
                *lpdwCount = dwCount;
            }
            else
            {
                HeapFree(ProcessHeap,
                         0,
                         lpDependencies);

                lpDependencies = NULL;
            }
        }
    }

    return lpDependencies;
}


VOID
TV2_AddDependantsToTree(PSERVICEPROPSHEET pDlgInfo,
                        HTREEITEM hParent,
                        LPTSTR lpServiceName)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    LPENUM_SERVICE_STATUS lpServiceStatus;
    LPTSTR lpNoDepends;
    DWORD count, i;
    BOOL bHasChildren;

    /* Set the first items in each tree view */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               lpServiceName,
                               SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_CONFIG);
        if (hService)
        {
            /* Get a list of service dependents */
            lpServiceStatus = TV2_GetDependants(hService, &count);
            if (lpServiceStatus)
            {
                for (i = 0; i < count; i++)
                {
                    /* Does this item need a +/- box? */
                    bHasChildren = TV2_HasDependantServices(lpServiceStatus[i].lpServiceName);

                    /* Add it */
                    AddItemToTreeView(pDlgInfo->hDependsTreeView2,
                                      hParent,
                                      lpServiceStatus[i].lpDisplayName,
                                      lpServiceStatus[i].lpServiceName,
                                      lpServiceStatus[i].ServiceStatus.dwServiceType,
                                      bHasChildren);
                }
            }
            else
            {
                /* If there is no parent, set the tree to 'no dependencies' */
                if (!hParent)
                {
                    /* Load the 'No dependencies' string */
                    AllocAndLoadString(&lpNoDepends, hInstance, IDS_NO_DEPENDS);

                    AddItemToTreeView(pDlgInfo->hDependsTreeView2,
                                      NULL,
                                      lpNoDepends,
                                      NULL,
                                      0,
                                      FALSE);

                    HeapFree(ProcessHeap,
                             0,
                             lpNoDepends);

                    /* Disable the window */
                    EnableWindow(pDlgInfo->hDependsTreeView2, FALSE);
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }
}

BOOL
TV2_Initialize(PSERVICEPROPSHEET pDlgInfo,
               LPTSTR lpServiceName)
{
    BOOL bRet = FALSE;

    /* Accociate the imagelist with TV2 */
    pDlgInfo->hDependsTreeView2 = GetDlgItem(pDlgInfo->hDependsWnd, IDC_DEPEND_TREE2);
    if (!pDlgInfo->hDependsTreeView2)
    {
        ImageList_Destroy(pDlgInfo->hDependsImageList);
        pDlgInfo->hDependsImageList = NULL;
        return FALSE;
    }
    (void)TreeView_SetImageList(pDlgInfo->hDependsTreeView2,
                                pDlgInfo->hDependsImageList,
                                TVSIL_NORMAL);

    /* Set the first items in the control */
    TV2_AddDependantsToTree(pDlgInfo, NULL, lpServiceName);

    return bRet;
}
