/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdStart.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

/* Enumerate all running services */
static
INT
EnumerateRunningServices(VOID)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwServiceCount;
    DWORD dwResumeHandle = 0;
    LPENUM_SERVICE_STATUS lpServiceBuffer = NULL;
    INT i;
    INT nError = 0;
    DWORD dwError = ERROR_SUCCESS;

    hManager = OpenSCManagerW(NULL,
                              SERVICES_ACTIVE_DATABASE,
                              SC_MANAGER_ENUMERATE_SERVICE);
    if (hManager == NULL)
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

    EnumServicesStatusW(hManager,
                        SERVICE_WIN32,
                        SERVICE_ACTIVE,
                        NULL,
                        0,
                        &dwBufferSize,
                        &dwServiceCount,
                        &dwResumeHandle);

    if (dwBufferSize != 0)
    {
        lpServiceBuffer = HeapAlloc(GetProcessHeap(), 0, dwBufferSize);
        if (lpServiceBuffer != NULL)
        {
            if (EnumServicesStatusW(hManager,
                                    SERVICE_WIN32,
                                    SERVICE_ACTIVE,
                                    lpServiceBuffer,
                                    dwBufferSize,
                                    &dwBufferSize,
                                    &dwServiceCount,
                                    &dwResumeHandle))
            {
                PrintToConsole(L"The following services hav been started:\n\n");

                for (i = 0; i < dwServiceCount; i++)
                {
                    PrintToConsole(L"  %s\n", lpServiceBuffer[i].lpDisplayName);
                }
            }

            HeapFree(GetProcessHeap(), 0, lpServiceBuffer);
        }
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

/* Start the service argv[2] */
static
INT
StartOneService(INT argc, WCHAR **argv)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    LPCWSTR *lpArgVectors = NULL;
    DWORD dwError = ERROR_SUCCESS;
    INT nError = 0;
    INT i;

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
                            SERVICE_START);
    if (hService == NULL)
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

    lpArgVectors = HeapAlloc(GetProcessHeap(),
                             0,
                             (argc - 2) * sizeof(LPCWSTR));
    if (lpArgVectors == NULL)
    {
        dwError = GetLastError();
        nError = 1;
        goto done;
    }

    for (i = 2; i < argc; i++)
    {
        lpArgVectors[i - 2] = argv[i];
    }

    if (!StartServiceW(hService,
                       (DWORD)argc - 2,
                       lpArgVectors))
    {
        dwError = GetLastError();
        nError = 1;
    }

done:
    if (lpArgVectors != NULL)
        HeapFree(GetProcessHeap(), 0, (LPVOID)lpArgVectors);

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

INT
cmdStart(INT argc, WCHAR **argv)
{
    INT i;

    if (argc == 2)
    {
        return EnumerateRunningServices();
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            PrintResourceString(IDS_START_HELP);
            return 1;
        }
    }

    return StartOneService(argc, argv);
}
