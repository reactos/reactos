/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005-2010 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoStartService(PMAIN_WND_INFO Info,
               HWND hProgress,
               LPWSTR lpStartParams)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded = 0;
    DWORD dwStartTickCount;
    DWORD dwOldCheckPoint;
    DWORD dwWaitTime;
    DWORD dwMaxWait;
    BOOL bRet = FALSE;

    BOOL bWhiteSpace = TRUE;
    LPWSTR lpChar;
    DWORD dwArgsCount = 0;
    LPCWSTR *lpArgsVector = NULL;

    if (lpStartParams != NULL)
    {
        /* Count the number of arguments */
        lpChar = lpStartParams;
        while (*lpChar != 0)
        {
            if (iswspace(*lpChar))
            {
                bWhiteSpace = TRUE;
            }
            else
            {
                if (bWhiteSpace == TRUE)
                {
                    dwArgsCount++;
                    bWhiteSpace = FALSE;
                }
            }

            lpChar++;
        }

        /* Allocate the arguments vector and add one for the service name */
        lpArgsVector = LocalAlloc(LMEM_FIXED, (dwArgsCount + 1) * sizeof(LPCWSTR));
        if (!lpArgsVector)
            return FALSE;

        /* Make the service name the first argument */
        lpArgsVector[0] = Info->pCurrentService->lpServiceName;

        /* Fill the arguments vector */
        dwArgsCount = 1;
        bWhiteSpace = TRUE;
        lpChar = lpStartParams;
        while (*lpChar != 0)
        {
            if (iswspace(*lpChar))
            {
                *lpChar = 0;
                bWhiteSpace = TRUE;
            }
            else
            {
                if (bWhiteSpace == TRUE)
                {
                    lpArgsVector[dwArgsCount] = lpChar;
                    dwArgsCount++;
                    bWhiteSpace = FALSE;
                }
            }

            lpChar++;
        }
    }

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               Info->pCurrentService->lpServiceName,
                               SERVICE_START | SERVICE_QUERY_STATUS);
        if (hService)
        {
            if (hProgress)
            {
                /* Increment the progress bar */
                IncrementProgressBar(hProgress, DEFAULT_STEP);
            }

            /* Start the service */
            bRet = StartService(hService,
                                dwArgsCount,
                                lpArgsVector);
            if (!bRet && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
            {
                /* If it's already running, just return TRUE */
                bRet = TRUE;
            }
            else if (bRet)
            {
                bRet = FALSE;

                /* Get the service status to check if it's running */
                if (QueryServiceStatusEx(hService,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(SERVICE_STATUS_PROCESS),
                                         &BytesNeeded))
                {
                    /* We don't want to wait for more than 30 seconds */
                    dwMaxWait = 30000;
                    dwStartTickCount = GetTickCount();

                    /* Loop until it's running */
                    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                    {
                        dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                        dwWaitTime = ServiceStatus.dwWaitHint / 10;

                        /* Get the latest status info */
                        if (!QueryServiceStatusEx(hService,
                                                  SC_STATUS_PROCESS_INFO,
                                                  (LPBYTE)&ServiceStatus,
                                                  sizeof(SERVICE_STATUS_PROCESS),
                                                  &BytesNeeded))
                        {
                            /* Something went wrong... */
                            break;
                        }

                        /* Is the service making progress? */
                        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
                        {
                            /* It is, get the latest tickcount to reset the max wait time */
                            dwStartTickCount = GetTickCount();
                            dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                            IncrementProgressBar(hProgress, DEFAULT_STEP);
                        }
                        else
                        {
                            /* It's not, make sure we haven't exceeded our wait time */
                            if (GetTickCount() >= dwStartTickCount + dwMaxWait)
                            {
                                /* We have, give up */
                                break;
                            }
                        }

                        /* Adjust the wait hint times */
                        if (dwWaitTime < 200)
                            dwWaitTime = 200;
                        else if (dwWaitTime > 10000)
                            dwWaitTime = 10000;

                        /* Wait before trying again */
                        Sleep(dwWaitTime);
                    }
                }

                if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
                {
                    bRet = TRUE;
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }

    if (lpArgsVector)
        LocalFree(lpArgsVector);

    return bRet;
}

BOOL
DoStart(PMAIN_WND_INFO Info, LPWSTR lpStartParams)
{
    HWND hProgress;
    BOOL bRet = FALSE;

    /* Create a progress window to track the progress of the stopping service */
    hProgress = CreateProgressDialog(Info->hMainWnd,
                                     IDS_PROGRESS_INFO_START);
    if (hProgress)
    {
        /* Set the service name and reset the progress bag */
        InitializeProgressDialog(hProgress, Info->pCurrentService->lpServiceName);

        /* Start the requested service */
        bRet = DoStartService(Info, hProgress, lpStartParams);

        /* Complete and destroy the progress bar */
        DestroyProgressDialog(hProgress, bRet);
    }

    return bRet;
}
