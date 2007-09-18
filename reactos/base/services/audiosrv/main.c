/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/main.c
 * PURPOSE:          Audio Service
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>

/* This is currently set to avoid conflicting service names in Windows! */
#define SERVICE_NAME                "RosAudioSrv"

/* A named mutex is used for synchronizing access to the device list.
   If this mutex doesn't exist, it means the audio service isn't running. */
#define DEVICE_LIST_MUTEX_NAME      "Global\\AudioDeviceListSync"

/* ...and this is where the device list will be available */
#define DEVICE_LIST_MAPPED_FILENAME "Global\\AudioDeviceList"



/* Prototypes */

VOID CALLBACK
ServiceMain(DWORD argc, char** argv);

VOID WINAPI
ServiceControlHandler(DWORD request);


/* Service table */
SERVICE_TABLE_ENTRY service_table[2] =
{
    { "AudioSrv", (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;

/* Synchronization of access to the event list */
HANDLE device_list_mutex = INVALID_HANDLE_VALUE;


/* Implementation */

VOID WINAPI
ServiceControlHandler(DWORD request)
{
    switch ( request )
    {
        case SERVICE_CONTROL_STOP :
        case SERVICE_CONTROL_SHUTDOWN :
        {
            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(service_status_handle, &service_status);

            CloseHandle(device_list_mutex);

            service_status.dwWin32ExitCode = 0;
            service_status.dwCurrentState = SERVICE_STOPPED;

            SetServiceStatus(service_status_handle, &service_status);

            return;
        }


        default :
            break;
    };

    SetServiceStatus(service_status_handle, &service_status);
}

VOID CALLBACK
ServiceMain(DWORD argc, char** argv)
{
    service_status_handle = RegisterServiceCtrlHandler(SERVICE_NAME,
                                                       ServiceControlHandler);

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

    /* We should create the mapped section here along with the mutex to
       sync access to the device list... */

    device_list_mutex = CreateMutex(NULL, FALSE, DEVICE_LIST_MUTEX_NAME);

    if ( ! device_list_mutex)
    {
        service_status.dwCurrentState = SERVICE_STOPPED;
        service_status.dwWin32ExitCode = -1;    // ok?
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
}
