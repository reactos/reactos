/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             base/services/umpnpmgr/event.c
 * PURPOSE:          PNP Event thread
 * PROGRAMMER:       Eric Kohl (eric.kohl@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Colin Finck (colin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static
VOID
ProcessTargetDeviceEvent(
    _In_ PPLUGPLAY_EVENT_BLOCK PnpEvent)
{
    RPC_STATUS RpcStatus;

    DPRINT("ProcessTargetDeviceEvent(%p)\n", PnpEvent);

    if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ARRIVAL, &RpcStatus))
    {
//        DWORD dwRecipient;

        DPRINT("Device arrival: %S\n", PnpEvent->TargetDevice.DeviceIds);

//       dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//       BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                               &dwRecipient,
//                               WM_DEVICECHANGE,
//                               DBT_DEVNODES_CHANGED,
//                               0);
        SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_EJECT_VETOED, &RpcStatus))
    {
        DPRINT1("Eject vetoed: %S\n", PnpEvent->TargetDevice.DeviceIds);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_KERNEL_INITIATED_EJECT, &RpcStatus))
    {
        DPRINT1("Kernel initiated eject: %S\n", PnpEvent->TargetDevice.DeviceIds);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_SAFE_REMOVAL, &RpcStatus))
    {
//        DWORD dwRecipient;

        DPRINT1("Safe removal: %S\n", PnpEvent->TargetDevice.DeviceIds);

//        dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//        BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                                &dwRecipient,
//                                WM_DEVICECHANGE,
//                                DBT_DEVNODES_CHANGED,
//                                0);
        SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_SURPRISE_REMOVAL, &RpcStatus))
    {
//        DWORD dwRecipient;

        DPRINT1("Surprise removal: %S\n", PnpEvent->TargetDevice.DeviceIds);

//        dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//        BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                                &dwRecipient,
//                                WM_DEVICECHANGE,
//                                DBT_DEVNODES_CHANGED,
//                                0);
        SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_REMOVAL_VETOED, &RpcStatus))
    {
        DPRINT1("Removal vetoed: %S\n", PnpEvent->TargetDevice.DeviceIds);
    }
    else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_REMOVE_PENDING, &RpcStatus))
    {
        DPRINT1("Removal pending: %S\n", PnpEvent->TargetDevice.DeviceIds);
    }
    else
    {
        DPRINT1("Unknown event, GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                PnpEvent->EventGuid.Data1, PnpEvent->EventGuid.Data2, PnpEvent->EventGuid.Data3,
                PnpEvent->EventGuid.Data4[0], PnpEvent->EventGuid.Data4[1], PnpEvent->EventGuid.Data4[2],
                PnpEvent->EventGuid.Data4[3], PnpEvent->EventGuid.Data4[4], PnpEvent->EventGuid.Data4[5],
                PnpEvent->EventGuid.Data4[6], PnpEvent->EventGuid.Data4[7]);
    }
}


static
VOID
ProcessDeviceClassChangeEvent(
    _In_ PPLUGPLAY_EVENT_BLOCK PnpEvent)
{
    DPRINT("ProcessDeviceClassChangeEvent(%p)\n", PnpEvent);
    DPRINT("SymbolicLink: %S\n", PnpEvent->DeviceClass.SymbolicLinkName);
    DPRINT("ClassGuid: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
           PnpEvent->DeviceClass.ClassGuid.Data1, PnpEvent->DeviceClass.ClassGuid.Data2, PnpEvent->DeviceClass.ClassGuid.Data3,
           PnpEvent->DeviceClass.ClassGuid.Data4[0], PnpEvent->DeviceClass.ClassGuid.Data4[1], PnpEvent->DeviceClass.ClassGuid.Data4[2],
           PnpEvent->DeviceClass.ClassGuid.Data4[3], PnpEvent->DeviceClass.ClassGuid.Data4[4], PnpEvent->DeviceClass.ClassGuid.Data4[5],
           PnpEvent->DeviceClass.ClassGuid.Data4[6], PnpEvent->DeviceClass.ClassGuid.Data4[7]);
}


static
VOID
ProcessDeviceInstallEvent(
    _In_ PPLUGPLAY_EVENT_BLOCK PnpEvent)
{
    DeviceInstallParams* Params;
    DWORD len;
    DWORD DeviceIdLength;

    DPRINT("ProcessDeviceInstallEvent(%p)\n", PnpEvent);
    DPRINT("Device enumerated: %S\n", PnpEvent->InstallDevice.DeviceId);

    DeviceIdLength = lstrlenW(PnpEvent->InstallDevice.DeviceId);
    if (DeviceIdLength)
    {
        /* Allocate a new device-install event */
        len = FIELD_OFFSET(DeviceInstallParams, DeviceIds) + (DeviceIdLength + 1) * sizeof(WCHAR);
        Params = HeapAlloc(GetProcessHeap(), 0, len);
        if (Params)
        {
            wcscpy(Params->DeviceIds, PnpEvent->InstallDevice.DeviceId);

            /* Queue the event (will be dequeued by DeviceInstallThread) */
            WaitForSingleObject(hDeviceInstallListMutex, INFINITE);
            InsertTailList(&DeviceInstallListHead, &Params->ListEntry);
            ReleaseMutex(hDeviceInstallListMutex);

            SetEvent(hDeviceInstallListNotEmpty);
        }
    }
}


DWORD
WINAPI
PnpEventThread(
    _In_ LPVOID lpParameter)
{
    PLUGPLAY_CONTROL_USER_RESPONSE_DATA ResponseData = {0, 0, 0, 0};
    DWORD dwRet = ERROR_SUCCESS;
    NTSTATUS Status;
    PPLUGPLAY_EVENT_BLOCK PnpEvent, NewPnpEvent;
    ULONG PnpEventSize;

    UNREFERENCED_PARAMETER(lpParameter);

    PnpEventSize = 0x1000;
    PnpEvent = HeapAlloc(GetProcessHeap(), 0, PnpEventSize);
    if (PnpEvent == NULL)
        return ERROR_OUTOFMEMORY;

    for (;;)
    {
        DPRINT("Calling NtGetPlugPlayEvent()\n");

        /* Wait for the next PnP event */
        Status = NtGetPlugPlayEvent(0, 0, PnpEvent, PnpEventSize);

        /* Resize the buffer for the PnP event if it's too small */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PnpEventSize += 0x400;
            NewPnpEvent = HeapReAlloc(GetProcessHeap(), 0, PnpEvent, PnpEventSize);
            if (NewPnpEvent == NULL)
            {
                dwRet = ERROR_OUTOFMEMORY;
                break;
            }
            PnpEvent = NewPnpEvent;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtGetPlugPlayEvent() failed (Status 0x%08lx)\n", Status);
            break;
        }

        /* Process the PnP event */
        DPRINT("Received PnP Event\n");
        switch (PnpEvent->EventCategory)
        {
//            case HardwareProfileChangeEvent:

            case TargetDeviceChangeEvent:
                ProcessTargetDeviceEvent(PnpEvent);
                break;

            case DeviceClassChangeEvent:
                ProcessDeviceClassChangeEvent(PnpEvent);
                break;

//            case CustomDeviceEvent:

            case DeviceInstallEvent:
                ProcessDeviceInstallEvent(PnpEvent);
                break;

//            case DeviceArrivalEvent:
//            case PowerEvent:
//            case VetoEvent:
//            case BlockedDriverEvent:

            default:
                DPRINT1("Unsupported Event Category: %lu\n", PnpEvent->EventCategory);
                break;
        }

        /* Dequeue the current PnP event and signal the next one */
        Status = NtPlugPlayControl(PlugPlayControlUserResponse,
                                   &ResponseData,
                                   sizeof(ResponseData));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtPlugPlayControl(PlugPlayControlUserResponse) failed (Status 0x%08lx)\n", Status);
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, PnpEvent);

    return dwRet;
}

/* EOF */
