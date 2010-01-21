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
        /* Set the service name and reset the progress bag */
        InitializeProgressDialog(hProgress, lpServiceName);
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
                    /* Don't sleep for more than 3 seconds */
                    if (ServiceStatus.dwWaitHint > 3000)
                        ServiceStatus.dwWaitHint = 3000;

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
                      LPWSTR lpServiceList,
                      HWND hProgress OPTIONAL)
{
    LPWSTR lpStr;
    BOOL bRet = FALSE;

    lpStr = lpServiceList;

    /* Loop through all the services in the list */
    while (TRUE)
    {
        /* Break when we hit the double null */
        if (*lpStr == L'\0' && *(lpStr + 1) == L'\0')
            break;

        /* If this isn't our first time in the loop we'll
           have been left on a null char */
        if (*lpStr == L'\0')
            lpStr++;

        /* Stop the requested service */
        bRet = StopService(pInfo,
                           lpStr,
                           hProgress);

        /* Complete the progress bar if we succeeded */
        if (bRet)
        {
            CompleteProgressBar(hProgress);
        }

        /* Move onto the next string */
        while (*lpStr != L'\0')
            lpStr++;
    }

    return bRet;
}


BOOL
DoStop(PMAIN_WND_INFO pInfo)
{
    HWND hProgress;
    LPWSTR lpServiceList;
    BOOL bRet = FALSE;
    BOOL bStopMainService = TRUE;

    if (pInfo)
    {
        /* Does the service have any dependent services which need stopping first */
        lpServiceList = GetListOfServicesToStop(pInfo->pCurrentService->lpServiceName);
        if (lpServiceList)
        {
            /* Tag the service list to the main wnd info */
            pInfo->pTag = (PVOID)lpServiceList;

            /* List them and ask the user if they want to stop them */
            if (DialogBoxParamW(hInstance,
                                     MAKEINTRESOURCEW(IDD_DLG_DEPEND_STOP),
                                     pInfo->hMainWnd,
                                     StopDependsDialogProc,
                                     (LPARAM)pInfo) == IDOK)
            {
                /* Create a progress window to track the progress of the stopping services */
                hProgress = CreateProgressDialog(pInfo->hMainWnd,
                                                 IDS_PROGRESS_INFO_STOP);

                /* Stop all the dependant services */
                StopDependantServices(pInfo, lpServiceList, hProgress);

                /* Now stop the requested one */
                bRet = StopService(pInfo,
                                   pInfo->pCurrentService->lpServiceName,
                                   hProgress);

                /* We've already stopped the main service, don't try to stop it again */
                bStopMainService = FALSE;

                if (hProgress)
                {
                    /* Complete and destroy the progress bar */
                    DestroyProgressDialog(hProgress, TRUE);
                }
            }
            else
            {
                /* Don't stop the main service if the user selected not to */
                bStopMainService = FALSE;
            }

            HeapFree(GetProcessHeap(),
                     0,
                     lpServiceList);
        }

        /* If the service has no running dependents, then we stop it here */
        if (bStopMainService)
        {
            /* Create a progress window to track the progress of the stopping service */
            hProgress = CreateProgressDialog(pInfo->hMainWnd,
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
    }

    return bRet;
}
