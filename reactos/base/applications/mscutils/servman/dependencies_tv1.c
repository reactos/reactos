/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/tv1_dependencies.c
 * PURPOSE:     Helper functions for service dependents
 * COPYRIGHT:   Copyright 2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


LPTSTR
TV1_GetDependants(PSERVICEPROPSHEET pDlgInfo,
                  SC_HANDLE hService)
{
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    LPTSTR lpStr = NULL;
    DWORD bytesNeeded;
    DWORD bytes;

    /* Get the info for this service */
    if (!QueryServiceConfig(hService,
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
            if (QueryServiceConfig(hService,
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
                    while (TRUE)
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
                    bytes *= sizeof(TCHAR);
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

static VOID
TV1_AddDependantsToTree(PSERVICEPROPSHEET pDlgInfo,
                        HTREEITEM hParent,
                        SC_HANDLE hService)
{
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    LPTSTR lpDependants;
    LPTSTR lpStr;
    LPTSTR lpNoDepends;
    BOOL bHasChildren;

    /* Get a list of service dependents */
    lpDependants = TV1_GetDependants(pDlgInfo, hService);
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
                bHasChildren = lpServiceConfig->lpDependencies ? TRUE : FALSE;

                /* Add it */
                AddItemToTreeView(pDlgInfo->hDependsTreeView1,
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

            AddItemToTreeView(pDlgInfo->hDependsTreeView1,
                              NULL,
                              lpNoDepends,
                              NULL,
                              0,
                              FALSE);

            HeapFree(ProcessHeap,
                     0,
                     lpNoDepends);

            /* Disable the window */
            EnableWindow(pDlgInfo->hDependsTreeView1, FALSE);
        }
    }
}


BOOL
TV1_Initialize(PSERVICEPROPSHEET pDlgInfo,
               SC_HANDLE hService)
{
    BOOL bRet = FALSE;

    /* Accociate the imagelist with TV1 */
    pDlgInfo->hDependsTreeView1 = GetDlgItem(pDlgInfo->hDependsWnd, IDC_DEPEND_TREE1);
    if (!pDlgInfo->hDependsTreeView1)
    {
        ImageList_Destroy(pDlgInfo->hDependsImageList);
        pDlgInfo->hDependsImageList = NULL;
        return FALSE;
    }
    (void)TreeView_SetImageList(pDlgInfo->hDependsTreeView1,
                                pDlgInfo->hDependsImageList,
                                TVSIL_NORMAL);

    /* Set the first items in the control */
    TV1_AddDependantsToTree(pDlgInfo, NULL, hService);

    return bRet;
}
