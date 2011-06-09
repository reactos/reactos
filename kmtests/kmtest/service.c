/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Loader service control functions
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define UNICODE
#include <windows.h>
#include <strsafe.h>

#include "kmtest.h"

#define SERVICE_NAME L"Kmtest"
#define SERVICE_PATH L"\\kmtest_drv.sys"

DWORD Service_Create(SC_HANDLE hScm)
{
    DWORD error = ERROR_SUCCESS;
    SC_HANDLE hService = NULL;
    wchar_t driverPath[MAX_PATH];
    HRESULT result = S_OK;

    if (!GetCurrentDirectory(sizeof driverPath / sizeof driverPath[0], driverPath)
            || FAILED(result = StringCbCat(driverPath, sizeof driverPath, SERVICE_PATH)))
    {
        if (FAILED(result))
            error = result;
        else
            error = GetLastError();
        goto cleanup;
    }

    hService = CreateService(hScm, SERVICE_NAME, L"ReactOS Kernel-Mode Test Suite Driver",
                            SERVICE_START, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                            SERVICE_ERROR_NORMAL, driverPath, NULL, NULL, NULL, NULL, NULL);

    if (!hService)
        error = GetLastError();

cleanup:
    return error;
}

DWORD Service_Delete(SC_HANDLE hScm)
{
    DWORD error = ERROR_SUCCESS;
    SC_HANDLE hService = NULL;

    hService = OpenService(hScm, SERVICE_NAME, DELETE);

    if (!hService)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!DeleteService(hService))
        error = GetLastError();

cleanup:
    if (hService)
        CloseServiceHandle(hService);

    return error;
}

DWORD Service_Start(SC_HANDLE hScm)
{
    DWORD error = ERROR_SUCCESS;
    SC_HANDLE hService = NULL;

    hService = OpenService(hScm, SERVICE_NAME, SERVICE_START);

    if (!hService)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!StartService(hService, 0, NULL))
        error = GetLastError();

cleanup:
    if (hService)
        CloseServiceHandle(hService);

    return error;
}

DWORD Service_Stop(SC_HANDLE hScm)
{
    DWORD error = ERROR_SUCCESS;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS serviceStatus;

    hService = OpenService(hScm, SERVICE_NAME, SERVICE_STOP);

    if (!hService)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
        error = GetLastError();

cleanup:
    if (hService)
        CloseServiceHandle(hService);

    return error;
}

DWORD Service_Control(SERVICE_FUNC *Service_Func)
{
    DWORD error = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;

    hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (!hScm)
    {
        error = GetLastError();
        goto cleanup;
    }

    error = Service_Func(hScm);

cleanup:
    if (hScm)
        CloseServiceHandle(hScm);

    return error;
}
