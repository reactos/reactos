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

BOOL Control(DWORD Control, LPCTSTR ServiceName, LPCTSTR *Args)
{
    SC_HANDLE hSc;
    SERVICE_STATUS Status;

#ifdef SCDBG    
    /* testing */
    _tprintf(_T("service to control - %s\n\n"), ServiceName);
    _tprintf(_T("command - %lu\n\n"), Control);
    _tprintf(_T("Arguments :\n"));
    while (*Args)
    {
        printf("%s\n", *Args);
        Args++;
    }
#endif /* SCDBG */

    hSc = OpenService(hSCManager, ServiceName,
                      SERVICE_INTERROGATE | SERVICE_PAUSE_CONTINUE |
                      SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL |
                      SERVICE_QUERY_STATUS);

    if (hSc == NULL)
    {
        _tprintf(_T("openService failed\n"));
        ReportLastError();
        return FALSE;
    }

    if (! ControlService(hSc, Control, &Status))
    {
		_tprintf(_T("[SC] controlService FAILED %lu:\n\n"), GetLastError());
        ReportLastError();
        return FALSE;
    }

    CloseServiceHandle(hSc);
    
    /* print the status information */
    
    return TRUE;

}
