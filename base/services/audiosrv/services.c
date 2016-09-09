/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/services.c
 * PURPOSE:          Audio Service Plug and Play
 * COPYRIGHT:        Copyright 2009 Johannes Anderwald
 */

#include "audiosrv.h"

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
            logmsg("QueryServiceStatusEx failed %x\n", GetLastError());
            break;
        }

        if (Info.dwCurrentState == SERVICE_RUNNING)
            return TRUE;

        Sleep(1000);

    }while(Index++ < RetryCount);

    logmsg("Timeout while waiting for service to become ready %p\n", hService);

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
        logmsg("Failed to open service %S %x\n", ServiceName, GetLastError());
        return FALSE;
    }

    if (!StartService(hService, 0, NULL))
    {
        logmsg("Failed to start service %S %x\n", ServiceName, GetLastError());
        CloseServiceHandle(hService);
        return FALSE;
    }

    ret = WaitForService(hService, RetryCount);

    CloseServiceHandle(hService);
    return ret;
}




BOOL
StartSystemAudioServices()
{
    SC_HANDLE hSCManager;

    logmsg("Starting system audio services\n");

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        logmsg("Failed to open service manager %x\n", GetLastError());
        return FALSE;
    }

    logmsg("Starting sysaudio service\n");
    StartAudioService(hSCManager, L"sysaudio", 20);
    logmsg("Starting wdmaud service\n");
    StartAudioService(hSCManager, L"wdmaud", 20);

    CloseServiceHandle(hSCManager);
    return TRUE;
}











