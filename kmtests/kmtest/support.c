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
#include <winioctl.h>

#include <assert.h>

#include "kmtest.h"
#include <kmt_public.h>
#include <kmt_test.h>

/* pseudo-tests */
START_TEST(Create)
{
    // nothing to do here. All tests create the service if needed
}

START_TEST(Delete)
{
    SC_HANDLE Handle = NULL;
    DWORD Error = KmtDeleteService(L"Kmtest", &Handle);

    ok_eq_hex(Error, (DWORD)ERROR_SUCCESS);
}

START_TEST(Start)
{
    // nothing to do here. All tests start the service
}

START_TEST(Stop)
{
    // TODO: requiring the service to be started for this is... bad,
    // especially when it's marked for deletion and won't start ;)
    SC_HANDLE Handle = NULL;
    DWORD Error = KmtStopService(L"Kmtest", &Handle);

    ok_eq_hex(Error, (DWORD)ERROR_SUCCESS);
    Error = KmtCloseService(&Handle);
    ok_eq_hex(Error, (DWORD)ERROR_SUCCESS);
}

/* test support functions for special-purpose drivers */

extern HANDLE KmtestHandle;

/**
 * @name KmtRunKernelTest
 *
 * Run the specified kernel-mode test part
 *
 * @param TestName
 *        Name of the test to run
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtRunKernelTest(
    IN PCSTR TestName)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD BytesRead;

    if (!DeviceIoControl(KmtestHandle, IOCTL_KMTEST_RUN_TEST, (PVOID)TestName, strlen(TestName), NULL, 0, &BytesRead, NULL))
        error(Error);

    return Error;
}

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

    TestDeviceHandle = CreateFile(DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
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
    DWORD BytesRead;

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), NULL, 0, NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
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
    DWORD BytesRead;

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), (PVOID)String, strlen(String), NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
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
    IN OUT PDWORD Length)
{
    assert(Length);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), Buffer, *Length, NULL, 0, Length, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}