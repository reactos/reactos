/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoStartService(PMAIN_WND_INFO Info,
               HWND hProgDlg)
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

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
    {
        return FALSE;
    }

    hService = OpenServiceW(hSCManager,
                            Info->pCurrentService->lpServiceName,
                            SERVICE_START | SERVICE_QUERY_STATUS);
    if (hService)
    {
        bRet = StartServiceW(hService,
                             0,
                             NULL);
        if (!bRet && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
        {
            bRet = TRUE;
        }
        else if (bRet)
        {
            bRet = FALSE;

            if (QueryServiceStatusEx(hService,
                                     SC_STATUS_PROCESS_INFO,
                                     (LPBYTE)&ServiceStatus,
                                     sizeof(SERVICE_STATUS_PROCESS),
                                     &BytesNeeded))
            {
                dwStartTickCount = GetTickCount();
                dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                dwMaxWait = 30000; // 30 secs

                while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                {
                    dwWaitTime = ServiceStatus.dwWaitHint / 10;

                    if (!QueryServiceStatusEx(hService,
                                              SC_STATUS_PROCESS_INFO,
                                              (LPBYTE)&ServiceStatus,
                                              sizeof(SERVICE_STATUS_PROCESS),
                                              &BytesNeeded))
                    {
                        break;
                    }

                    if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
                    {
                        /* The service is making progress*/
                        dwStartTickCount = GetTickCount();
                        dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                    }
                    else
                    {
                        if (GetTickCount() >= dwStartTickCount + dwMaxWait)
                        {
                            /* We exceeded our max wait time, give up */
                            break;
                        }
                    }

                    if (dwWaitTime < 200)
                        dwWaitTime = 200;
                    else if (dwWaitTime > 10000)
                        dwWaitTime = 10000;

                    Sleep(dwWaitTime);
                }

                if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
                {
                    bRet = TRUE;
                }
            }
        }

        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hSCManager);


    return bRet;
}

BOOL
DoStart(PMAIN_WND_INFO Info)
{
    HWND hProgDlg;
    BOOL bRet = FALSE;

    hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                    Info->pCurrentService->lpServiceName,
                                    IDS_PROGRESS_INFO_START);

    if (hProgDlg)
    {
        IncrementProgressBar(hProgDlg);

        bRet = DoStartService(Info,
                              hProgDlg);

        if (bRet)
        {
            CompleteProgressBar(hProgDlg);
            Sleep(500);
            bRet = TRUE;
        }
        else
        {
            GetError();
        }

        DestroyWindow(hProgDlg);
    }

    return bRet;
}

