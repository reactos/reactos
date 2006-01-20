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


BOOL Control(DWORD Control)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    SERVICE_STATUS Status;
    LVITEM item;

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

    /* open handle to the service */
    hSc = OpenService(hSCManager, Service->lpServiceName,
                      SERVICE_PAUSE_CONTINUE | SERVICE_STOP);
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

    CloseServiceHandle(hSc);

    return TRUE;

}
