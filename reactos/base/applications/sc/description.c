/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/description.c
 * PURPOSE:     Change the service description
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL SetDescription(LPCTSTR ServiceName, LPCTSTR Description)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    SERVICE_DESCRIPTION ServiceDescription;

#ifdef SCDBG
    _tprintf(_T("service to set description - %s\n\n"), ServiceName);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, SERVICE_CHANGE_CONFIG);
    if (hService == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    ServiceDescription.lpDescription = (LPTSTR)Description;

    if (!ChangeServiceConfig2(hService,
                              SERVICE_CONFIG_DESCRIPTION,
                              (LPBYTE)&ServiceDescription))
    {
        bResult = FALSE;
        goto done;
    }

done:
    if (bResult == FALSE)
        ReportLastError();

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
