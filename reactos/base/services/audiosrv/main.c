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


/* This is for testing only! */
VOID
InitializeFakeDevice()
{
    PnP_AudioDevice* list_node;

    list_node = CreateDeviceDescriptor(L"ThisDeviceDoesNotReallyExist", TRUE);
    AppendAudioDeviceToList(list_node);
    DestroyDeviceDescriptor(list_node);
}

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
            logmsg("* Interrogation\n");
            return NO_ERROR;
        }

        case SERVICE_CONTROL_STOP :
        case SERVICE_CONTROL_SHUTDOWN :
        {
            logmsg("* Service Stop/Shutdown request received\n");

            logmsg("Unregistering device notifications\n");
            UnregisterDeviceNotifications();

            logmsg("Destroying audio device list\n");
            DestroyAudioDeviceList();

            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(service_status_handle, &service_status);

            service_status.dwWin32ExitCode = 0;
            service_status.dwCurrentState = SERVICE_STOPPED;

            SetServiceStatus(service_status_handle, &service_status);

            logmsg("* Service stopped\n");

            return NO_ERROR;
        }

        case SERVICE_CONTROL_DEVICEEVENT :
        {
            logmsg("* Device Event\n");
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
    logmsg("* Service starting\n");
    logmsg("Registering service control handler...\n");
    service_status_handle = RegisterServiceCtrlHandlerEx(SERVICE_NAME,
                                                         ServiceControlHandler,
                                                         NULL);

    logmsg("Service status handle %d\n", service_status_handle);
    if ( ! service_status_handle )
    {
        logmsg("Failed to register service control handler\n");
        /* FIXME - we should fail */
    }

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

    logmsg("Creating audio device list\n");
    /* This creates the audio device list and mutex */
    if ( ! CreateAudioDeviceList(AUDIO_LIST_MAX_SIZE) )
    {
        logmsg("Failed to create audio device list\n");
        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    logmsg("Registering for device notifications\n");
    /* We want to know when devices are added/removed */
    if ( ! RegisterForDeviceNotifications() )
    {
        /* FIXME: This is not fatal at present as ROS does not support this */
        logmsg("Failed to register for device notifications\n");
/*
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
*/
    }

    InitializeFakeDevice();

    logmsg("Processing existing devices\n");
    /* Now find any devices that already exist on the system */
    if ( ! ProcessExistingDevices() )
    {
        logmsg("Could not process existing devices\n");
        UnregisterDeviceNotifications();
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    logmsg("* Service started");
    /* Tell SCM we are now running, and we may be stopped */
    service_status.dwCurrentState = SERVICE_RUNNING;
    service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(service_status_handle, &service_status);
}

int main()
{
    logmsg("Audio Service main()\n");
    StartServiceCtrlDispatcher(service_table);
    return 0;
}
