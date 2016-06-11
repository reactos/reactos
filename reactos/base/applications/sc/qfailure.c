/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/qfailure.c
 * PURPOSE:     Show the service failure action
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL QueryFailure(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD cbBytesNeeded = 0;
    LPSERVICE_FAILURE_ACTIONS pServiceFailure = NULL;
    INT i;

#ifdef SCDBG
    _tprintf(_T("service to show failure action - %s\n\n"), ServiceName);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             NULL,
                             0,
                             &cbBytesNeeded))
    {
        if (cbBytesNeeded == 0)
        {
            bResult = FALSE;
            goto done;
        }
    }

    pServiceFailure = HeapAlloc(GetProcessHeap(), 0, cbBytesNeeded);
    if (pServiceFailure == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             (LPBYTE)pServiceFailure,
                             cbBytesNeeded,
                             &cbBytesNeeded))
    {
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("SERVICE_NAME: %s\n"), ServiceName);
    _tprintf(_T("        RESET_PERIOD       : %lu seconds\n"), pServiceFailure->dwResetPeriod);
    _tprintf(_T("        REBOOT_MESSAGE     : %s\n"), (pServiceFailure->lpRebootMsg) ? pServiceFailure->lpRebootMsg : _T(""));
    _tprintf(_T("        COMMAND_LINE       : %s\n"), (pServiceFailure->lpCommand) ? pServiceFailure->lpCommand : _T(""));
    _tprintf(_T("        FAILURE_ACTIONS    : "));
    for (i = 0; i < pServiceFailure->cActions; i++)
    {
        if (i != 0)
            _tprintf(_T("                             "));
        switch (pServiceFailure->lpsaActions[i].Type)
        {
            case SC_ACTION_NONE:
                continue;

            case SC_ACTION_RESTART:
                _tprintf(_T("RESTART -- Delay = %lu milliseconds.\n"), pServiceFailure->lpsaActions[i].Delay);
                break;

            case SC_ACTION_REBOOT:
                _tprintf(_T("REBOOT -- Delay = %lu milliseconds.\n"), pServiceFailure->lpsaActions[i].Delay);
                break;

            case SC_ACTION_RUN_COMMAND:
                _tprintf(_T("RUN_COMMAND -- Delay = %lu milliseconds.\n"), pServiceFailure->lpsaActions[i].Delay);
                break;

            default:
                _tprintf(_T("\n"));
                break;
        }
    }

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pServiceFailure != NULL)
        HeapFree(GetProcessHeap(), 0, pServiceFailure);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
