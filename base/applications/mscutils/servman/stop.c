/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop.c
 * PURPOSE:     Stops running a service
 * COPYRIGHT:   Copyright 2006-2015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define MAX_WAIT_TIME   30000

DWORD
DoStopService(_In_z_ LPWSTR ServiceName,
              _In_opt_ HANDLE hProgress)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded;
    DWORD StartTime;
    DWORD WaitTime;
    DWORD Timeout;
    DWORD dwResult = ERROR_SUCCESS;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_CONNECT);
    if (!hSCManager) return GetLastError();

    hService = OpenServiceW(hSCManager,
                            ServiceName,
                            SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!hService)
    {
        dwResult = GetLastError();
        CloseServiceHandle(hSCManager);
        return dwResult;
    }

    if (hProgress)
    {
        /* Increment the progress bar */
        IncrementProgressBar(hProgress, DEFAULT_STEP);
    }

    /* Set the start and max wait times */
    StartTime = GetTickCount();
    Timeout = MAX_WAIT_TIME;

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

            if (QueryServiceStatusEx(hService,
                                        SC_STATUS_PROCESS_INFO,
                                        (LPBYTE)&ServiceStatus,
                                        sizeof(SERVICE_STATUS_PROCESS),
                                        &BytesNeeded))
            {
                /* Have we exceeded our wait time? */
                if (GetTickCount() - StartTime > Timeout)
                {
                    /* Yep, give up */
                    DPRINT1("Timeout\n");
                    dwResult = ERROR_SERVICE_REQUEST_TIMEOUT;
                    break;
                }
            }
            else
            {
                dwResult = GetLastError();
                DPRINT1("QueryServiceStatusEx failed: %d\n", dwResult);
            }
        }

        /* If the service is stopped, return TRUE */
        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
        {
            dwResult = ERROR_SUCCESS;
        }
    }
    else
    {
        dwResult = GetLastError();
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return dwResult;
}
