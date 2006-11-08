/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/control
 * PURPOSE:     Stops, pauses and resumes a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

BOOL
Control(PMAIN_WND_INFO Info,
        DWORD Control)
{
    HWND hProgBar;
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    SERVICE_STATUS Status;
    LVITEM item;
    DWORD BytesNeeded = 0;
    DWORD dwStartTickCount, dwOldCheckPoint;

    item.mask = LVIF_PARAM;
    item.iItem = Info->SelectedItem;
    SendMessage(Info->hListView,
                LVM_GETITEM,
                0,
                (LPARAM)&item);

    /* set the progress bar range and step */
    hProgBar = GetDlgItem(Info->hProgDlg,
                          IDC_SERVCON_PROGRESS);
    SendMessage(hProgBar,
                PBM_SETRANGE,
                0,
                MAKELPARAM(0, PROGRESSRANGE));

    SendMessage(hProgBar,
                PBM_SETSTEP,
                (WPARAM)1,
                0);

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    /* open handle to the service */
    hSc = OpenService(hSCManager,
                      Info->CurrentService->lpServiceName,
                      SC_MANAGER_ALL_ACCESS);
    if (hSc == NULL)
    {
        GetError();
        return FALSE;
    }

    /* process requested action */
    if (! ControlService(hSc,
                         Control,
                         &Status))
    {
        GetError();
        CloseServiceHandle(hSc);
        return FALSE;
    }

    /* query the state of the service */
    if (! QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               (LPBYTE)&ServiceStatus,
                               sizeof(SERVICE_STATUS_PROCESS),
                               &BytesNeeded))
    {
        GetError();
        return FALSE;
    }

    /* Save the tick count and initial checkpoint. */
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ServiceStatus.dwCheckPoint;

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState != Control)
    {
        DWORD dwWaitTime;

        dwWaitTime = ServiceStatus.dwWaitHint / 10;

        if (dwWaitTime < 500)
            dwWaitTime = 500;
        else if (dwWaitTime > 5000)
            dwWaitTime = 5000;

        /* increment the progress bar */
        SendMessage(hProgBar,
                    PBM_STEPIT,
                    0,
                    0);

        /* wait before checking status */
        Sleep(dwWaitTime);

        /* check status again */
        if (! QueryServiceStatusEx(hSc,
                                   SC_STATUS_PROCESS_INFO,
                                   (LPBYTE)&ServiceStatus,
                                   sizeof(SERVICE_STATUS_PROCESS),
                                   &BytesNeeded))
        {
            GetError();
            return FALSE;
        }

        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
        {
            /* The service is making progress. increment the progress bar */
            SendMessage(hProgBar,
                        PBM_STEPIT,
                        0,
                        0);
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

    CloseServiceHandle(hSc);

    if (ServiceStatus.dwCurrentState == Control)
    {
        SendMessage(hProgBar,
                    PBM_DELTAPOS,
                    PROGRESSRANGE,
                    0);
        Sleep(1000);
        return TRUE;
    }
    else
        return FALSE;

}
