/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for open_osfhandle on WSASocket
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "ws2_32.h"
#include <io.h>
#include <fcntl.h>


static void run_open_osfhandle(void)
{
    DWORD type;
    int handle, err;
    SOCKET fd = WSASocketA(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
    ok (fd != INVALID_SOCKET, "Invalid socket\n");
    if (fd == INVALID_SOCKET)
        return;

    type = GetFileType((HANDLE)fd);
    ok(type == FILE_TYPE_PIPE, "Expected type FILE_TYPE_PIPE, was: %lu\n", type);

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

