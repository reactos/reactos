/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/control.c
 * PURPOSE:     Stops, pauses and resumes a service
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

BOOL
Control(PMAIN_WND_INFO Info,
        HWND hProgDlg,
        DWORD Control)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus = {0};
    SERVICE_STATUS Status;
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
            if (ControlService(hSc,
                               Control,
                               &Status))
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

                    while (ServiceStatus.dwCurrentState != Control)
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

    if (ServiceStatus.dwCurrentState == Control)
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


BOOL DoPause(PMAIN_WND_INFO Info)
{
    BOOL ret = FALSE;
    HWND hProgDlg;

    hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                    Info->pCurrentService->lpServiceName,
                                    IDS_PROGRESS_INFO_PAUSE);
    if (hProgDlg)
    {
        ret = Control(Info,
                      hProgDlg,
                      SERVICE_CONTROL_PAUSE);

        DestroyWindow(hProgDlg);
    }

    return ret;
}


BOOL DoResume(PMAIN_WND_INFO Info)
{
    BOOL ret = FALSE;
    HWND hProgDlg;

    hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                    Info->pCurrentService->lpServiceName,
                                    IDS_PROGRESS_INFO_RESUME);
    if (hProgDlg)
    {
        ret = Control(Info,
                      hProgDlg,
                      SERVICE_CONTROL_CONTINUE);

        DestroyWindow(hProgDlg);
    }

    return ret;
}
