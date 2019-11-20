/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite user-mode support routines
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2013 Nikolay Borisov <nib9@aber.ac.uk>
 */

#include <kmt_test.h>

#include "kmtest.h"
#include <kmt_public.h>

#include <assert.h>
#include <debug.h>

extern HANDLE KmtestHandle;

/**
 * @name KmtUserCallbackThread
 *
 * Thread routine which awaits callback requests from kernel-mode
 *
 * @return Win32 error code
 */
DWORD
WINAPI
KmtUserCallbackThread(
    PVOID Parameter)
{
    DWORD Error = ERROR_SUCCESS;
    /* TODO: RequestPacket? */
    KMT_CALLBACK_REQUEST_PACKET RequestPacket;
    KMT_RESPONSE Response;
    DWORD BytesReturned;
    HANDLE LocalKmtHandle;

    UNREFERENCED_PARAMETER(Parameter);

    /* concurrent IoCtls on the same (non-overlapped) handle aren't possible,
     * so open a separate one.
     * For more info http://www.osronline.com/showthread.cfm?link=230782 */
    LocalKmtHandle = CreateFile(KMTEST_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (LocalKmtHandle == INVALID_HANDLE_VALUE)
        error_goto(Error, cleanup);

    while (1)
    {
        if (!DeviceIoControl(LocalKmtHandle, IOCTL_KMTEST_USERMODE_AWAIT_REQ, NULL, 0, &RequestPacket, sizeof(RequestPacket), &BytesReturned, NULL))
            error_goto(Error, cleanup);
        ASSERT(BytesReturned == sizeof(RequestPacket));

        switch (RequestPacket.OperationClass)
        {
            case QueryVirtualMemory:
            {
                SIZE_T InfoBufferSize = VirtualQuery(RequestPacket.Parameters, &Response.MemInfo, sizeof(Response.MemInfo));
                /* FIXME: an error is a valid result. That should go as a response to kernel mode instead of terminating the thread */
                if (InfoBufferSize == 0)
                    error_goto(Error, cleanup);

                if (!DeviceIoControl(LocalKmtHandle, IOCTL_KMTEST_USERMODE_SEND_RESPONSE, &RequestPacket.RequestId, sizeof(RequestPacket.RequestId), &Response, sizeof(Response), &BytesReturned, NULL))
                    error_goto(Error, cleanup);
                ASSERT(BytesReturned == 0);

                break;
            }
            default:
                DPRINT1("Unrecognized user-mode callback request\n");
                break;
        }
    }

cleanup:
    if (LocalKmtHandle != INVALID_HANDLE_VALUE)
        CloseHandle(LocalKmtHandle);

    DPRINT("Callback handler dying! Error code %lu", Error);
    return Error;
}


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
    HANDLE CallbackThread;
    DWORD Error = ERROR_SUCCESS;
    DWORD BytesRead;

    CallbackThread = CreateThread(NULL, 0, KmtUserCallbackThread, NULL, 0, NULL);

    if (!DeviceIoControl(KmtestHandle, IOCTL_KMTEST_RUN_TEST, (PVOID)TestName, (DWORD)strlen(TestName), NULL, 0, &BytesRead, NULL))
        error(Error);

    if (CallbackThread != NULL)
         CloseHandle(CallbackThread);

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

    StringCbCopyW(ServicePath, sizeof(ServicePath), ServiceName);
    StringCbCatW(ServicePath, sizeof(ServicePath), L"_drv.sys");

    StringCbCopyW(TestServiceName, sizeof(TestServiceName), L"Kmtest-");
    StringCbCatW(TestServiceName, sizeof(TestServiceName), ServiceName);

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

    StringCbCopyW(DevicePath, sizeof(DevicePath), L"\\\\.\\Global\\GLOBALROOT\\Device\\");
    StringCbCatW(DevicePath, sizeof(DevicePath), TestServiceName);

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
 * Send an I/O control message with no arguments to the driver opened with KmtOpenDriver
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

    assert(ControlCode < 0x400);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), NULL, 0, NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}

/**
 * @name KmtSendStringToDriver
 *
 * Send an I/O control message with a string argument to the driver opened with KmtOpenDriver
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

    assert(ControlCode < 0x400);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), (PVOID)String, (DWORD)strlen(String), NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}

/**
 * @name KmtSendWStringToDriver
 *
 * Send an I/O control message with a wide string argument to the driver opened with KmtOpenDriver
 *
 * @param ControlCode
 * @param String
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendWStringToDriver(
    IN DWORD ControlCode,
    IN PCWSTR String)
{
    DWORD BytesRead;

    assert(ControlCode < 0x400);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), (PVOID)String, (DWORD)wcslen(String) * sizeof(WCHAR), NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}

/**
 * @name KmtSendUlongToDriver
 *
 * Send an I/O control message with an integer argument to the driver opened with KmtOpenDriver
 *
 * @param ControlCode
 * @param Value
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendUlongToDriver(
    IN DWORD ControlCode,
    IN DWORD Value)
{
    DWORD BytesRead;

    assert(ControlCode < 0x400);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), &Value, sizeof(Value), NULL, 0, &BytesRead, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}

/**
 * @name KmtSendBufferToDriver
 *
 * Send an I/O control message with the specified arguments to the driver opened with KmtOpenDriver
 *
 * @param ControlCode
 * @param Buffer
 * @param InLength
 * @param OutLength
 *
 * @return Win32 error code as returned by DeviceIoControl
 */
DWORD
KmtSendBufferToDriver(
    IN DWORD ControlCode,
    IN OUT PVOID Buffer OPTIONAL,
    IN DWORD InLength,
    IN OUT PDWORD OutLength)
{
    assert(OutLength);
    assert(Buffer || (!InLength && !*OutLength));
    assert(ControlCode < 0x400);

    if (!DeviceIoControl(TestDeviceHandle, KMT_MAKE_CODE(ControlCode), Buffer, InLength, Buffer, *OutLength, OutLength, NULL))
        return GetLastError();

    return ERROR_SUCCESS;
}
