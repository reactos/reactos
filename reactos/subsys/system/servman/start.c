/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern HWND hListView;

BOOL DoStartService(VOID)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    LVITEM item;
    DWORD BytesNeeded;
    INT ArgCount = 0;
    BOOL Loop = 10; //FIXME: testing value. needs better control

    item.mask = LVIF_PARAM;
    item.iItem = GetSelectedItem();
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    Service = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

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

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState == SERVICE_START_PENDING || !Loop)
    {
        /* wait before checking status */
        Sleep(ServiceStatus.dwWaitHint);

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

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        return TRUE;
    else
        return FALSE;

}

