/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005-2015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define MAX_WAIT_TIME   30000

BOOL
DoStartService(LPWSTR ServiceName,
               HANDLE hProgress,
               LPWSTR lpStartParams)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded = 0;
    DWORD StartTickCount;
    DWORD OldCheckPoint;
    DWORD WaitTime;
    DWORD MaxWait;
    BOOL Result = FALSE;

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
        lpArgsVector[0] = ServiceName;

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

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        if (lpArgsVector)
            LocalFree((LPVOID)lpArgsVector);
        return FALSE;
    }

    hService = OpenServiceW(hSCManager,
                            ServiceName,
                            SERVICE_START | SERVICE_QUERY_STATUS);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        if (lpArgsVector)
            LocalFree((LPVOID)lpArgsVector);
        return FALSE;
    }

    /* Start the service */
    Result = StartServiceW(hService,
                            dwArgsCount,
                            lpArgsVector);
    if (!Result && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        /* If it's already running, just return TRUE */
        Result = TRUE;
    }
    else if (Result)
    {
        if (hProgress)
        {
            /* Increment the progress bar */
            IncrementProgressBar(hProgress, DEFAULT_STEP);
        }

        /* Get the service status to check if it's running */
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

            /* Loop until it's running */
            while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            {
                int i;
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
                }
                else
                {
                    /* It's not, make sure we haven't exceeded our wait time */
                    if (GetTickCount() >= StartTickCount + MaxWait)
                    {
                        /* We have, give up */
                        break;
                    }
                }
            }
        }

        if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            Result = TRUE;
        }
    }

    CloseServiceHandle(hService);

    CloseServiceHandle(hSCManager);

    if (lpArgsVector)
        LocalFree((LPVOID)lpArgsVector);

    return Result;
}
