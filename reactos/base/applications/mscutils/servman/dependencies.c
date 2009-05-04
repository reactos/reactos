/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/dependencies.c
 * PURPOSE:     Helper functions for service dependents
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

LPENUM_SERVICE_STATUS
GetDependentServices(SC_HANDLE hService,
                     LPDWORD lpdwCount)
{
    LPENUM_SERVICE_STATUS lpDependencies;
    DWORD dwBytesNeeded;
    DWORD dwCount;

    if (EnumDependentServices(hService,
                              SERVICE_ACTIVE,
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
            if (EnumDependentServices(hService,
                                       SERVICE_ACTIVE,
                                       lpDependencies,
                                       dwBytesNeeded,
                                       &dwBytesNeeded,
                                       &dwCount))
            {
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


BOOL
HasDependentServices(SC_HANDLE hService)
{
    DWORD dwBytesNeeded, dwCount;
    BOOL bRet = FALSE;

    if (hService)
    {
        if (!EnumDependentServices(hService,
                                   SERVICE_ACTIVE,
                                   NULL,
                                   0,
                                   &dwBytesNeeded,
                                   &dwCount))
        {
             if (GetLastError() == ERROR_MORE_DATA)
                 bRet = TRUE;
        }
    }

    return bRet;
}

static BOOL
DoInitDependsDialog(PSTOP_INFO pStopInfo,
                    HWND hDlg)
{
    LPENUM_SERVICE_STATUS lpDependencies;
    DWORD dwCount;
    LPTSTR lpPartialStr, lpStr;
    DWORD fullLen;
    HICON hIcon = NULL;
    BOOL bRet = FALSE;

    if (pStopInfo)
    {
        SetWindowLongPtr(hDlg,
                         GWLP_USERDATA,
                         (LONG_PTR)pStopInfo);

        hIcon = (HICON)LoadImage(hInstance,
                                 MAKEINTRESOURCE(IDI_SM_ICON),
                                 IMAGE_ICON,
                                 16,
                                 16,
                                 0);
        if (hIcon)
        {
            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
            DestroyIcon(hIcon);
        }

        /* Add the label */
        if (AllocAndLoadString(&lpPartialStr,
                               hInstance,
                               IDS_STOP_DEPENDS))
        {
            fullLen = _tcslen(lpPartialStr) + _tcslen(pStopInfo->pInfo->pCurrentService->lpDisplayName) + 1;

            lpStr = HeapAlloc(ProcessHeap,
                              0,
                              fullLen * sizeof(TCHAR));
            if (lpStr)
            {
                _sntprintf(lpStr, fullLen, lpPartialStr, pStopInfo->pInfo->pCurrentService->lpDisplayName);

                SendDlgItemMessage(hDlg,
                                   IDC_STOP_DEPENDS,
                                   WM_SETTEXT,
                                   0,
                                   (LPARAM)lpStr);

                bRet = TRUE;

                HeapFree(ProcessHeap,
                         0,
                         lpStr);
            }

            HeapFree(ProcessHeap,
                     0,
                     lpPartialStr);
        }

        /* Get the list of dependencies */
        lpDependencies = GetDependentServices(pStopInfo->hMainService, &dwCount);
        if (lpDependencies)
        {
            LPENUM_SERVICE_STATUS lpEnumServiceStatus;
            DWORD i;

            for (i = 0; i < dwCount; i++)
            {
                lpEnumServiceStatus = &lpDependencies[i];

                /* Add the service to the listbox */
                SendDlgItemMessage(hDlg,
                                   IDC_STOP_DEPENDS_LB,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)lpEnumServiceStatus->lpDisplayName);
            }

            HeapFree(ProcessHeap,
                     0,
                     lpDependencies);
        }
    }

    return bRet;
}


INT_PTR CALLBACK
StopDependsDialogProc(HWND hDlg,
                      UINT message,
                      WPARAM wParam,
                      LPARAM lParam)
{
    PSTOP_INFO pStopInfo = NULL;

    /* Get the window context */
    pStopInfo = (PSTOP_INFO)GetWindowLongPtr(hDlg,
                                             GWLP_USERDATA);
    if (pStopInfo == NULL && message != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (message)
    {
        case WM_INITDIALOG:
        {
            BOOL bRet = FALSE;

            pStopInfo = (PSTOP_INFO)lParam;
            if (pStopInfo != NULL)
            {
                bRet = DoInitDependsDialog(pStopInfo, hDlg);
            }

            return bRet;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}
