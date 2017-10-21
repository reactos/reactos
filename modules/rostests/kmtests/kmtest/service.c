/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Loader service control functions
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "kmtest.h"

#include <assert.h>

#define SERVICE_ACCESS (SERVICE_START | SERVICE_STOP | DELETE)

/*
 * This is an internal function not meant for use by the kmtests app,
 * so we declare it here instead of kmtest.h
 */
DWORD
KmtpCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    IN DWORD ServiceType,
    OUT SC_HANDLE *ServiceHandle);


static SC_HANDLE ScmHandle;

/**
 * @name KmtServiceInit
 *
 * Initialize service management routines (by opening the service control manager)
 *
 * @return Win32 error code
 */
DWORD
KmtServiceInit(VOID)
{
    DWORD Error = ERROR_SUCCESS;

    assert(!ScmHandle);

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!ScmHandle)
        error(Error);

    return Error;
}

/**
 * @name KmtServiceCleanup
 *
 * Clean up resources used by service management routines.
 *
 * @param IgnoreErrors
 *        If TRUE, the function will never set ErrorLineAndFile, and always return ERROR_SUCCESS
 *
 * @return Win32 error code
 */
DWORD
KmtServiceCleanup(
    BOOLEAN IgnoreErrors)
{
    DWORD Error = ERROR_SUCCESS;

    if (ScmHandle && !CloseServiceHandle(ScmHandle) && !IgnoreErrors)
        error(Error);

    return Error;
}

/**
 * @name KmtCreateService
 *
 * Create the specified driver service and return a handle to it
 *
 * @param ServiceName
 *        Name of the service to create
 * @param ServicePath
 *        File name of the driver, relative to the current directory
 * @param DisplayName
 *        Service display name
 * @param ServiceHandle
 *        Pointer to a variable to receive the handle to the service
 *
 * @return Win32 error code
 */
DWORD
KmtCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle)
{
    return KmtpCreateService(ServiceName,
                             ServicePath,
                             DisplayName,
                             SERVICE_KERNEL_DRIVER,
                             ServiceHandle);
}

/**
 * @name KmtStartService
 *
 * Start the specified driver service by handle or name (and return a handle to it)
 *
 * @param ServiceName
 *        If *ServiceHandle is NULL, name of the service to start
 * @param ServiceHandle
 *        Pointer to a variable containing the service handle,
 *        or NULL (in which case it will be filled with a handle to the service)
 *
 * @return Win32 error code
 */
DWORD
KmtStartService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle)
{
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceHandle);
    assert(ServiceName || *ServiceHandle);

    if (!*ServiceHandle)
        *ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ACCESS);

    if (!*ServiceHandle)
        error_goto(Error, cleanup);

    if (!StartService(*ServiceHandle, 0, NULL))
        error_goto(Error, cleanup);

cleanup:
    return Error;
}

/**
 * @name KmtCreateAndStartService
 *
 * Create and start the specified driver service and return a handle to it
 *
 * @param ServiceName
 *        Name of the service to create
 * @param ServicePath
 *        File name of the driver, relative to the current directory
 * @param DisplayName
 *        Service display name
 * @param ServiceHandle
 *        Pointer to a variable to receive the handle to the service
 * @param RestartIfRunning
 *        TRUE to stop and restart the service if it is already running
 *
 * @return Win32 error code
 */
DWORD
KmtCreateAndStartService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle,
    IN BOOLEAN RestartIfRunning)
{
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceHandle);

    Error = KmtCreateService(ServiceName, ServicePath, DisplayName, ServiceHandle);

    if (Error && Error != ERROR_SERVICE_EXISTS)
        goto cleanup;

    Error = KmtStartService(ServiceName, ServiceHandle);

    if (Error != ERROR_SERVICE_ALREADY_RUNNING)
        goto cleanup;

    Error = ERROR_SUCCESS;

    if (!RestartIfRunning)
        goto cleanup;

    Error = KmtStopService(ServiceName, ServiceHandle);
    if (Error)
        goto cleanup;

    Error = KmtStartService(ServiceName, ServiceHandle);
    if (Error)
        goto cleanup;

cleanup:
    assert(Error || *ServiceHandle);
    return Error;
}

/**
 * @name KmtStopService
 *
 * Stop the specified driver service by handle or name (and return a handle to it)
 *
 * @param ServiceName
 *        If *ServiceHandle is NULL, name of the service to stop
 * @param ServiceHandle
 *        Pointer to a variable containing the service handle,
 *        or NULL (in which case it will be filled with a handle to the service)
 *
 * @return Win32 error code
 */
DWORD
KmtStopService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle)
{
    DWORD Error = ERROR_SUCCESS;
    SERVICE_STATUS ServiceStatus;

    assert(ServiceHandle);
    assert(ServiceName || *ServiceHandle);

    if (!*ServiceHandle)
        *ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ACCESS);

    if (!*ServiceHandle)
        error_goto(Error, cleanup);

    if (!ControlService(*ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus))
        error_goto(Error, cleanup);

cleanup:
    return Error;
}

/**
 * @name KmtDeleteService
 *
 * Delete the specified driver service by handle or name (and return a handle to it)
 *
 * @param ServiceName
 *        If *ServiceHandle is NULL, name of the service to delete
 * @param ServiceHandle
 *        Pointer to a variable containing the service handle.
 *        Will be set to NULL on success
 *
 * @return Win32 error code
 */
DWORD
KmtDeleteService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle)
{
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceHandle);
    assert(ServiceName || *ServiceHandle);

    if (!*ServiceHandle)
        *ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ACCESS);

    if (!*ServiceHandle)
        error_goto(Error, cleanup);

    if (!DeleteService(*ServiceHandle))
        error_goto(Error, cleanup);

    if (*ServiceHandle)
        CloseServiceHandle(*ServiceHandle);

cleanup:
    return Error;
}

/**
 * @name KmtCloseService
 *
 * Close the specified driver service handle
 *
 * @param ServiceHandle
 *        Pointer to a variable containing the service handle.
 *        Will be set to NULL on success
 *
 * @return Win32 error code
 */
DWORD KmtCloseService(
    IN OUT SC_HANDLE *ServiceHandle)
{
    DWORD Error = ERROR_SUCCESS;

    assert(ServiceHandle);

    if (*ServiceHandle && !CloseServiceHandle(*ServiceHandle))
        error_goto(Error, cleanup);

    *ServiceHandle = NULL;

cleanup:
    return Error;
}


/*
 * Private function, not meant for use in kmtests
 * See KmtCreateService & KmtFltCreateService
 */
DWORD
KmtpCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    IN DWORD ServiceType,
    OUT SC_HANDLE *ServiceHandle)
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR DriverPath[MAX_PATH];
    HRESULT result = S_OK;

    assert(ServiceHandle);
    assert(ServiceName && ServicePath);

    if (!GetModuleFileName(NULL, DriverPath, sizeof DriverPath / sizeof DriverPath[0]))
        error_goto(Error, cleanup);

    assert(wcsrchr(DriverPath, L'\\') != NULL);
    wcsrchr(DriverPath, L'\\')[1] = L'\0';

    result = StringCbCat(DriverPath, sizeof DriverPath, ServicePath);
    if (FAILED(result))
        error_value_goto(Error, result, cleanup);

    if (GetFileAttributes(DriverPath) == INVALID_FILE_ATTRIBUTES)
        error_goto(Error, cleanup);

    *ServiceHandle = CreateService(ScmHandle, ServiceName, DisplayName,
                                   SERVICE_ACCESS, ServiceType, SERVICE_DEMAND_START,
                                   SERVICE_ERROR_NORMAL, DriverPath, NULL, NULL, NULL, NULL, NULL);

    if (!*ServiceHandle)
        error_goto(Error, cleanup);

cleanup:
    return Error;
}