/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern HWND hListView;
extern HWND hMainWnd;

BOOL DoStartService(HWND hProgDlg)
{
    HWND hProgBar;
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    DWORD BytesNeeded = 0;
    INT ArgCount = 0;
    DWORD dwStartTickCount, dwOldCheckPoint;

    /* copy pointer to selected service */
    Service = GetSelectedService();

    /* set the progress bar range and step */
    hProgBar = GetDlgItem(hProgDlg, IDC_SERVCON_PROGRESS);
    SendMessage(hProgBar, PBM_SETRANGE, 0, MAKELPARAM(0, PROGRESSRANGE));
    SendMessage(hProgBar, PBM_SETSTEP, (WPARAM)1, 0);

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager, Service->lpServiceName, SERVICE_ALL_ACCESS);
    if (hSc == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* start the service opened */
    if (! StartService(hSc, ArgCount, NULL))
    {
        GetError(0);
        return FALSE;
    }

    /* query the state of the service */
    if (! QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               (LPBYTE)&ServiceStatus,
                               sizeof(SERVICE_STATUS_PROCESS),
                               &BytesNeeded))
    {
        GetError(0);
        return FALSE;
    }

    /* Save the tick count and initial checkpoint. */
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ServiceStatus.dwCheckPoint;

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        DWORD dwWaitTime;

        dwWaitTime = ServiceStatus.dwWaitHint / 10;

        if( dwWaitTime < 500 )
            dwWaitTime = 500;
        else if ( dwWaitTime > 5000 )
            dwWaitTime = 5000;

        /* increment the progress bar */
        SendMessage(hProgBar, PBM_STEPIT, 0, 0);

        /* wait before checking status */
        Sleep(ServiceStatus.dwWaitHint / 8);

        /* check status again */
        if (! QueryServiceStatusEx(
                hSc,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ServiceStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &BytesNeeded))
        {
            GetError(0);
            return FALSE;
        }

        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
        {
            /* The service is making progress. increment the progress bar */
            SendMessage(hProgBar, PBM_STEPIT, 0, 0);
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

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        SendMessage(hProgBar, PBM_DELTAPOS, PROGRESSRANGE, 0);
        Sleep(1000);
        return TRUE;
    }
    else
        return FALSE;

}

