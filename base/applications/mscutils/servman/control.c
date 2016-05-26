/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/control.c
 * PURPOSE:     Pauses and resumes a service
 * COPYRIGHT:   Copyright 2006-2015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define MAX_WAIT_TIME   30000

BOOL
DoControlService(LPWSTR ServiceName,
                 HWND hProgress,
                 DWORD Control)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus = {0};
    SERVICE_STATUS Status;
    DWORD BytesNeeded = 0;
    DWORD StartTickCount;
    DWORD OldCheckPoint;
    DWORD WaitTime;
    DWORD MaxWait;
    DWORD ReqState, i;
    BOOL Result;

    /* Set the state we're interested in */
    switch (Control)
    {
        case SERVICE_CONTROL_PAUSE:
            ReqState = SERVICE_PAUSED;
            break;
        case SERVICE_CONTROL_CONTINUE:
            ReqState = SERVICE_RUNNING;
            break;
        default:
            /* Unhandled control code */
            return FALSE;
    }

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_CONNECT);
    if (!hSCManager) return FALSE;

    hService = OpenServiceW(hSCManager,
                            ServiceName,
                            SERVICE_PAUSE_CONTINUE | SERVICE_INTERROGATE | SERVICE_QUERY_STATUS);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

        /* Send the control message to the service */
    Result = ControlService(hService,
                            Control,
                            &Status);
    if (Result)
    {
        if (hProgress)
        {
            /* Increment the progress bar */
            IncrementProgressBar(hProgress, DEFAULT_STEP);
        }

        /* Get the service status */
        Result = QueryServiceStatusEx(hService,
                                      SC_STATUS_PROCESS_INFO,
                                      (LPBYTE)&ServiceStatus,
                                      sizeof(SERVICE_STATUS_PROCESS),
                                      &BytesNeeded);
        if (Result)
        {
            Result = FALSE;
            MaxWait = MAX_WAIT_TIME;
            OldCheckPoint = ServiceStatus.dwCheckPoint;
            StartTickCount = GetTickCount();

            /* Loop until it's at the correct state */
            while (ServiceStatus.dwCurrentState != ReqState)
            {
                /* Fixup the wait time */
                WaitTime = ServiceStatus.dwWaitHint / 10;

                if (WaitTime < 1000) WaitTime = 1000;
                else if (WaitTime > 10000) WaitTime = 10000;

                /* We don't wanna wait for up to 10 secs without incrementing */
                for (i = WaitTime / 1000; i > 0; i--)
                {
                    Sleep(1000);
                    if (hProgress)
                    {
                        /* Increment the progress bar */
                        IncrementProgressBar(hProgress, DEFAULT_STEP);
                    }
                }

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
                if (ServiceStatus.dwCheckPoint > OldCheckPoint)
                {
                    /* It is, get the latest tickcount to reset the max wait time */
                    StartTickCount = GetTickCount();
                    OldCheckPoint = ServiceStatus.dwCheckPoint;
                    IncrementProgressBar(hProgress, DEFAULT_STEP);
                }
                else
                {
                    /* It's not, make sure we haven't exceeded our wait time */
                    if(GetTickCount() >= StartTickCount + MaxWait)
                    {
                        /* We have, give up */
                        break;
                    }
                }
            }
        }

        if (ServiceStatus.dwCurrentState == ReqState)
        {
            Result = TRUE;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return Result;
}
