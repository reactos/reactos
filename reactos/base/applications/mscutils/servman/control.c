/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/control.c
 * PURPOSE:     Pauses and resumes a service
 * COPYRIGHT:   Copyright 2006-2010 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoControl(PMAIN_WND_INFO Info,
          HWND hProgress,
          DWORD Control)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus = {0};
    SERVICE_STATUS Status;
    DWORD BytesNeeded = 0;
    DWORD dwStartTickCount;
    DWORD dwOldCheckPoint;
    DWORD dwWaitTime;
    DWORD dwMaxWait;
    DWORD dwReqState;
    BOOL bRet = FALSE;

    /* Set the state we're interested in */
    switch (Control)
    {
        case SERVICE_CONTROL_PAUSE:
            dwReqState = SERVICE_PAUSED;
            break;
        case SERVICE_CONTROL_CONTINUE:
            dwReqState = SERVICE_RUNNING;
            break;
        default:
            /* Unhandled control code */
            return FALSE;
    }

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager)
    {
        hService = OpenService(hSCManager,
                               Info->pCurrentService->lpServiceName,
                               SERVICE_PAUSE_CONTINUE | SERVICE_INTERROGATE | SERVICE_QUERY_STATUS);
        if (hService)
        {
            if (hProgress)
            {
                /* Increment the progress bar */
                IncrementProgressBar(hProgress, DEFAULT_STEP);
            }

            /* Send the control message to the service */
            if (ControlService(hService,
                               Control,
                               &Status))
            {
                /* Get the service status */
                if (QueryServiceStatusEx(hService,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(SERVICE_STATUS_PROCESS),
                                         &BytesNeeded))
                {
                    /* We don't want to wait for more than 30 seconds */
                    dwMaxWait = 30000;
                    dwStartTickCount = GetTickCount();

                    /* Loop until it's at the correct state */
                    while (ServiceStatus.dwCurrentState != dwReqState)
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
                            if(GetTickCount() >= dwStartTickCount + dwMaxWait)
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

                if (ServiceStatus.dwCurrentState == dwReqState)
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


BOOL
DoPause(PMAIN_WND_INFO Info)
{
    HWND hProgress;
    BOOL bRet = FALSE;

    /* Create a progress window to track the progress of the pausing service */
    hProgress = CreateProgressDialog(Info->hMainWnd,
                                     IDS_PROGRESS_INFO_PAUSE);
    if (hProgress)
    {
        /* Set the service name and reset the progress bag */
        InitializeProgressDialog(hProgress, Info->pCurrentService->lpServiceName);

        /* Resume the requested service */
        bRet = DoControl(Info, hProgress, SERVICE_CONTROL_PAUSE);

        /* Complete and destroy the progress bar */
        DestroyProgressDialog(hProgress, bRet);
    }

    return bRet;
}


BOOL
DoResume(PMAIN_WND_INFO Info)
{
    HWND hProgress;
    BOOL bRet = FALSE;

    /* Create a progress window to track the progress of the resuming service */
    hProgress = CreateProgressDialog(Info->hMainWnd,
                                     IDS_PROGRESS_INFO_RESUME);
    if (hProgress)
    {
        /* Set the service name and reset the progress bag */
        InitializeProgressDialog(hProgress, Info->pCurrentService->lpServiceName);

        /* Resume the requested service */
        bRet = DoControl(Info, hProgress, SERVICE_CONTROL_CONTINUE);

        /* Complete and destroy the progress bar */
        DestroyProgressDialog(hProgress, bRet);
    }

    return bRet;
}
