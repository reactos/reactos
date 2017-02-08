/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

INT cmdStop(INT argc, WCHAR **argv)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD dwError = ERROR_SUCCESS;
    INT nError = 0;

    if (argc != 3)
    {
        /* FIXME: Print usage message! */
        printf("Usage: NET STOP <Service name>\n");
        return 1;
    }

    hManager = OpenSCManagerW(NULL,
                              SERVICES_ACTIVE_DATABASE,
                              SC_MANAGER_ENUMERATE_SERVICE);
    if (hManager == NULL)
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

    hService = OpenServiceW(hManager,
                            argv[2],
                            SERVICE_STOP);
    if (hService == NULL)
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);

    if (dwError != ERROR_SUCCESS)
    {
        /* FIXME: Print proper error message */
        printf("Error: %lu\n", dwError);
    }

    return nError;
}
