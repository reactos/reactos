/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops running a service
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

typedef struct _STOP_INFO
{
    PMAIN_WND_INFO pInfo;
    SC_HANDLE hSCManager;
    SC_HANDLE hMainService;
} STOP_INFO, *PSTOP_INFO;


static BOOL
StopService(PSTOP_INFO pStopInfo,
            SC_HANDLE hService)
{
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD dwBytesNeeded;
    DWORD dwStartTime;
    DWORD dwTimeout;
    HWND hProgDlg;
    BOOL bRet = FALSE;

    dwStartTime = GetTickCount();
    dwTimeout = 30000; // 30 secs

    hProgDlg = CreateProgressDialog(pStopInfo->pInfo->hMainWnd,
                                    pStopInfo->pInfo->pCurrentService->lpServiceName,
                                    IDS_PROGRESS_INFO_STOP);
    if (hProgDlg)
    {
        IncrementProgressBar(hProgDlg);

        if (ControlService(hService,
                           SERVICE_CONTROL_STOP,
                           (LPSERVICE_STATUS)&ServiceStatus))
        {
            while (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                Sleep(ServiceStatus.dwWaitHint);

                if (QueryServiceStatusEx(hService,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(SERVICE_STATUS_PROCESS),
                                         &dwBytesNeeded))
                {
                    if (GetTickCount() - dwStartTime > dwTimeout)
                    {
                        /* We exceeded our max wait time, give up */
                        break;
                    }
                }
            }

            if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
            {
                bRet = TRUE;
            }
        }

        CompleteProgressBar(hProgDlg);
        Sleep(500);
        DestroyWindow(hProgDlg);
    }

    return bRet;
}

static LPENUM_SERVICE_STATUS
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

static BOOL
StopDependentServices(PSTOP_INFO pStopInfo,
                      SC_HANDLE hService)
{
    LPENUM_SERVICE_STATUS lpDependencies;
    SC_HANDLE hDepService;
    DWORD dwCount;
    BOOL bRet = FALSE;

    lpDependencies = GetDependentServices(hService, &dwCount);
    if (lpDependencies)
    {
        LPENUM_SERVICE_STATUS lpEnumServiceStatus;
        DWORD i;

        for (i = 0; i < dwCount; i++)
        {
            lpEnumServiceStatus = &lpDependencies[i];

            hDepService = OpenService(pStopInfo->hSCManager,
                                      lpEnumServiceStatus->lpServiceName,
                                      SERVICE_STOP | SERVICE_QUERY_STATUS);
            if (hDepService)
            {
                bRet = StopService(pStopInfo, hDepService);

                CloseServiceHandle(hDepService);

                if (!bRet)
                {
                    GetError();
                    break;
                }
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpDependencies);
    }

    return bRet;
}

static BOOL
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


BOOL
DoStop(PMAIN_WND_INFO pInfo)
{
    STOP_INFO stopInfo;
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    BOOL bRet = FALSE;

    if (pInfo)
    {
        stopInfo.pInfo = pInfo;

        hSCManager = OpenSCManager(NULL,
                                   NULL,
                                   SC_MANAGER_ALL_ACCESS);
        if (hSCManager)
        {
            hService = OpenService(hSCManager,
                                   pInfo->pCurrentService->lpServiceName,
                                   SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
            if (hService)
            {
                stopInfo.hSCManager = hSCManager;
                stopInfo.hMainService = hService;

                if (HasDependentServices(hService))
                {
                    INT ret = DialogBoxParam(hInstance,
                                             MAKEINTRESOURCE(IDD_DLG_DEPEND_STOP),
                                             pInfo->hMainWnd,
                                             StopDependsDialogProc,
                                             (LPARAM)&stopInfo);
                    if (ret == IDOK)
                    {
                        if (StopDependentServices(&stopInfo, hService))
                        {
                            bRet = StopService(&stopInfo, hService);
                        }
                    }
                }
                else
                {
                    bRet = StopService(&stopInfo, hService);
                }

                CloseServiceHandle(hService);
            }

            CloseServiceHandle(hSCManager);
        }
    }

    return bRet;
}
