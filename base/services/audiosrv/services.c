/*
 * PROJECT:     ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Audio Service Plug and Play
 * COPYRIGHT:   Copyright 2009 Johannes Anderwald
 */

#include "audiosrv.h"

#define NDEBUG
#include <debug.h>

BOOL
WaitForService(
    SC_HANDLE hService,
    ULONG RetryCount)
{
    ULONG Index = 0;
    DWORD dwSize;
    SERVICE_STATUS_PROCESS Info;

    do
    {
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(SERVICE_STATUS_PROCESS), &dwSize))
        {
            DPRINT("QueryServiceStatusEx failed %x\n", GetLastError());
            break;
        }

        if (Info.dwCurrentState == SERVICE_RUNNING)
            return TRUE;

        Sleep(1000);

    } while (Index++ < RetryCount);

    DPRINT("Timeout while waiting for service to become ready %p\n", hService);

    return FALSE;
}

BOOL
StartAudioService(
    SC_HANDLE hSCManager,
    LPWSTR ServiceName,
    ULONG RetryCount)
{
    SC_HANDLE hService;
    BOOL ret;

    hService = OpenService(hSCManager, ServiceName, SERVICE_ALL_ACCESS);
    if (!hService)
    {
        DPRINT("Failed to open service %S %x\n", ServiceName, GetLastError());
        return FALSE;
    }

    if (!StartService(hService, 0, NULL))
    {
        DPRINT("Failed to start service %S %x\n", ServiceName, GetLastError());
        CloseServiceHandle(hService);
        return FALSE;
    }

    ret = WaitForService(hService, RetryCount);

    CloseServiceHandle(hService);
    return ret;
}

BOOL
StartSystemAudioServices(VOID)
{
    SC_HANDLE hSCManager;

    DPRINT("Starting system audio services\n");

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        DPRINT("Failed to open service manager %x\n", GetLastError());
        return FALSE;
    }

    DPRINT("Starting sysaudio service\n");
    StartAudioService(hSCManager, L"sysaudio", 20);
    DPRINT("Starting wdmaud service\n");
    StartAudioService(hSCManager, L"wdmaud", 20);

    CloseServiceHandle(hSCManager);
    return TRUE;
}
