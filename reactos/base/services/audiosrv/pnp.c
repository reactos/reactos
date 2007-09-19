/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/pnp.c
 * PURPOSE:          Audio Service Plug and Play
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>
#include <winuser.h>
#include <dbt.h>
#include <setupapi.h>

#include <ks.h>
#include <ksmedia.h>

#include <audiosrv/audiosrv.h>
#include "audiosrv.h"

static HDEVNOTIFY device_notification_handle = NULL;


/*
    Finds all devices within the KSCATEGORY_AUDIO category and puts them
    in the shared device list.
*/

BOOL
ProcessExistingDevices()
{
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVINFO_DATA device_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data;
    HDEVINFO dev_info;
    DWORD length;
    int index = 0;

    const GUID category_guid = {STATIC_KSCATEGORY_AUDIO};

    dev_info = SetupDiGetClassDevsEx(&category_guid,
                                     NULL,
                                     NULL,
                                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
                                     NULL,
                                     NULL,
                                     NULL);

/*    printf("%s:\n", ClassString); */

    interface_data.cbSize = sizeof(interface_data);
    interface_data.Reserved = 0;

    /* Enumerate the devices within the category */
    index = 0;

    length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)
                + (MAX_PATH * sizeof(WCHAR));

    detail_data =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(),
                                                    0,
                                                    length);

    while ( 
    SetupDiEnumDeviceInterfaces(dev_info,
                                NULL,
                                &category_guid,
                                index,
                                &interface_data) )
    {
        PnP_AudioDevice* list_node;

        ZeroMemory(detail_data, length);

        /* NOTE: We don't actually use device_data... */
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        device_data.cbSize = sizeof(device_data);
        device_data.Reserved = 0;
        SetupDiGetDeviceInterfaceDetail(dev_info,
                                        &interface_data,
                                        detail_data,
                                        length,
                                        NULL,
                                        &device_data);

        list_node = CreateDeviceDescriptor(detail_data->DevicePath, TRUE);
        AppendAudioDeviceToList(list_node);
        DestroyDeviceDescriptor(list_node);

        /* TODO: Cleanup the device we enumerated? */

        index ++;
    };

    HeapFree(GetProcessHeap(), 0, detail_data);

    SetupDiDestroyDeviceInfoList(dev_info);

    return TRUE;
}


/*
    Add new devices to the list as they arrive.
*/

DWORD
ProcessDeviceArrival(DEV_BROADCAST_DEVICEINTERFACE* device)
{
    PnP_AudioDevice* list_node;
    list_node = CreateDeviceDescriptor(device->dbcc_name, TRUE);
    AppendAudioDeviceToList(list_node);
    DestroyDeviceDescriptor(list_node);

    return NO_ERROR;
}


/*
    Request notification of device additions/removals.
*/

BOOL
RegisterForDeviceNotifications()
{
    DEV_BROADCAST_DEVICEINTERFACE notification_filter;

    const GUID wdmaud_guid = {STATIC_KSCATEGORY_AUDIO};

    ZeroMemory(&notification_filter, sizeof(notification_filter));
    notification_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notification_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notification_filter.dbcc_classguid = wdmaud_guid;

    device_notification_handle =
        RegisterDeviceNotification((HANDLE) service_status_handle,
                                   &notification_filter,
                                   DEVICE_NOTIFY_SERVICE_HANDLE
/* |
                                   DEVICE_NOTIFY_ALL_INTERFACE_CLASSES*/);

    if ( ! device_notification_handle )
    {
        logmsg("RegisterDeviceNotification() failed with error %d\n", GetLastError());
    }

    return ( device_notification_handle != NULL );
}


/*
    When we're not interested in device notifications any more, this gets
    called.
*/

VOID UnregisterDeviceNotifications()
{
    /* TODO -- NOT IMPLEMENTED! */

    if ( device_notification_handle )
    {
        /* TODO */
        device_notification_handle = NULL;
    }
}


/*
    Device events from the main service handler get passed to this.
*/

DWORD
HandleDeviceEvent(
    DWORD dwEventType,
    LPVOID lpEventData)
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
