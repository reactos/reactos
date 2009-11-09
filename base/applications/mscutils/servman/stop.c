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

static BOOL
StopDependentServices(PSTOP_INFO pStopInfo,
                      SC_HANDLE hService)
{
    LPENUM_SERVICE_STATUS lpDependencies;
    SC_HANDLE hDepService;
    DWORD dwCount;
    BOOL bRet = FALSE;

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
    }

    return bRet;
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
