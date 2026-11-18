/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        base/applications/network/ndisuio_smoke/ndisuio_smoke.c
 * PURPOSE:     Open NDISUIO and print a serial accept marker
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * Smoke-open for the in-tree ndisuio.sys control device.
 *
 * Open name and IOCTLs come from
 * sdk/include/reactos/drivers/ndisuio/nuiouser.h. The open sequence
 * matches base/applications/network/wlanconf/wlanconf.c
 * OpenDriverHandle / OpenAdapterHandle. IRP_MJ_CREATE and
 * IOCTL_NDISUIO_OPEN_DEVICE handling are in
 * drivers/network/ndisuio/createclose.c and
 * drivers/network/ndisuio/ioctl.c.
 *
 * Boot-NdisDisk serial accept: NDISUIO SMOKE OK
 * (same DbgPrint path as base/applications/network/ping/ping.c).
 */

#define WIN32_LEAN_AND_MEAN

#include <windef.h>
#include <winbase.h>
#include <devioctl.h>
#include <ntddndis.h>
#include <nuiouser.h>
#include <stdio.h>

ULONG DbgPrint(const char *Format, ...);

static HANDLE
OpenNdisuio(VOID)
{
    HANDLE hDriver;
    DWORD BytesReturned;
    BOOL Ok;

    hDriver = CreateFileW(NDISUIO_DEVICE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    if (hDriver == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    Ok = DeviceIoControl(hDriver,
                         IOCTL_NDISUIO_BIND_WAIT,
                         NULL,
                         0,
                         NULL,
                         0,
                         &BytesReturned,
                         NULL);
    if (!Ok)
    {
        CloseHandle(hDriver);
        return INVALID_HANDLE_VALUE;
    }

    return hDriver;
}

static BOOL
OpenFirstBinding(HANDLE hDriver)
{
    DWORD QuerySize;
    PNDISUIO_QUERY_BINDING QueryBinding;
    DWORD BytesReturned;
    BOOL Ok;

    QuerySize = sizeof(NDISUIO_QUERY_BINDING) + (1024 * sizeof(WCHAR));
    QueryBinding = HeapAlloc(GetProcessHeap(), 0, QuerySize);
    if (!QueryBinding)
    {
        return FALSE;
    }

    QueryBinding->BindingIndex = 0;
    Ok = DeviceIoControl(hDriver,
                         IOCTL_NDISUIO_QUERY_BINDING,
                         QueryBinding,
                         QuerySize,
                         QueryBinding,
                         QuerySize,
                         &BytesReturned,
                         NULL);
    if (!Ok)
    {
        HeapFree(GetProcessHeap(), 0, QueryBinding);
        return FALSE;
    }

    Ok = DeviceIoControl(hDriver,
                         IOCTL_NDISUIO_OPEN_DEVICE,
                         (PUCHAR)QueryBinding + QueryBinding->DeviceNameOffset,
                         QueryBinding->DeviceNameLength,
                         NULL,
                         0,
                         &BytesReturned,
                         NULL);
    HeapFree(GetProcessHeap(), 0, QueryBinding);
    return Ok;
}

INT
wmain(INT argc, WCHAR *argv[])
{
    HANDLE hDriver;
    DWORD Error;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    hDriver = OpenNdisuio();
    if (hDriver == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        DbgPrint("NDISUIO SMOKE fail open %08lx\n", Error);
        wprintf(L"NDISUIO SMOKE fail open %08lx\n", Error);
        return 1;
    }

    /* CreateFile + BIND_WAIT prove \\.\Ndisuio is usable. OPEN_DEVICE
     * needs a protocol-bound NIC and is reported separately. */
    DbgPrint("NDISUIO SMOKE OK\n");
    wprintf(L"NDISUIO SMOKE OK\n");

    if (OpenFirstBinding(hDriver))
    {
        DbgPrint("NDISUIO SMOKE bind ok\n");
        wprintf(L"NDISUIO SMOKE bind ok\n");
    }
    else
    {
        Error = GetLastError();
        DbgPrint("NDISUIO SMOKE bind miss %08lx\n", Error);
        wprintf(L"NDISUIO SMOKE bind miss %08lx\n", Error);
    }

    CloseHandle(hDriver);
    return 0;
}
