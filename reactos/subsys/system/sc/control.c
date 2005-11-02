/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/control.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

/*
 * handles the following commands:
 * control, continue, interrogate, pause, stop
 */

BOOL Control(DWORD Control, TCHAR **Args)
{
    SC_HANDLE hSc;
    SERVICE_STATUS Status;
    LPCTSTR ServiceName = *Args;


    hSc = OpenService(hSCManager, ServiceName, DELETE);

    if (hSc == NULL)
    {
        _tprintf(_T("openService failed\n"));
        ReportLastError();
        return FALSE;
    }

    if (! ControlService(hSc, Control, &Status))
    {
        _tprintf(_T("controlService failed\n"));
        ReportLastError();
        return FALSE;
    }

    CloseServiceHandle(hSc);
    return TRUE;

}
