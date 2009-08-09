/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/services.c
 * PURPOSE:          Audio Service Plug and Play
 * COPYRIGHT:        Copyright 2009 Johannes Anderwald
 */

#include <windows.h>
#include <winuser.h>
#include <dbt.h>
#include <setupapi.h>

#include <ks.h>
#include <ksmedia.h>

#include <audiosrv/audiosrv.h>
#include "audiosrv.h"


BOOL
StartSystemAudioServices()
{
    SC_HANDLE hSCManager, hService;

    logmsg("Starting system audio services\n");

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        logmsg("Failed to open service manager\n");
        return FALSE;
    }

    hService = OpenService(hSCManager, L"sysaudio", SERVICE_ALL_ACCESS);
    if (hService)
    {
        if (!StartService(hService, 0, NULL))
        {
            logmsg("Failed to start sysaudio service\n");
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return FALSE;
        }
        CloseServiceHandle(hService);
        logmsg("Sysaudio service started\n");
        // FIXME
        // wait untill service is started
    }

    hService = OpenService(hSCManager, L"wdmaud", SERVICE_ALL_ACCESS);
    if (hService)
    {
        if (!StartService(hService, 0, NULL))
        {
            logmsg("Failed to start sysaudio service\n");
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return FALSE;
        }
        CloseServiceHandle(hService);
        logmsg("Wdmaud service started\n");
    }


    CloseServiceHandle(hSCManager);
    return TRUE;
}











