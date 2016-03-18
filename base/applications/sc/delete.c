/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/delete.c
 * PURPOSE:     Delete a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

BOOL Delete(LPCTSTR ServiceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;

#ifdef SCDBG
    _tprintf(_T("service to delete - %s\n\n"), ServiceName);
#endif

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        hSc = OpenService(hSCManager, ServiceName, DELETE);
        if (hSc != NULL)
        {
            if (DeleteService(hSc))
            {
                _tprintf(_T("[SC] DeleteService SUCCESS\n"));

                CloseServiceHandle(hSc);
                CloseServiceHandle(hSCManager);

                return TRUE;
            }
        }
    }

    ReportLastError();

    if (hSc) CloseServiceHandle(hSc);
    if (hSCManager) CloseServiceHandle(hSCManager);

    return FALSE;
}
