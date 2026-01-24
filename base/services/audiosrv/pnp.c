/*
 * PROJECT:     ReactOS
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Audio Service Plug and Play
 * COPYRIGHT:   Copyright 2007 Andrew Greenwood <silverblade@reactos.org>
 *              Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "audiosrv.h"

#include <winreg.h>
#include <winuser.h>
#include <mmsystem.h>
#include <setupapi.h>
#include <ks.h>
#include <ksmedia.h>

#define NDEBUG
#include <debug.h>

static HDEVNOTIFY device_notification_handle = NULL;

/*
    Finds all devices within the KSCATEGORY_AUDIO category and puts them
    in the shared device list.
*/

BOOL
ProcessExistingDevices(VOID)
{
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVINFO_DATA device_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail_data;
    HDEVINFO dev_info;
    DWORD length;
    int index = 0;

    const GUID category_guid = {STATIC_KSCATEGORY_AUDIO};

    dev_info = SetupDiGetClassDevsExW(&category_guid,
                                      NULL,
                                      NULL,
                                      DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
                                      NULL,
                                      NULL,
                                      NULL);

    interface_data.cbSize = sizeof(interface_data);
    interface_data.Reserved = 0;

    /* Enumerate the devices within the category */
    index = 0;

    length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)
                + (MAX_PATH * sizeof(WCHAR));

    detail_data =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(),
                                                      0,
                                                      length);

    if ( ! detail_data )
    {
        DPRINT("failed to allocate detail_data\n");
        return TRUE;
    }

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
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        device_data.cbSize = sizeof(device_data);
        device_data.Reserved = 0;
        SetupDiGetDeviceInterfaceDetailW(dev_info,
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
RegisterForDeviceNotifications(VOID)
{
    DEV_BROADCAST_DEVICEINTERFACE notification_filter;

    const GUID wdmaud_guid = {STATIC_KSCATEGORY_AUDIO};

    ZeroMemory(&notification_filter, sizeof(notification_filter));
    notification_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notification_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notification_filter.dbcc_classguid = wdmaud_guid;

    device_notification_handle =
        RegisterDeviceNotificationW((HANDLE) service_status_handle,
                                    &notification_filter,
                                    DEVICE_NOTIFY_SERVICE_HANDLE);
    if (!device_notification_handle)
    {
        DPRINT("failed with error %d\n", GetLastError());
    }

    return ( device_notification_handle != NULL );
}


/*
    When we're not interested in device notifications any more, this gets
    called.
*/

VOID
UnregisterDeviceNotifications(VOID)
{
    if (device_notification_handle)
    {
        if (UnregisterDeviceNotification(device_notification_handle))
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
    switch (dwEventType)
    {
        case DBT_DEVICEARRIVAL:
        {
            DEV_BROADCAST_DEVICEINTERFACE* incoming_device =
                (DEV_BROADCAST_DEVICEINTERFACE*)lpEventData;

            return ProcessDeviceArrival(incoming_device);
        }

        default :
        {
            break;
        }
    }

    return NO_ERROR;
}
