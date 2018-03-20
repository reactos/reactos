/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for open_osfhandle on WSASocket
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "ws2_32.h"
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <io.h>
#include <fcntl.h>

#define WINVER_WIN8    0x0602

static void run_open_osfhandle(void)
{
    DWORD type;
    int handle, err;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;

    SOCKET fd = WSASocketA(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
    ok (fd != INVALID_SOCKET, "Invalid socket\n");
    if (fd == INVALID_SOCKET)
        return;

    type = GetFileType((HANDLE)fd);
    ok(type == FILE_TYPE_PIPE, "Expected type FILE_TYPE_PIPE, was: %lu\n", type);

    Status = NtQueryVolumeInformationFile((HANDLE)fd, &StatusBlock, &DeviceInfo, sizeof(DeviceInfo), FileFsDeviceInformation);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lx\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        RTL_OSVERSIONINFOEXW rtlinfo = { sizeof(rtlinfo), 0 };
        ULONG Characteristics;
        DWORD dwWinVersion;
        ok(DeviceInfo.DeviceType == FILE_DEVICE_NAMED_PIPE, "Expected FILE_DEVICE_NAMED_PIPE, got: 0x%lx\n", DeviceInfo.DeviceType);

        RtlGetVersion((PRTL_OSVERSIONINFOW)&rtlinfo);
        dwWinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;
        Characteristics = dwWinVersion >= WINVER_WIN8 ? 0x20000 : 0;
        ok(DeviceInfo.Characteristics == Characteristics, "Expected 0x%lx, got: 0x%lx\n", Characteristics, DeviceInfo.Characteristics);
    }

    handle = _open_osfhandle(fd, _O_BINARY | _O_RDWR);
    err = *_errno();

    ok(handle != -1, "Expected a valid handle (%i)\n", err);
    if (handle != -1)
    {
        /* To close a file opened with _open_osfhandle, call _close. The underlying handle is also closed by
           a call to _close, so it is not necessary to call the Win32 function CloseHandle on the original handle. */
        _close(handle);
    }
    else
    {
        closesocket(fd);
    }
}

START_TEST(open_osfhandle)
{
    WSADATA wdata;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wdata);
    ok(iResult == 0, "WSAStartup failed, iResult == %d\n", iResult);
    run_open_osfhandle();
    WSACleanup();
}

