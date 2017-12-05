/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/dependencies_tv1.c
 * PURPOSE:     Helper functions for service dependents
 * COPYRIGHT:   Copyright 2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

LPWSTR
TV1_GetDependants(SC_HANDLE hService)
{
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    LPWSTR lpStr = NULL;
    DWORD bytesNeeded;
    DWORD bytes;

    /* Get the info for this service */
    if (!QueryServiceConfigW(hService,
                             NULL,
                             0,
                             &bytesNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        lpServiceConfig = HeapAlloc(ProcessHeap,
                                    0,
                                    bytesNeeded);
        if (lpServiceConfig)
        {
            if (QueryServiceConfigW(hService,
                                    lpServiceConfig,
                                    bytesNeeded,
                                    &bytesNeeded))
            {
                /* Does this service have any dependencies? */
                if (lpServiceConfig->lpDependencies &&
                    *lpServiceConfig->lpDependencies != '\0')
                {
                    lpStr = lpServiceConfig->lpDependencies;
                    bytes = 0;

                    /* Work out how many bytes we need to hold the list */
                    for (;;)
                    {
                        bytes++;

                        if (!*lpStr && !*(lpStr + 1))
                        {
                            bytes++;
                            break;
                        }

                        lpStr++;
                    }

                    /* Allocate and copy the list */
                    bytes *= sizeof(WCHAR);
                    lpStr = HeapAlloc(ProcessHeap,
                                      0,
                                      bytes);
                    if (lpStr)
                    {
                        CopyMemory(lpStr,
                                   lpServiceConfig->lpDependencies,
                                   bytes);
                    }
                }
            }

            HeapFree(GetProcessHeap(),
                     0,
                     lpServiceConfig);
        }
    }

    return lpStr;
}

VOID
TV1_AddDependantsToTree(PDEPENDDATA pDependData,
                        HTREEITEM hParent,
                        LPWSTR lpServiceName)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    LPWSTR lpDependants;
    LPWSTR lpStr;
    LPWSTR lpNoDepends;
    BOOL bHasChildren;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenServiceW(hSCManager,
                                lpServiceName,
                                SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_CONFIG);
        if (hService)
        {
            /* Get a list of service dependents */
            lpDependants = TV1_GetDependants(hService);
            if (lpDependants)
            {
                lpStr = lpDependants;

                /* Make sure this isn't the end of the list */
                while (*lpStr)
                {
                    /* Get the info for this service */
                    lpServiceConfig = GetServiceConfig(lpStr);
                    if (lpServiceConfig)
                    {
                        /* Does this item need a +/- box? */
                        if (lpServiceConfig->lpDependencies &&
                            *lpServiceConfig->lpDependencies != '\0')
                        {
                            bHasChildren = TRUE;
                        }
                        else
                        {
                            bHasChildren = FALSE;
                        }

                        /* Add it */
                        AddItemToTreeView(pDependData->hDependsTreeView1,
                                          hParent,
                                          lpServiceConfig->lpDisplayName,
                                          lpStr,
                                          lpServiceConfig->dwServiceType,
                                          bHasChildren);

                        HeapFree(GetProcessHeap(),
                                 0,
                                 lpServiceConfig);
                    }

                    /* Move to the end of the string */
                    while (*lpStr++)
                        ;
                }

                HeapFree(GetProcessHeap(),
                         0,
                         lpDependants);
            }
            else
            {
                /* If there is no parent, set the tree to 'no dependencies' */
                if (!hParent)
                {
                    /* Load the 'No dependencies' string */
                    AllocAndLoadString(&lpNoDepends, hInstance, IDS_NO_DEPENDS);

                    AddItemToTreeView(pDependData->hDependsTreeView1,
                                      NULL,
                                      lpNoDepends,
                                      NULL,
                                      0,
                                      FALSE);

                    LocalFree(lpNoDepends);

                    /* Disable the window */
                    EnableWindow(pDependData->hDependsTreeView1, FALSE);
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }
}

BOOL
TV1_Initialize(PDEPENDDATA pDependData,
               LPWSTR lpServiceName)
{
    BOOL bRet = FALSE;

    /* Associate the imagelist with TV1 */
    pDependData->hDependsTreeView1 = GetDlgItem(pDependData->hDependsWnd, IDC_DEPEND_TREE1);
    if (!pDependData->hDependsTreeView1)
    {
        ImageList_Destroy(pDependData->hDependsImageList);
        pDependData->hDependsImageList = NULL;
        return FALSE;
    }
    (void)TreeView_SetImageList(pDependData->hDependsTreeView1,
                                pDependData->hDependsImageList,
                                TVSIL_NORMAL);

    /* Set the first items in the control */
    TV1_AddDependantsToTree(pDependData, NULL, lpServiceName);

    return bRet;
}
