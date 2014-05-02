/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            cmdPause.c
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

    if (argc != 3)
    {
        puts("Usage: NET PAUSE <Service Name>");
        return 1;
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

