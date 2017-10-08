#include "DriverTester.h"


BOOL
RegisterDriver(LPCWSTR lpDriverName,
               LPCWSTR lpPathName)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return FALSE;

retry:
    hService = CreateServiceW(hSCManager,
                              lpDriverName,
                              lpDriverName,
                              SERVICE_ALL_ACCESS,
                              SERVICE_KERNEL_DRIVER,
                              SERVICE_DEMAND_START,
                              SERVICE_ERROR_NORMAL,
                              lpPathName,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if (hService)
    {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return TRUE;
    }
    else
    {
        DWORD err = GetLastError();

        if (err == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            StopDriver(DRIVER_NAME);
            goto retry;
        }

        CloseServiceHandle(hSCManager);

        // return TRUE if the driver is already registered
        return (err == ERROR_SERVICE_EXISTS);
    }
}

BOOL
StartDriver(LPCWSTR lpDriverName)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    BOOL bRet;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return FALSE;

    hService = OpenServiceW(hSCManager,
                            lpDriverName,
                            SERVICE_ALL_ACCESS);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    bRet = StartServiceW(hService, 0, NULL);
    if (!bRet)
    {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
        {
            wprintf(L"%s.sys already running\n", DRIVER_NAME);
            bRet = TRUE;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return bRet;
}

BOOL
StopDriver(LPCWSTR lpDriverName)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    SERVICE_STATUS serviceStatus;
    BOOL bRet;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return FALSE;

    hService = OpenServiceW(hSCManager,
                            lpDriverName,
                            SERVICE_ALL_ACCESS);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    bRet = ControlService(hService,
                          SERVICE_CONTROL_STOP,
                          &serviceStatus);
    if (!bRet)
    {
        if (GetLastError() == ERROR_SERVICE_NOT_ACTIVE)
        {
            wprintf(L"%s.sys wasn't running\n", DRIVER_NAME);
            bRet = TRUE;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return bRet;
}

BOOL
UnregisterDriver(LPCWSTR lpDriverName)
{
    SC_HANDLE hService;
    SC_HANDLE hSCManager;
    BOOL bRet;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return FALSE;

    hService = OpenServiceW(hSCManager,
                            lpDriverName,
                            SERVICE_ALL_ACCESS);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    bRet = DeleteService(hService);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return bRet;
}
