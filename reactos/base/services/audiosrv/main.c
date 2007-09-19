/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/main.c
 * PURPOSE:          Audio Service
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>
#include <winuser.h>
#include <dbt.h>
#include <audiosrv/audiosrv.h>
#include <pnp_list_manager.h>

#include <ksmedia.h>



/* Prototypes */

VOID CALLBACK
ServiceMain(DWORD argc, char** argv);

DWORD WINAPI
ServiceControlHandler(
    DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext);

/* Service table */
SERVICE_TABLE_ENTRY service_table[2] =
{
    { L"AudioSrv", (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;
HDEVNOTIFY device_notification_handle = NULL;

/* Synchronization of access to the event list */
HANDLE device_list_mutex = INVALID_HANDLE_VALUE;


/* Implementation */

DWORD
ProcessDeviceArrival(DEV_BROADCAST_DEVICEINTERFACE* device)
{
    PnP_AudioDevice* list_node;
    list_node = CreateDeviceDescriptor(device->dbcc_name, TRUE);
    AppendAudioDeviceToList(list_node);
    DestroyDeviceDescriptor(list_node);

    return NO_ERROR;
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
            return NO_ERROR;
        }

        case SERVICE_CONTROL_STOP :
        case SERVICE_CONTROL_SHUTDOWN :
        {
            /* FIXME: This function doesn't exist?! */
/*
            UnregisterDeviceNotification(device_notification_handle);
            device_notification_handle = NULL;
*/

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
            switch ( dwEventType )
            {
                case DBT_DEVICEARRIVAL :
                {
                    DEV_BROADCAST_DEVICEINTERFACE* incoming_device =
                        (DEV_BROADCAST_DEVICEINTERFACE*) lpEventData;

                    return ProcessDeviceArrival(incoming_device);
                }

                default :
                {
                    break;
                }
            }

            return NO_ERROR;
        }

        default :
            return ERROR_CALL_NOT_IMPLEMENTED;
    };

    /*SetServiceStatus(service_status_handle, &service_status);*/
}

BOOL
RegisterForDeviceNotifications()
{
    DEV_BROADCAST_DEVICEINTERFACE notification_filter;

    const GUID wdmaud_guid = {STATIC_KSCATEGORY_WDMAUD};

    /* FIXME: This currently lists ALL device interfaces... */
    ZeroMemory(&notification_filter, sizeof(notification_filter));
    notification_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notification_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notification_filter.dbcc_classguid = wdmaud_guid;

    device_notification_handle =
        RegisterDeviceNotification((HANDLE) service_status_handle,
                                   &notification_filter,
                                   DEVICE_NOTIFY_SERVICE_HANDLE |
                                   DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    return ( device_notification_handle != NULL );
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
