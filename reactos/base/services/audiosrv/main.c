/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/main.c
 * PURPOSE:          Audio Service
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>

#include <audiosrv/audiosrv.h>
#include "audiosrv.h"


/* Service table */

SERVICE_TABLE_ENTRY service_table[2] =
{
    { L"AudioSrv", (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;


DWORD WINAPI
ServiceControlHandler(
    DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext)
{
    switch ( dwControl )
    {
        case SERVICE_CONTROL_INTERROGATE :
        {
            return NO_ERROR;
        }

        case SERVICE_CONTROL_STOP :
        case SERVICE_CONTROL_SHUTDOWN :
        {
            UnregisterDeviceNotifications();
            DestroyAudioDeviceList();

            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(service_status_handle, &service_status);

            service_status.dwWin32ExitCode = 0;
            service_status.dwCurrentState = SERVICE_STOPPED;

            SetServiceStatus(service_status_handle, &service_status);

            return NO_ERROR;
        }

        case SERVICE_CONTROL_DEVICEEVENT :
        {
            return HandleDeviceEvent(dwEventType, lpEventData);
        }

        default :
            return ERROR_CALL_NOT_IMPLEMENTED;
    };

    /*SetServiceStatus(service_status_handle, &service_status);*/
}

VOID CALLBACK
ServiceMain(DWORD argc, char** argv)
{
    service_status_handle = RegisterServiceCtrlHandlerEx(SERVICE_NAME,
                                                         ServiceControlHandler,
                                                         NULL);

    /* Set these to defaults */
    service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    service_status.dwServiceSpecificExitCode = 0;
    service_status.dwWin32ExitCode = NO_ERROR;
    service_status.dwWaitHint = 0;
    service_status.dwControlsAccepted = 0;
    service_status.dwCheckPoint = 0;

    /* Tell SCM we're starting */
    service_status.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(service_status_handle, &service_status);

    /* This creates the audio device list and mutex */
    if ( ! CreateAudioDeviceList(AUDIO_LIST_MAX_SIZE) )
    {
        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    /* We want to know when devices are added/removed */
    if ( ! RegisterForDeviceNotifications() )
    {
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    /* Now find any devices that already exist on the system */
    if ( ! ProcessExistingDevices() )
    {
        UnregisterDeviceNotifications();
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    /* Tell SCM we are now running, and we may be stopped */
    service_status.dwCurrentState = SERVICE_RUNNING;
    service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(service_status_handle, &service_status);
}

int main()
{
    StartServiceCtrlDispatcher(service_table);
    return 0;
}
