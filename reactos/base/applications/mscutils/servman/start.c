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
                if (QueryServiceStatusEx(hSc,
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(SERVICE_STATUS_PROCESS),
                                         &BytesNeeded))
                {
                    DWORD dwStartTickCount = GetTickCount();
                    DWORD dwOldCheckPoint = ServiceStatus.dwCheckPoint;

                    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                    {
                        DWORD dwWaitTime = ServiceStatus.dwWaitHint / 10;

                        if(dwWaitTime < 1000)
                            dwWaitTime = 500;
                        else if (dwWaitTime > 10000)
                            dwWaitTime = 10000;

                        IncrementProgressBar(hProgDlg);
                        Sleep(dwWaitTime );
                        IncrementProgressBar(hProgDlg);

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
                            if(GetTickCount() - dwStartTickCount > ServiceStatus.dwWaitHint)
                            {
                                /* No progress made within the wait hint */
                                break;
                            }
                        }
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
        Sleep(1000);
        bRet = TRUE;
    }
    else
        GetError();

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
        bRet = DoStartService(Info,
                              hProgDlg);

        SendMessage(hProgDlg,
                    WM_DESTROY,
                    0,
                    0);
    }

    return bRet;
}
