/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/control
 * PURPOSE:     Stops, pauses and resumes a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern HWND hListView;


BOOL Control(HWND hProgDlg, DWORD Control)
{
    HWND hProgBar;
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    SERVICE_STATUS_PROCESS ServiceStatus;
    SERVICE_STATUS Status;
    LVITEM item;
    DWORD BytesNeeded = 0;
    DWORD Loop = 5; //FIXME: testing value. needs better control

    item.mask = LVIF_PARAM;
    item.iItem = GetSelectedItem();
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    Service = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

    /* set the progress bar range and step */
    hProgBar = GetDlgItem(hProgDlg, IDC_SERVCON_PROGRESS);
    SendMessage(hProgBar, PBM_SETRANGE, 0, MAKELPARAM(0, Loop));
    SendMessage(hProgBar, PBM_SETSTEP, (WPARAM)1, 0);

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* open handle to the service */
    hSc = OpenService(hSCManager, Service->lpServiceName,
                      SC_MANAGER_ALL_ACCESS);
    if (hSc == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* process requested action */
    if (! ControlService(hSc, Control, &Status))
    {
        GetError(0);
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
        GetError(0);
        return FALSE;
    }

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState != Control || !Loop)
    {
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
        Loop--;
    }

    CloseServiceHandle(hSc);

    if (ServiceStatus.dwCurrentState == Control)
    {
        SendMessage(hProgBar, PBM_DELTAPOS, Loop, 0);
        Sleep(1000);
        return TRUE;
    }
    else
        return FALSE;

}
