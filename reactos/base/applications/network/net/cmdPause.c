/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdPause.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "net.h"

INT cmdPause(INT argc, WCHAR **argv)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS status;
    INT nError = 0;
    INT i;

    if (argc != 3)
    {
        PrintResourceString(IDS_PAUSE_SYNTAX);
        return 1;
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            PrintResourceString(IDS_PAUSE_HELP);
            return 1;
        }
    }

    hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
    if (hManager == NULL)
    {
        printf("[OpenSCManager] Error: %ld\n", GetLastError());
        nError = 1;
        goto done;
    }

    hService = OpenService(hManager, argv[2], SERVICE_PAUSE_CONTINUE);
    if (hService == NULL)
    {
        printf("[OpenService] Error: %ld\n", GetLastError());
        nError = 1;
        goto done;
    }

    if (!ControlService(hService, SERVICE_CONTROL_PAUSE, &status))
    {
        printf("[ControlService] Error: %ld\n", GetLastError());
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);

    return nError;
}

/* EOF */

