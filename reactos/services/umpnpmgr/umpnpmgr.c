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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/umpnpmgr/umpnpmgr.c
 * PURPOSE:          User-mode Plug and Play manager
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <ndk/sysguid.h>
#include <ddk/wdmguid.h>
#include <ddk/cfgmgr32.h>

#include <rpc.h>
#include <rpcdce.h>

#include "pnp_c.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv);

static SERVICE_TABLE_ENTRY ServiceTable[2] =
{
    {TEXT("PlugPlay"), ServiceMain},
    {NULL, NULL}
};

static WCHAR szRootDeviceId[] = L"HTREE\\ROOT\\0";

static HKEY hEnumKey = NULL;
static HKEY hClassKey = NULL;

/* FUNCTIONS *****************************************************************/

static DWORD WINAPI
RpcServerThread(LPVOID lpParameter)
{
    RPC_STATUS Status;

    DPRINT("RpcServerThread() called\n");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    20,
                                    L"\\pipe\\umpnpmgr",
                                    NULL);  // Security descriptor
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(pnp_v1_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1,
                             20,
                             FALSE);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerListen() failed (Status %lx)\n", Status);
        return 0;
    }

    DPRINT("RpcServerThread() done\n");

    return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


CONFIGRET
PNP_GetVersion(handle_t BindingHandle,
               unsigned short *Version)
{
    *Version = 0x0400;
    return CR_SUCCESS;
}


CONFIGRET
PNP_GetGlobalState(handle_t BindingHandle,
                   unsigned long *State,
                   unsigned long Flags)
{
    *State = CM_GLOBAL_STATE_CAN_DO_UI | CM_GLOBAL_STATE_SERVICES_AVAILABLE;
    return CR_SUCCESS;
}


CONFIGRET
PNP_ValidateDeviceInstance(handle_t BindingHandle,
                           wchar_t *DeviceInstance,
                           unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hEnumKey = NULL;
    HKEY hDeviceKey = NULL;

    DPRINT("PNP_ValidateDeviceInstance(%S %lx) called\n",
           DeviceInstance, Flags);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Enum",
                      0,
                      KEY_ALL_ACCESS,
                      &hEnumKey))
    {
        DPRINT("Could not open the Enum Key!\n");
        ret = CR_FAILURE;
        goto Done;
    }

    if (RegOpenKeyExW(hEnumKey,
                      DeviceInstance,
                      0,
                      KEY_READ,
                      &hDeviceKey))
    {
        DPRINT("Could not open the Device Key!\n");
        ret = CR_NO_SUCH_DEVNODE;
        goto Done;
    }

    /* FIXME: add more tests */

Done:
    if (hDeviceKey != NULL)
        RegCloseKey(hDeviceKey);

    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);

    DPRINT("PNP_ValidateDeviceInstance() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_GetRootDeviceInstance(handle_t BindingHandle,
                          wchar_t *DeviceInstance,
                          unsigned long Length)
{
    CONFIGRET ret = CR_SUCCESS;

    DPRINT("PNP_GetRootDeviceInstance() called\n");

    if (Length < lstrlenW(szRootDeviceId) + 1)
    {
        ret = CR_BUFFER_SMALL;
        goto Done;
    }

    lstrcpyW(DeviceInstance,
             szRootDeviceId);

Done:
    DPRINT("PNP_GetRootDeviceInstance() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_GetRelatedDeviceInstance(handle_t BindingHandle,
                             unsigned long Relationship,
                             wchar_t *DeviceId,
                             wchar_t *RelatedDeviceId,
                             unsigned long Length,
                             unsigned long Flags)
{
    PLUGPLAY_CONTROL_RELATED_DEVICE_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT("PNP_GetRelatedDeviceInstance() called\n");
    DPRINT("  Relationship %ld\n", Relationship);
    DPRINT("  DeviceId %S\n", DeviceId);

    RtlInitUnicodeString(&PlugPlayData.TargetDeviceInstance,
                         DeviceId);

    PlugPlayData.Relation = Relationship;

    PlugPlayData.RelatedDeviceInstance.Length = 0;
    PlugPlayData.RelatedDeviceInstance.MaximumLength = Length;
    PlugPlayData.RelatedDeviceInstance.Buffer = RelatedDeviceId;

    Status = NtPlugPlayControl(PlugPlayControlGetRelatedDevice,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));
    if (!NT_SUCCESS(Status))
    {
        /* FIXME: Map Status to ret */
        ret = CR_FAILURE;
    }

    DPRINT("PNP_GetRelatedDeviceInstance() done (returns %lx)\n", ret);
    if (ret == CR_SUCCESS)
    {
        DPRINT("RelatedDevice: %wZ\n", &PlugPlayData.RelatedDeviceInstance);
    }

    return ret;
}


CONFIGRET
PNP_EnumerateSubKeys(handle_t BindingHandle,
                     unsigned long Branch,
                     unsigned long Index,
                     wchar_t *Buffer,
                     unsigned long Length,
                     unsigned long *RequiredLength,
                     DWORD Flags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hKey;
    DWORD dwError;

    DPRINT1("PNP_EnumerateSubKeys() called\n");

    switch (Branch)
    {
        case 1:
            hKey = hEnumKey;
            break;

        case 2:
            hKey = hClassKey;
            break;

        default:
            return CR_FAILURE;
    }

    *RequiredLength = Length;
    dwError = RegEnumKeyExW(hKey,
                            Index,
                            Buffer,
                            RequiredLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (dwError != ERROR_SUCCESS)
    {
        ret = CR_FAILURE;
    }

    DPRINT1("PNP_EnumerateSubKeys() done (returns %lx)\n", ret);
    if (ret == CR_SUCCESS)
    {
        DPRINT1("Sub key: %S\n", Buffer);
    }

    return ret;
}


CONFIGRET
PNP_GetDepth(handle_t BindingHandle,
             wchar_t *DeviceInstance,
             unsigned long *Depth,
             DWORD Flags)
{
    PLUGPLAY_CONTROL_DEPTH_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT("PNP_GetDepth() called\n");

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         DeviceInstance);

    Status = NtPlugPlayControl(PlugPlayControlGetDeviceDepth,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_DEPTH_DATA));
    if (NT_SUCCESS(Status))
    {
        *Depth = PlugPlayData.Depth;
    }
    else
    {
        ret = CR_FAILURE; /* FIXME */
    }

    DPRINT("PNP_GetDepth() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_SetDeviceRegProp(handle_t BindingHandle,
                     wchar_t *DeviceId,
                     unsigned long Property,
                     unsigned long DataType,
                     char *Buffer,
                     unsigned long Length,
                     unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hKey = 0;

    DPRINT("PNP_SetDeviceRegProp() called\n");

    DPRINT("DeviceId: %S\n", DeviceId);
    DPRINT("Property: %lu\n", Property);
    DPRINT("DataType: %lu\n", DataType);
    DPRINT("Length: %lu\n", Length);

    switch (Property)
    {
        case CM_DRP_DEVICEDESC:
            lpValueName = L"DeviceDesc";
            break;

        case CM_DRP_HARDWAREID:
            lpValueName = L"HardwareID";
            break;

        case CM_DRP_COMPATIBLEIDS:
            lpValueName = L"CompatibleIDs";
            break;

        case CM_DRP_SERVICE:
            lpValueName = L"Service";
            break;

        case CM_DRP_CLASS:
            lpValueName = L"Class";
            break;

        case CM_DRP_CLASSGUID:
            lpValueName = L"ClassGUID";
            break;

        case CM_DRP_DRIVER:
            lpValueName = L"Driver";
            break;

        case CM_DRP_CONFIGFLAGS:
            lpValueName = L"ConfigFlags";
            break;

        case CM_DRP_MFG:
            lpValueName = L"Mfg";
            break;

        case CM_DRP_FRIENDLYNAME:
            lpValueName = L"FriendlyName";
            break;

        case CM_DRP_LOCATION_INFORMATION:
            lpValueName = L"LocationInformation";
            break;

        case CM_DRP_UPPERFILTERS:
            lpValueName = L"UpperFilters";
            break;

        case CM_DRP_LOWERFILTERS:
            lpValueName = L"LowerFilters";
            break;

        default:
            return CR_INVALID_PROPERTY;
    }

    DPRINT("Value name: %S\n", lpValueName);

    if (RegOpenKeyExW(hEnumKey,
                      DeviceId,
                      0,
                      KEY_ALL_ACCESS,
                      &hKey))
        return CR_INVALID_DEVNODE;

    if (Length == 0)
    {
        if (RegDeleteValueW(hKey,
                            lpValueName))
            ret = CR_REGISTRY_ERROR;
    }
    else
    {
        if (RegSetValueExW(hKey,
                           lpValueName,
                           0,
                           DataType,
                           (const BYTE*)Buffer,
                           Length))
            ret = CR_REGISTRY_ERROR;
    }

    RegCloseKey(hKey);

    DPRINT("PNP_SetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_GetDeviceRegProp(handle_t BindingHandle,
                     wchar_t *DeviceInstance,
                     unsigned long Property,
                     unsigned long *DataType,
                     char *Buffer,
                     unsigned long *TransferLen,
                     unsigned long *Length,
                     DWORD Flags)
{
    PLUGPLAY_CONTROL_PROPERTY_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hKey = 0;
    NTSTATUS Status;

    DPRINT("PNP_GetDeviceRegProp() called\n");

    switch (Property)
    {
        case CM_DRP_DEVICEDESC:
            lpValueName = L"DeviceDesc";
            break;

        case CM_DRP_HARDWAREID:
            lpValueName = L"HardwareID";
            break;

        case CM_DRP_COMPATIBLEIDS:
            lpValueName = L"CompatibleIDs";
            break;

        case CM_DRP_SERVICE:
            lpValueName = L"Service";
            break;

        case CM_DRP_CLASS:
            lpValueName = L"Class";
            break;

        case CM_DRP_CLASSGUID:
            lpValueName = L"ClassGUID";
            break;

        case CM_DRP_DRIVER:
            lpValueName = L"Driver";
            break;

        case CM_DRP_CONFIGFLAGS:
            lpValueName = L"ConfigFlags";
            break;

        case CM_DRP_MFG:
            lpValueName = L"Mfg";
            break;

        case CM_DRP_FRIENDLYNAME:
            lpValueName = L"FriendlyName";
            break;

        case CM_DRP_LOCATION_INFORMATION:
            lpValueName = L"LocationInformation";
            break;

        case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:
            lpValueName = NULL;
            break;

        case CM_DRP_CAPABILITIES:
            lpValueName = L"Capabilities";
            break;

        case CM_DRP_UI_NUMBER:
            lpValueName = NULL;
            break;

        case CM_DRP_UPPERFILTERS:
            lpValueName = L"UpperFilters";
            break;

        case CM_DRP_LOWERFILTERS:
            lpValueName = L"LowerFilters";
            break;

        case CM_DRP_BUSTYPEGUID:
            lpValueName = NULL;
            break;

        case CM_DRP_LEGACYBUSTYPE:
            lpValueName = NULL;
            break;

        case CM_DRP_BUSNUMBER:
            lpValueName = NULL;
            break;

        case CM_DRP_ENUMERATOR_NAME:
            lpValueName = NULL;
            break;

        default:
            return CR_INVALID_PROPERTY;
    }

    DPRINT("Value name: %S\n", lpValueName);

    if (lpValueName)
    {
        /* Retrieve information from the Registry */
        if (RegOpenKeyExW(hEnumKey,
                          DeviceInstance,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey))
            return CR_INVALID_DEVNODE;

        if (RegQueryValueExW(hKey,
                             lpValueName,
                             NULL,
                             DataType,
                             (LPBYTE)Buffer,
                             Length))
            ret = CR_REGISTRY_ERROR;

        /* FIXME: Check buffer size */

        RegCloseKey(hKey);
    }
    else
    {
        /* Retrieve information from the Device Node */
        RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                             DeviceInstance);
        PlugPlayData.Buffer = Buffer;
        PlugPlayData.BufferSize = *TransferLen;

        switch (Property)
        {
#if 0
            case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:
                PlugPlayData.Property = DevicePropertyPhysicalDeviceObjectName;
                break;

            case CM_DRP_UI_NUMBER:
                PlugPlayData.Property = DevicePropertyUINumber;
                break;

            case CM_DRP_BUSTYPEGUID:
                PlugPlayData.Property = DevicePropertyBusTypeGuid;
                break;

            case CM_DRP_LEGACYBUSTYPE:
                PlugPlayData.Property = DevicePropertyLegacyBusType;
                break;

            case CM_DRP_BUSNUMBER:
                PlugPlayData.Property = DevicePropertyBusNumber;
                break;

            case CM_DRP_ENUMERATOR_NAME:
                PlugPlayData.Property = DevicePropertyEnumeratorName;
                break;
#endif

            default:
                return CR_INVALID_PROPERTY;
        }

        Status = NtPlugPlayControl(PlugPlayControlProperty,
                                   (PVOID)&PlugPlayData,
                                   sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA));
        if (NT_SUCCESS(Status))
        {
            *Length = PlugPlayData.BufferSize;
        }
        else
        {
            ret = CR_FAILURE; /* FIXME */
        }
    }

    DPRINT("PNP_GetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_GetDeviceStatus(handle_t BindingHandle,
                    wchar_t *DeviceInstance,
                    unsigned long *pStatus,
                    unsigned long *pProblem,
                    DWORD Flags)
{
    PLUGPLAY_CONTROL_STATUS_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT("PNP_GetDeviceStatus() called\n");

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         DeviceInstance);
    PlugPlayData.Operation = 0; /* Get status */

    Status = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    if (NT_SUCCESS(Status))
    {
        *pStatus = PlugPlayData.DeviceStatus;
        *pProblem = PlugPlayData.DeviceProblem;
    }
    else
    {
        ret = CR_FAILURE; /* FIXME */
    }

    DPRINT("PNP_GetDeviceStatus() done (returns %lx)\n", ret);

    return ret;
}


CONFIGRET
PNP_SetDeviceProblem(handle_t BindingHandle,
                     wchar_t *DeviceInstance,
                     unsigned long Problem,
                     DWORD Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    DPRINT1("PNP_SetDeviceProblem() called\n");

    /* FIXME */

    DPRINT1("PNP_SetDeviceProblem() done (returns %lx)\n", ret);

    return ret;
}


static DWORD WINAPI
PnpEventThread(LPVOID lpParameter)
{
    PPLUGPLAY_EVENT_BLOCK PnpEvent;
    ULONG PnpEventSize;
    NTSTATUS Status;
    RPC_STATUS RpcStatus;

    PnpEventSize = 0x1000;
    PnpEvent = HeapAlloc(GetProcessHeap(), 0, PnpEventSize);
    if (PnpEvent == NULL)
        return ERROR_OUTOFMEMORY;

    for (;;)
    {
        DPRINT("Calling NtGetPlugPlayEvent()\n");

        /* Wait for the next pnp event */
        Status = NtGetPlugPlayEvent(0, 0, PnpEvent, PnpEventSize);

        /* Resize the buffer for the PnP event if it's too small. */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PnpEventSize += 0x400;
            PnpEvent = HeapReAlloc(GetProcessHeap(), 0, PnpEvent, PnpEventSize);
            if (PnpEvent == NULL)
                return ERROR_OUTOFMEMORY;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtPlugPlayEvent() failed (Status %lx)\n", Status);
            break;
        }

        DPRINT("Received PnP Event\n");
        if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ARRIVAL, &RpcStatus))
        {
            DPRINT1("Device arrival event: %S\n", PnpEvent->TargetDevice.DeviceIds);
        }
        else
        {
            DPRINT1("Unknown event\n");
        }

        /* FIXME: Process the pnp event */

        /* Dequeue the current pnp event and signal the next one */
        NtPlugPlayControl(PlugPlayControlUserResponse, NULL, 0);
    }

    HeapFree(GetProcessHeap(), 0, PnpEvent);

    return ERROR_SUCCESS;
}


static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{
    HANDLE hThread;
    DWORD dwThreadId;

    DPRINT("ServiceMain() called\n");

    hThread = CreateThread(NULL,
                           0,
                           PnpEventThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    hThread = CreateThread(NULL,
                           0,
                           RpcServerThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    DPRINT("ServiceMain() done\n");
}


int
main(int argc, char *argv[])
{
    DWORD dwError;

    DPRINT("Umpnpmgr: main() started\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Enum",
                            0,
                            KEY_ALL_ACCESS,
                            &hEnumKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Could not open the Enum Key! (Error %lu)\n", dwError);
        return dwError;
    }

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Control\\Class",
                            0,
                            KEY_ALL_ACCESS,
                            &hClassKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Could not open the Class Key! (Error %lu)\n", dwError);
        return dwError;
    }

    StartServiceCtrlDispatcher(ServiceTable);

    DPRINT("Umpnpmgr: main() done\n");

    ExitThread(0);

    return 0;
}

/* EOF */
