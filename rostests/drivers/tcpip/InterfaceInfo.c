/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         User mode part of the TcpIp.sys test suite
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <apitest.h>

#include <ipexport.h>
#include <winioctl.h>
#include <tcpioctl.h>
#include <tcpip_undoc.h>

START_TEST(InterfaceInfo)
{
    IP_INTERFACE_INFO* pInterfaceInfo;
    IP_INTERFACE_INFO InterfaceInfo;
    HANDLE FileHandle;
    DWORD BufferSize;
    BOOL Result;
    ULONG i;

    /* Open a control channel file for TCP */
    FileHandle = CreateFileW(
        L"\\\\.\\Tcp",
        FILE_READ_DATA | FILE_WRITE_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    ok(FileHandle != INVALID_HANDLE_VALUE, "CreateFile failed, GLE %lu\n", GetLastError());

    /* Try the IOCTL */
    BufferSize = 0;
    pInterfaceInfo = &InterfaceInfo;
    Result = DeviceIoControl(
        FileHandle,
        IOCTL_IP_INTERFACE_INFO,
        NULL,
        0,
        pInterfaceInfo,
        sizeof(InterfaceInfo),
        &BufferSize,
        NULL);
    ok(!Result, "DeviceIoControl succeeded.\n");
    ok_long(GetLastError(), ERROR_INVALID_FUNCTION);
    ok_long(BufferSize, 0);

    CloseHandle(FileHandle);

    /* This IOCTL only works with \Device\Ip */
    FileHandle = CreateFileW(
        L"\\\\.\\Ip",
        FILE_READ_DATA | FILE_WRITE_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    ok(FileHandle != INVALID_HANDLE_VALUE, "CreateFile failed, GLE %lu\n", GetLastError());

    /* Get the buffer size. */
    BufferSize = 0;
    pInterfaceInfo = &InterfaceInfo;
    Result = DeviceIoControl(
        FileHandle,
        IOCTL_IP_INTERFACE_INFO,
        NULL,
        0,
        pInterfaceInfo,
        sizeof(InterfaceInfo),
        &BufferSize,
        NULL);
    ok(Result, "DeviceIoControl failed.\n");
    ok(BufferSize != 0, "Buffer size is zero.\n");
    trace("Buffer size is %lu.\n", BufferSize);

    if (BufferSize > sizeof(InterfaceInfo))
    {
        pInterfaceInfo = HeapAlloc(GetProcessHeap(), 0, BufferSize);
        ok(pInterfaceInfo != NULL, "HeapAlloc failed.\n");

        /* Send IOCTL to fill the buffer in. */
        Result = DeviceIoControl(
                FileHandle,
                IOCTL_IP_INTERFACE_INFO,
                NULL,
                0,
                pInterfaceInfo,
                BufferSize,
                &BufferSize,
                NULL);
        ok(Result, "DeviceIoControl failed.\n");
    }

    /* Nothing much to test. Just print out the adapters we got */
    trace("We got %ld adapters.\n", pInterfaceInfo->NumAdapters);
    for (i = 0; i < pInterfaceInfo->NumAdapters; i++)
    {
        trace("\tIndex %lu, name %S\n", pInterfaceInfo->Adapter[i].Index, pInterfaceInfo->Adapter[i].Name);
    }

    if (pInterfaceInfo != &InterfaceInfo)
        HeapFree(GetProcessHeap(), 0, pInterfaceInfo);
    CloseHandle(FileHandle);
}
