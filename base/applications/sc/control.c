/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/control.c
 * PURPOSE:     Stops, pauses and resumes a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

BOOL
Control(DWORD Control,
        LPCTSTR ServiceName,
        LPCTSTR *Args,
        INT ArgCount)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    SERVICE_STATUS Status;
    DWORD dwDesiredAccess = 0;

#ifdef SCDBG
    LPCTSTR *TmpArgs = Args;
    INT TmpCnt = ArgCount;
    _tprintf(_T("service to control - %s\n"), ServiceName);
    _tprintf(_T("command - %lu\n"), Control);
    _tprintf(_T("Arguments:\n"));
    while (TmpCnt)
    {
        _tprintf(_T("  %s\n"), *TmpArgs);
        TmpArgs++;
        TmpCnt--;
    }
    _tprintf(_T("\n"));
#endif /* SCDBG */

    switch (Control)
    {
        case SERVICE_CONTROL_STOP:
            dwDesiredAccess = SERVICE_STOP;
            break;

        case SERVICE_CONTROL_PAUSE:
            dwDesiredAccess = SERVICE_PAUSE_CONTINUE;
            break;

        case SERVICE_CONTROL_CONTINUE:
            dwDesiredAccess = SERVICE_PAUSE_CONTINUE;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            dwDesiredAccess = SERVICE_INTERROGATE;
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            dwDesiredAccess = 0;
            break;

    }

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        hSc = OpenService(hSCManager,
                          ServiceName,
                          dwDesiredAccess);
        if (hSc != NULL)
        {
            if (ControlService(hSc,
                               Control,
                               &Status))
            {
                SERVICE_STATUS_PROCESS StatusEx;

                /* FIXME: lazy hack ;) */
                CopyMemory(&StatusEx, &Status, sizeof(Status));
                StatusEx.dwProcessId = 0;
                StatusEx.dwServiceFlags = 0;

                PrintService(ServiceName,
                             &StatusEx,
                             FALSE);

                CloseServiceHandle(hSc);
                CloseServiceHandle(hSCManager);

                return TRUE;
            }
        }
        else
            _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
    }

    ReportLastError();
    if (hSc) CloseServiceHandle(hSc);
    if (hSCManager) CloseServiceHandle(hSCManager);
    return FALSE;
}
