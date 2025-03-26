#include "DriverTester.h"

static BOOL
Initialize(LPCWSTR lpDriverPath)
{
    if (!RegisterDriver(DRIVER_NAME, lpDriverPath))
    {
        wprintf(L"[%lu] Failed to install %s\n", GetLastError(), DRIVER_NAME);
        return FALSE;
    }

    return TRUE;
}

static BOOL
Uninitialize(LPCWSTR lpDriverPath)
{
    if (!UnregisterDriver(DRIVER_NAME))
    {
        wprintf(L"[%lu] Failed to unregister %s\n", GetLastError(), DRIVER_NAME);
        return FALSE;
    }

    return TRUE;
}

static BOOL
UsermodeMethod(LPCWSTR lpDriverPath)
{
    wprintf(L"\nStarting %s.sys via the SCM\n", DRIVER_NAME);

    if (!StartDriver(DRIVER_NAME))
    {
        wprintf(L"[%lu] Failed to start %s\n", GetLastError(), DRIVER_NAME);
        UnregisterDriver(DRIVER_NAME);
        return FALSE;
    }

    wprintf(L"\tStarted\n");

    wprintf(L"Stopping %s.sys via the SCM\n", DRIVER_NAME);

    if (!StopDriver(DRIVER_NAME))
    {
        wprintf(L"[%lu] Failed to stop %s\n", GetLastError(), DRIVER_NAME);
        UnregisterDriver(DRIVER_NAME);
        return FALSE;
    }

    wprintf(L"\tStopped\n");

    return TRUE;
}

static BOOL
UndocumentedMethod(LPCWSTR lpDriverPath)
{
    wprintf(L"\nStarting %s.sys via native API\n", DRIVER_NAME);

    if (!NtStartDriver(DRIVER_NAME))
    {
        wprintf(L"[%lu] Failed to start %s\n", GetLastError(), DRIVER_NAME);
        UnregisterDriver(DRIVER_NAME);
        return FALSE;
    }

    wprintf(L"\tStarted\n");

    wprintf(L"Stopping %s.sys  via native API\n", DRIVER_NAME);

    if (!NtStopDriver(DRIVER_NAME))
    {
        wprintf(L"[%lu] Failed to stop %s\n", GetLastError(), DRIVER_NAME);
        UnregisterDriver(DRIVER_NAME);
        return FALSE;
    }

    wprintf(L"\tStopped\n");

    return TRUE;
}


static BOOL
SneakyUndocumentedMethods(LPCWSTR lpDriverPath)
{
    WCHAR szDevice[MAX_PATH];

    if (ConvertPath(lpDriverPath, szDevice))
    {
        wprintf(L"\nStarting %s.sys via NtSetSystemInformation with SystemLoadGdiDriverInformation\n", DRIVER_NAME);
        if (LoadVia_SystemLoadGdiDriverInformation(szDevice))
        {
            wprintf(L"\tStarted\n");

            NtStopDriver(DRIVER_NAME);
        }

        wprintf(L"\nStarting %s.sys via NtSetSystemInformation with SystemExtendServiceTableInformation\n", DRIVER_NAME);
        if (LoadVia_SystemExtendServiceTableInformation(szDevice))
        {
            wprintf(L"\tStarted\n");

            NtStopDriver(DRIVER_NAME);
        }

        return TRUE;
    }

    return FALSE;
}


int __cdecl wmain(int argc, wchar_t *argv[])
{
    WCHAR buf[MAX_PATH];

    if (argc != 2)
    {
        wprintf(L"Usage: DriverTester.exe <path>");
        return -1;
    }

    if (!SearchPathW(NULL,
                     argv[1],
                     L".sys",
                     MAX_PATH,
                     buf,
                     NULL))
    {
        wprintf(L"%s does not exist", argv[1]);
        return -1;
    }

    if (Initialize(argv[1]))
    {
        //
        // Load using conventional SCM methods
        //
        UsermodeMethod(argv[1]);

        //
        // Load using undocumented NtLoad/UnloadDriver
        //
        UndocumentedMethod(argv[1]);

        //
        // Load using hidden unknown methods
        //
        SneakyUndocumentedMethods(argv[1]);

        Uninitialize(argv[1]);
    }

    return 0;
}

