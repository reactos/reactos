/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/delete.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

BOOL Delete(LPCTSTR ServiceName)
{
    SC_HANDLE hSc;

#ifdef SCDBG  
    /* testing */
    printf("service to delete - %s\n\n", ServiceName);
#endif

    hSc = OpenService(hSCManager, ServiceName, DELETE);

    if (hSc == NULL)
    {
        _tprintf(_T("openService failed\n"));
        ReportLastError();
        return FALSE;
    }

    if (! DeleteService(hSc))
    {
        _tprintf(_T("DeleteService failed\n"));
        ReportLastError();
        return FALSE;
    }

	_tprintf(_T("[SC] DeleteService SUCCESS\n"));
    CloseServiceHandle(hSc);
    return TRUE;
}
