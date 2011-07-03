/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

#include "kmtest.h"
#include <kmt_test.h>
#include <kmt_public.h>

/* pseudo-tests */
START_TEST(Create)
{
    // nothing to do here. All tests start the service if needed
}

START_TEST(Delete)
{
    // TODO: delete kmtest service
}

START_TEST(Start)
{
    // nothing to do here. All tests start the service
}

START_TEST(Stop)
{
    // TODO: stop kmtest service
}

/* test support functions for special-purpose drivers */

static WCHAR TestServiceName[MAX_PATH];
static SC_HANDLE TestServiceHandle;
static HANDLE TestDeviceHandle;

/**
 * @name KmtLoadDriver
 *
 * Load the specified special-purpose driver (create/start the service)
 *
 * @param ServiceName
 *        Name of the driver service (Kmtest- prefix will be added automatically)
 * @param RestartIfRunning
 *        TRUE to stop and restart the service if it is already running
 */
VOID
KmtLoadDriver(
    IN PCWSTR ServiceName,
    IN BOOLEAN RestartIfRunning)
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR ServicePath[MAX_PATH];

    StringCbCopy(ServicePath, sizeof ServicePath, ServiceName);
    StringCbCat(ServicePath, sizeof ServicePath, L"_drv.sys");

    StringCbCopy(TestServiceName, sizeof TestServiceName, L"Kmtest-");
    StringCbCat(TestServiceName, sizeof TestServiceName, ServiceName);

    Error = KmtCreateAndStartService(TestServiceName, ServicePath, NULL, &TestServiceHandle, RestartIfRunning);

    if (Error)
    {
        // TODO
        __debugbreak();
    }
}

/**
 * @name KmtUnloadDriver
 *
 * Unload special-purpose driver (stop the service)
 */
VOID
KmtUnloadDriver(VOID)
{
    DWORD Error = ERROR_SUCCESS;

    Error = KmtStopService(TestServiceName, &TestServiceHandle);

    if (Error)
    {
        // TODO
        __debugbreak();
    }
}

/**
 * @name KmtOpenDriver
 *
 * Open special-purpose driver (acquire a device handle)
 */
VOID
KmtOpenDriver(VOID)
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR DevicePath[MAX_PATH];

    StringCbCopy(DevicePath, sizeof DevicePath, L"\\\\.\\Global\\GLOBALROOT\\Device\\");
    StringCbCat(DevicePath, sizeof DevicePath, TestServiceName);

    TestDeviceHandle = CreateFile(KMTEST_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (TestDeviceHandle == INVALID_HANDLE_VALUE)
        error(Error);

    if (Error)
    {
        // TODO
        __debugbreak();
    }

}

/**
 * @name KmtCloseDriver
 *
 * Close special-purpose driver (close device handle)
 */
VOID
KmtCloseDriver(VOID)
{
    DWORD Error = ERROR_SUCCESS;

    if (TestDeviceHandle && !CloseHandle(TestDeviceHandle))
        error(Error);

    if (Error)
    {
        // TODO
        __debugbreak();
    }
}

/* TODO: check if these will be useful */

/**
 * @name KmtSendToDriver
 *
 * Unload special-purpose driver (stop the service)
 *
 * @param ControlCode
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendToDriver(
    IN DWORD ControlCode)
{
    // TODO
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**
 * @name KmtSendStringToDriver
 *
 * Unload special-purpose driver (stop the service)
 *
 * @param ControlCode
 * @param String
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendStringToDriver(
    IN DWORD ControlCode,
    IN PCSTR String)
{
    // TODO
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**
 * @name KmtSendBufferToDriver
 *
 * @param ControlCode
 * @param Buffer
 * @param Length
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendBufferToDriver(
    IN DWORD ControlCode,
    IN OUT PVOID Buffer,
    IN DWORD Length)
{
    // TODO
    return ERROR_CALL_NOT_IMPLEMENTED;
}