/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops running a service
 * COPYRIGHT:   Copyright 2006-2010 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


static BOOL
StopService(PMAIN_WND_INFO pInfo,
            LPWSTR lpServiceName,
            HWND hProgress OPTIONAL)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD dwBytesNeeded;
    DWORD dwStartTime;
    DWORD dwTimeout;
    BOOL bRet = FALSE;

    if (hProgress)
    {
        /* Increment the progress bar */
        IncrementProgressBar(hProgress, DEFAULT_STEP);
    }

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               lpServiceName,
                               SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (hService)
        {
            if (hProgress)
            {
                /* Increment the progress bar */
                IncrementProgressBar(hProgress, DEFAULT_STEP);
            }

            /* Set the wait time to 30 secs */
            dwStartTime = GetTickCount();
            dwTimeout = 30000;

            /* Send the service the stop code */
            if (ControlService(hService,
                               SERVICE_CONTROL_STOP,
                               (LPSERVICE_STATUS)&ServiceStatus))
            {
                if (hProgress)
                {
                    /* Increment the progress bar */
                    IncrementProgressBar(hProgress, DEFAULT_STEP);
                }

                while (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
                {
                    Sleep(ServiceStatus.dwWaitHint);

                    if (hProgress)
                    {
                        /* Increment the progress bar */
                        IncrementProgressBar(hProgress, DEFAULT_STEP);
                    }

                    if (QueryServiceStatusEx(hService,
                                             SC_STATUS_PROCESS_INFO,
                                             (LPBYTE)&ServiceStatus,
                                             sizeof(SERVICE_STATUS_PROCESS),
                                             &dwBytesNeeded))
                    {
                        /* Have we exceeded our wait time? */
                        if (GetTickCount() - dwStartTime > dwTimeout)
                        {
                            /* Yep, give up */
                            break;
                        }
                    }
                }

                /* If the service is stopped, return TRUE */
                if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
                {
                    bRet = TRUE;
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }

    return bRet;
}

static BOOL
StopDependantServices(PMAIN_WND_INFO pInfo,
                      LPWSTR lpServiceName)
{
    BOOL bRet = FALSE;

    MessageBox(NULL, L"Rewrite StopDependentServices", NULL, 0);

    return bRet;
}


BOOL
DoStop(PMAIN_WND_INFO pInfo)
{
    HWND hProgress;
    LPWSTR lpServiceList;
    BOOL bRet = FALSE;

    if (pInfo)
    {
        /* Does this service have anything which depends on it? */
        if (TV2_HasDependantServices(pInfo->pCurrentService->lpServiceName))
        {
            /* It does, get a list of all the services which need stopping */
            lpServiceList = GetListOfServicesToStop(pInfo->pCurrentService->lpServiceName);
            if (lpServiceList)
            {
                /* List them and ask the user if they want to stop them */
                if (DialogBoxParamW(hInstance,
                                    MAKEINTRESOURCEW(IDD_DLG_DEPEND_STOP),
                                    pInfo->hMainWnd,
                                    StopDependsDialogProc,
                                    (LPARAM)lpServiceList) == IDOK)
                {
                    /* Stop all the dependant services */
                    StopDependantServices(pInfo, pInfo->pCurrentService->lpServiceName);
                }
            }
        }

        /* Create a progress window to track the progress of the stopping service */
        hProgress = CreateProgressDialog(pInfo->hMainWnd,
                                         pInfo->pCurrentService->lpServiceName,
                                         IDS_PROGRESS_INFO_STOP);

        /* Stop the requested service */
        bRet = StopService(pInfo,
                           pInfo->pCurrentService->lpServiceName,
                           hProgress);

        if (hProgress)
        {
            /* Complete and destroy the progress bar */
            DestroyProgressDialog(hProgress, TRUE);
        }
    }

    return bRet;
}
