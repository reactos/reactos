/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoStartService(PMAIN_WND_INFO Info,
               HWND hProgDlg)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded = 0;
    BOOL bRet = FALSE;
    BOOL bDispErr = TRUE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager != NULL)
    {
        hSc = OpenService(hSCManager,
                          Info->pCurrentService->lpServiceName,
                          SERVICE_ALL_ACCESS);
        if (hSc != NULL)
        {
            if (StartService(hSc,
                              0,
                              NULL))
            {
                bDispErr = FALSE;

                if (QueryServiceStatusEx(hSc,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(SERVICE_STATUS_PROCESS),
                                         &BytesNeeded))
                {
                    DWORD dwStartTickCount = GetTickCount();
                    DWORD dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                    DWORD dwMaxWait = 2000 * 60; // wait for 2 mins

                    IncrementProgressBar(hProgDlg);

                    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                    {
                        DWORD dwWaitTime = ServiceStatus.dwWaitHint / 10;

                        if (!QueryServiceStatusEx(hSc,
                                                  SC_STATUS_PROCESS_INFO,
                                                  (LPBYTE)&ServiceStatus,
                                                  sizeof(SERVICE_STATUS_PROCESS),
                                                  &BytesNeeded))
                        {
                            break;
                        }

                        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
                        {
                            /* The service is making progress, increment the progress bar */
                            IncrementProgressBar(hProgDlg);
                            dwStartTickCount = GetTickCount();
                            dwOldCheckPoint = ServiceStatus.dwCheckPoint;
                        }
                        else
                        {
                            if(GetTickCount() >= dwStartTickCount + dwMaxWait)
                            {
                                /* give up */
                                break;
                            }
                        }

                        if(dwWaitTime < 200)
                            dwWaitTime = 200;
                        else if (dwWaitTime > 10000)
                            dwWaitTime = 10000;

                        Sleep(dwWaitTime);
                    }
                }
            }

            CloseServiceHandle(hSc);
        }

        CloseServiceHandle(hSCManager);
    }

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        CompleteProgressBar(hProgDlg);
        Sleep(500);
        bRet = TRUE;
    }
    else
    {
        if (bDispErr)
            GetError();
        else
            DisplayString(_T("The service failed to start"));
    }

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

        DestroyWindow(hProgDlg);
    }

    return bRet;
}
