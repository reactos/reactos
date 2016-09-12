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
    BOOL bRet = TRUE;

#ifdef SCDBG
    _tprintf(_T("service to delete - %s\n\n"), ServiceName);
#endif

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bRet = FALSE;
        goto done;
    }

    hSc = OpenService(hSCManager, ServiceName, DELETE);
    if (hSc == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bRet = FALSE;
        goto done;
    }

    if (!DeleteService(hSc))
    {
        _tprintf(_T("[SC] DeleteService FAILED %lu:\n\n"), GetLastError());
        bRet = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] DeleteService SUCCESS\n\n"));

done:
    if (bRet == FALSE)
        ReportLastError();

    if (hSc)
        CloseServiceHandle(hSc);

    if (hSCManager)
        CloseServiceHandle(hSCManager);

    return bRet;
}
