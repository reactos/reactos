/*
 * PROJECT:     ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Audio Service
 * COPYRIGHT:   Copyright 2007 Andrew Greenwood
 */

#include "audiosrv.h"

#define NDEBUG
#include <debug.h>

SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;


/* This is for testing only! */
VOID
InitializeFakeDevice(VOID)
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
    switch (dwControl)
    {
        case SERVICE_CONTROL_INTERROGATE :
        {
            DPRINT("* Interrogation\n");
            return NO_ERROR;
        }

        case SERVICE_CONTROL_STOP :
        case SERVICE_CONTROL_SHUTDOWN :
        {
            DPRINT("* Service Stop/Shutdown request received\n");

            DPRINT("Unregistering device notifications\n");
            UnregisterDeviceNotifications();

            DPRINT("Destroying audio device list\n");
            DestroyAudioDeviceList();

            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(service_status_handle, &service_status);

            service_status.dwWin32ExitCode = 0;
            service_status.dwCurrentState = SERVICE_STOPPED;

            SetServiceStatus(service_status_handle, &service_status);

            DPRINT("* Service stopped\n");

            return NO_ERROR;
        }

        case SERVICE_CONTROL_DEVICEEVENT :
        {
            DPRINT("* Device Event\n");
            return HandleDeviceEvent(dwEventType, lpEventData);
        }

        default :
            return ERROR_CALL_NOT_IMPLEMENTED;
    };

    /*SetServiceStatus(service_status_handle, &service_status);*/
}

VOID CALLBACK
ServiceMain(DWORD argc, LPWSTR argv)
{
    DPRINT("* Service starting\n");
    DPRINT("Registering service control handler\n");
    service_status_handle = RegisterServiceCtrlHandlerExW(SERVICE_NAME,
                                                          ServiceControlHandler,
                                                          NULL);

    DPRINT("Service status handle %d\n", service_status_handle);
    if (!service_status_handle)
    {
        DPRINT("Failed to register service control handler\n");
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

    DPRINT("Creating audio device list\n");
    /* This creates the audio device list and mutex */
    if (!CreateAudioDeviceList(AUDIO_LIST_MAX_SIZE))
    {
        DPRINT("Failed to create audio device list\n");
        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    DPRINT("Registering for device notifications\n");
    /* We want to know when devices are added/removed */
    if (!RegisterForDeviceNotifications())
    {
        /* FIXME: This is not fatal at present as ROS does not support this */
        DPRINT("Failed to register for device notifications\n");
/*
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
*/
    }
    /* start system audio services */
    StartSystemAudioServices();

    InitializeFakeDevice();

    DPRINT("Processing existing devices\n");
    /* Now find any devices that already exist on the system */
    if (!ProcessExistingDevices())
    {
        DPRINT("Could not process existing devices\n");
        UnregisterDeviceNotifications();
        DestroyAudioDeviceList();

        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;
        SetServiceStatus(service_status_handle, &service_status);
        return;
    }

    DPRINT("* Service started\n");
    /* Tell SCM we are now running, and we may be stopped */
    service_status.dwCurrentState = SERVICE_RUNNING;
    service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(service_status_handle, &service_status);
}

int wmain(VOID)
{
    SERVICE_TABLE_ENTRYW service_table[] =
    {
        { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTIONW) ServiceMain },
        { NULL, NULL }
    };

    DPRINT("Audio Service main()\n");
    if (!StartServiceCtrlDispatcherW(service_table))
        DPRINT("StartServiceCtrlDispatcher failed\n");

    return 0;
}
