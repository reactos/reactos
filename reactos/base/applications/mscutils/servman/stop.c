/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops running a service
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


static BOOL
StopService(PMAIN_WND_INFO pInfo,
            LPWSTR lpServiceName)
{
    //SERVICE_STATUS_PROCESS ServiceStatus;
    //DWORD dwBytesNeeded;
    //DWORD dwStartTime;
   // DWORD dwTimeout;
    //HWND hProgDlg;
    BOOL bRet = FALSE;
/*
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
                        We exceeded our max wait time, give up
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
*/
    return bRet;
}

static BOOL
StopDependantServices(PMAIN_WND_INFO pInfo,
                      LPWSTR lpServiceName)
{
    //LPENUM_SERVICE_STATUS lpDependencies;
    //SC_HANDLE hDepService;
    //DWORD dwCount;
    BOOL bRet = FALSE;

    MessageBox(NULL, L"Rewrite StopDependentServices", NULL, 0);
    /*

    lpDependencies = GetServiceDependents(hService, &dwCount);
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
    }*/

    return bRet;
}


BOOL
DoStop(PMAIN_WND_INFO pInfo)
{
    BOOL bRet = FALSE;

    if (pInfo)
    {
        /* Does this service have anything which depends on it? */
        if (TV2_HasDependantServices(pInfo->pCurrentService->lpServiceName))
        {
            /* It does, list them and ask the user if they want to stop them as well */
            if (DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_DLG_DEPEND_STOP),
                               pInfo->hMainWnd,
                               StopDependsDialogProc,
                               (LPARAM)&pInfo) == IDOK)
            {
                /* Stop all the dependany services */
                if (StopDependantServices(pInfo, pInfo->pCurrentService->lpServiceName))
                {
                    /* Finally stop the requested service */
                    bRet = StopService(pInfo, pInfo->pCurrentService->lpServiceName);
                }
            }
        }
        else
        {
            /* No dependants, just stop the service */
            bRet = StopService(pInfo, pInfo->pCurrentService->lpServiceName);
        }
    }

    return bRet;
}
