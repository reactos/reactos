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
 *                   Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/
//#define HAVE_SLIST_ENTRY_IMPLEMENTED
#define WIN32_NO_STATUS
#include <windows.h>
#include <cmtypes.h>
#include <cmfuncs.h>
#include <rtlfuncs.h>
#include <umpnpmgr/sysguid.h>
#include <wdmguid.h>
#include <cfgmgr32.h>
#include <regstr.h>

#include <rpc.h>
#include <rpcdce.h>

#include "pnp_s.h"

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

static HANDLE hUserToken = NULL;
static HANDLE hInstallEvent = NULL;
static HANDLE hNoPendingInstalls = NULL;

#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
static SLIST_HEADER DeviceInstallListHead;
#else
static LIST_ENTRY DeviceInstallListHead;
#endif
static HANDLE hDeviceInstallListNotEmpty;

typedef struct
{
#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
    SLIST_ENTRY ListEntry;
#else
    LIST_ENTRY ListEntry;
#endif
    WCHAR DeviceIds[1];
} DeviceInstallParams;

/* FUNCTIONS *****************************************************************/

static DWORD WINAPI
RpcServerThread(LPVOID lpParameter)
{
    RPC_STATUS Status;

    UNREFERENCED_PARAMETER(lpParameter);

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

    /* ROS HACK (this should never happen...) */
    DPRINT1("*** Other devices won't be installed correctly. If something\n");
    DPRINT1("*** doesn't work, try to reboot to get a new chance.\n");

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


static CONFIGRET WINAPI
NtStatusToCrError(NTSTATUS Status)
{
    switch (Status)
    {
        case STATUS_NO_SUCH_DEVICE:
            return CR_NO_SUCH_DEVINST;

        default:
            /* FIXME: add more mappings */
            DPRINT1("Unable to map status 0x%08lx\n", Status);
            return CR_FAILURE;
    }
}


/* Function 0 */
DWORD PNP_Disconnect(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 1 */
DWORD PNP_Connect(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 2 */
DWORD PNP_GetVersion(
    handle_t hBinding,
    WORD *pVersion)
{
    UNREFERENCED_PARAMETER(hBinding);

    *pVersion = 0x0400;
    return CR_SUCCESS;
}


/* Function 3 */
DWORD PNP_GetGlobalState(
    handle_t hBinding,
    DWORD *pulState,
    DWORD ulFlags)
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    *pulState = CM_GLOBAL_STATE_CAN_DO_UI | CM_GLOBAL_STATE_SERVICES_AVAILABLE;
    return CR_SUCCESS;
}


/* Function 4 */
DWORD PNP_InitDetection(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_InitDetection() called\n");
    return CR_SUCCESS;
}


/* Function 5 */
DWORD PNP_ReportLogOn(
    handle_t hBinding,
    BOOL Admin,
    DWORD ProcessId)
{
    HANDLE hProcess;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(Admin);

    DPRINT("PNP_ReportLogOn(%u, %u) called\n", Admin, ProcessId);

    if (hInstallEvent != NULL)
        SetEvent(hInstallEvent);

    /* Get the users token */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS,
                           TRUE,
                           ProcessId);
    if (hProcess != NULL)
    {
        if (hUserToken != NULL)
        {
            CloseHandle(hUserToken);
            hUserToken = NULL;
        }

        OpenProcessToken(hProcess,
                         TOKEN_ALL_ACCESS,
                         &hUserToken);
        CloseHandle(hProcess);
    }

    /* Trigger the installer thread */
    /*if (hInstallEvent != NULL)
        SetEvent(hInstallEvent);*/

    return CR_SUCCESS;
}


/* Function 6 */
DWORD PNP_ValidateDeviceInstance(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hDeviceKey = NULL;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_ValidateDeviceInstance(%S %lx) called\n",
           pDeviceID, ulFlags);

    if (RegOpenKeyExW(hEnumKey,
                      pDeviceID,
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

    DPRINT("PNP_ValidateDeviceInstance() done (returns %lx)\n", ret);

    return ret;
}


/* Function 7 */
DWORD PNP_GetRootDeviceInstance(
    handle_t hBinding,
    LPWSTR pDeviceID,
    PNP_RPC_STRING_LEN ulLength)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetRootDeviceInstance() called\n");

    if (!pDeviceID)
    {
        ret = CR_INVALID_POINTER;
        goto Done;
    }
    if (ulLength < lstrlenW(szRootDeviceId) + 1)
    {
        ret = CR_BUFFER_SMALL;
        goto Done;
    }

    lstrcpyW(pDeviceID,
             szRootDeviceId);

Done:
    DPRINT("PNP_GetRootDeviceInstance() done (returns %lx)\n", ret);

    return ret;
}


/* Function 8 */
DWORD PNP_GetRelatedDeviceInstance(
    handle_t hBinding,
    DWORD ulRelationship,
    LPWSTR pDeviceID,
    LPWSTR pRelatedDeviceId,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    PLUGPLAY_CONTROL_RELATED_DEVICE_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetRelatedDeviceInstance() called\n");
    DPRINT("  Relationship %ld\n", ulRelationship);
    DPRINT("  DeviceId %S\n", pDeviceID);

    RtlInitUnicodeString(&PlugPlayData.TargetDeviceInstance,
                         pDeviceID);

    PlugPlayData.Relation = ulRelationship;

    PlugPlayData.RelatedDeviceInstanceLength = *pulLength;
    PlugPlayData.RelatedDeviceInstance = pRelatedDeviceId;

    Status = NtPlugPlayControl(PlugPlayControlGetRelatedDevice,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));
    if (!NT_SUCCESS(Status))
    {
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetRelatedDeviceInstance() done (returns %lx)\n", ret);
    if (ret == CR_SUCCESS)
    {
        DPRINT("RelatedDevice: %wZ\n", &PlugPlayData.RelatedDeviceInstance);
    }

    return ret;
}


/* Function 9 */
DWORD PNP_EnumerateSubKeys(
    handle_t hBinding,
    DWORD ulBranch,
    DWORD ulIndex,
    LPWSTR Buffer,
    PNP_RPC_STRING_LEN ulLength,
    PNP_RPC_STRING_LEN *pulRequiredLen,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hKey;
    DWORD dwError;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_EnumerateSubKeys() called\n");

    switch (ulBranch)
    {
        case PNP_ENUMERATOR_SUBKEYS:
            hKey = hEnumKey;
            break;

        case PNP_CLASS_SUBKEYS:
            hKey = hClassKey;
            break;

        default:
            return CR_FAILURE;
    }

    *pulRequiredLen = ulLength;
    dwError = RegEnumKeyExW(hKey,
                            ulIndex,
                            Buffer,
                            pulRequiredLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (dwError != ERROR_SUCCESS)
    {
        ret = (dwError == ERROR_NO_MORE_ITEMS) ? CR_NO_SUCH_VALUE : CR_FAILURE;
    }
    else
    {
        (*pulRequiredLen)++;
    }

    DPRINT("PNP_EnumerateSubKeys() done (returns %lx)\n", ret);

    return ret;
}


/* Function 10 */
DWORD PNP_GetDeviceList(
    handle_t hBinding,
    LPWSTR pszFilter,
    LPWSTR Buffer,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 11 */
DWORD PNP_GetDeviceListSize(
    handle_t hBinding,
    LPWSTR pszFilter,
    PNP_RPC_BUFFER_SIZE *pulLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 12 */
DWORD PNP_GetDepth(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    DWORD *pulDepth,
    DWORD ulFlags)
{
    PLUGPLAY_CONTROL_DEPTH_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetDepth() called\n");

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pszDeviceID);

    Status = NtPlugPlayControl(PlugPlayControlGetDeviceDepth,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_DEPTH_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulDepth = PlugPlayData.Depth;
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetDepth() done (returns %lx)\n", ret);

    return ret;
}


/* Function 13 */
DWORD PNP_GetDeviceRegProp(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulProperty,
    DWORD *pulRegDataType,
    BYTE *Buffer,
    PNP_PROP_SIZE *pulTransferLen,
    PNP_PROP_SIZE *pulLength,
    DWORD ulFlags)
{
    PLUGPLAY_CONTROL_PROPERTY_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hKey = 0;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetDeviceRegProp() called\n");

    switch (ulProperty)
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
                          pDeviceID,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey))
            return CR_INVALID_DEVNODE;

        if (RegQueryValueExW(hKey,
                             lpValueName,
                             NULL,
                             pulRegDataType,
                             Buffer,
                             pulLength))
            ret = CR_REGISTRY_ERROR;

        /* FIXME: Check buffer size */

        RegCloseKey(hKey);
    }
    else
    {
        /* Retrieve information from the Device Node */
        RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                             pDeviceID);
        PlugPlayData.Buffer = Buffer;
        PlugPlayData.BufferSize = *pulTransferLen;

        switch (ulProperty)
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
            *pulLength = PlugPlayData.BufferSize;
        }
        else
        {
            ret = NtStatusToCrError(Status);
        }
    }

    DPRINT("PNP_GetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 14 */
DWORD PNP_SetDeviceRegProp(
    handle_t hBinding,
    LPWSTR pDeviceId,
    DWORD ulProperty,
    DWORD ulDataType,
    BYTE *Buffer,
    PNP_PROP_SIZE ulLength,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hKey = 0;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_SetDeviceRegProp() called\n");

    DPRINT("DeviceId: %S\n", pDeviceId);
    DPRINT("Property: %lu\n", ulProperty);
    DPRINT("DataType: %lu\n", ulDataType);
    DPRINT("Length: %lu\n", ulLength);

    switch (ulProperty)
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
                      pDeviceId,
                      0,
                      KEY_ALL_ACCESS, /* FIXME: so much? */
                      &hKey))
        return CR_INVALID_DEVNODE;

    if (ulLength == 0)
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
                           ulDataType,
                           Buffer,
                           ulLength))
            ret = CR_REGISTRY_ERROR;
    }

    RegCloseKey(hKey);

    DPRINT("PNP_SetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 15 */
DWORD PNP_GetClassInstance(
    handle_t hBinding,
    LPWSTR pDeviceId,
    LPWSTR pszClassInstance,
    PNP_RPC_STRING_LEN ulLength)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 16 */
DWORD PNP_CreateKey(
    handle_t hBinding,
    LPWSTR pszSubKey,
    DWORD samDesired,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 17 */
DWORD PNP_DeleteRegistryKey(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    LPWSTR pszParentKey,
    LPWSTR pszChildKey,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 18 */
DWORD PNP_GetClassCount(
    handle_t hBinding,
    DWORD *pulClassCount,
    DWORD ulFlags)
{
    HKEY hKey;
    DWORD dwError;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            REGSTR_PATH_CLASS,
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return CR_INVALID_DATA;

    dwError = RegQueryInfoKeyW(hKey,
                               NULL,
                               NULL,
                               NULL,
                               pulClassCount,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS)
        return CR_INVALID_DATA;

    return CR_SUCCESS;
}


/* Function 19 */
DWORD PNP_GetClassName(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    LPWSTR Buffer,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    WCHAR szKeyName[MAX_PATH];
    CONFIGRET ret = CR_SUCCESS;
    HKEY hKey;
    DWORD dwSize;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetClassName() called\n");

    lstrcpyW(szKeyName, L"System\\CurrentControlSet\\Control\\Class\\");
    if(lstrlenW(pszClassGuid) + 1 < sizeof(szKeyName)/sizeof(WCHAR)-(lstrlenW(szKeyName) * sizeof(WCHAR)))
        lstrcatW(szKeyName, pszClassGuid);
    else return CR_INVALID_DATA;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      szKeyName,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey))
        return CR_REGISTRY_ERROR;

    dwSize = *pulLength * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
                         L"Class",
                         NULL,
                         NULL,
                         (LPBYTE)Buffer,
                         &dwSize))
    {
        *pulLength = 0;
        ret = CR_REGISTRY_ERROR;
    }
    else
    {
        *pulLength = dwSize / sizeof(WCHAR);
    }

    RegCloseKey(hKey);

    DPRINT("PNP_GetClassName() done (returns %lx)\n", ret);

    return ret;
}


/* Function 20 */
DWORD PNP_DeleteClassKey(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetClassName(%S, %lx) called\n", pszClassGuid, ulFlags);

    if (ulFlags & CM_DELETE_CLASS_SUBKEYS)
    {
        if (RegDeleteTreeW(hClassKey, pszClassGuid) != ERROR_SUCCESS)
            ret = CR_REGISTRY_ERROR;
    }
    else
    {
        if (RegDeleteKeyW(hClassKey, pszClassGuid) != ERROR_SUCCESS)
            ret = CR_REGISTRY_ERROR;
    }

    DPRINT("PNP_DeleteClassKey() done (returns %lx)\n", ret);

    return ret;
}


/* Function 21 */
DWORD PNP_GetInterfaceDeviceAlias(
    handle_t hBinding,
    LPWSTR pszInterfaceDevice,
    GUID *AliasInterfaceGuid,
    LPWSTR pszAliasInterfaceDevice,
    PNP_RPC_STRING_LEN *pulLength,
    PNP_RPC_STRING_LEN *pulTransferLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 22 */
DWORD PNP_GetInterfaceDeviceList(
    handle_t hBinding,
    GUID *InterfaceGuid,
    LPWSTR pszDeviceID,
    BYTE *Buffer,
    PNP_RPC_BUFFER_SIZE *pulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 23 */
DWORD PNP_GetInterfaceDeviceListSize(
    handle_t hBinding,
    PNP_RPC_BUFFER_SIZE *pulLen,
    GUID *InterfaceGuid,
    LPWSTR pszDeviceID,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 24 */
DWORD PNP_RegisterDeviceClassAssociation(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    GUID *InterfaceGuid,
    LPWSTR pszReference,
    LPWSTR pszSymLink,
    PNP_RPC_STRING_LEN *pulLength,
    PNP_RPC_STRING_LEN *pulTransferLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 25 */
DWORD PNP_UnregisterDeviceClassAssociation(
    handle_t hBinding,
    LPWSTR pszInterfaceDevice,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 26 */
DWORD PNP_GetClassRegProp(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    DWORD ulProperty,
    DWORD *pulRegDataType,
    BYTE *Buffer,
    PNP_RPC_STRING_LEN *pulTransferLen,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 27 */
DWORD PNP_SetClassRegProp(
    handle_t hBinding,
    LPWSTR *pszClassGuid,
    DWORD ulProperty,
    DWORD ulDataType,
    BYTE *Buffer,
    PNP_PROP_SIZE ulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 28 */
DWORD PNP_CreateDevInst(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    LPWSTR pszParentDeviceID,
    PNP_RPC_STRING_LEN ulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 29 */
#define PNP_DEVINST_SETUP       0x3
#define PNP_DEVINST_ENABLE      0x4
#define PNP_DEVINST_REENUMERATE 0x7
DWORD PNP_DeviceInstanceAction(
    handle_t hBinding,
    DWORD ulMajorAction,
    DWORD ulMinorAction,
    LPWSTR pszDeviceInstance1,
    LPWSTR pszDeviceInstance2)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulMinorAction);
    UNREFERENCED_PARAMETER(pszDeviceInstance1);
    UNREFERENCED_PARAMETER(pszDeviceInstance2);

    DPRINT("PNP_DeviceInstanceAction() called\n");

    switch (ulMajorAction)
    {
        case PNP_DEVINST_SETUP:
            DPRINT("Setup device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_ENABLE:
            DPRINT("Enable device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case PNP_DEVINST_REENUMERATE:
            DPRINT("Reenumerate device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("Unknown function %lu\n", ulMajorAction);
            ret = CR_CALL_NOT_IMPLEMENTED;
    }

    DPRINT("PNP_DeviceInstanceAction() done (returns %lx)\n", ret);

    return ret;
}


/* Function 30 */
DWORD PNP_GetDeviceStatus(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD *pulStatus,
    DWORD *pulProblem,
    DWORD ulFlags)
{
    PLUGPLAY_CONTROL_STATUS_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetDeviceStatus() called\n");

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pDeviceID);
    PlugPlayData.Operation = 0; /* Get status */

    Status = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulStatus = PlugPlayData.DeviceStatus;
        *pulProblem = PlugPlayData.DeviceProblem;
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetDeviceStatus() done (returns %lx)\n", ret);

    return ret;
}


/* Function 31 */
DWORD PNP_SetDeviceProblem(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulProblem,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 32 */
DWORD PNP_DisableDevInst(
    handle_t hBinding,
    LPWSTR pDeviceID,
    PPNP_VETO_TYPE pVetoType,
    LPWSTR pszVetoName,
    DWORD ulNameLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}

/* Function 33 */
DWORD PNP_UninstallDevInst(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


static BOOL
CheckForDeviceId(LPWSTR lpDeviceIdList,
                 LPWSTR lpDeviceId)
{
    LPWSTR lpPtr;
    DWORD dwLength;

    lpPtr = lpDeviceIdList;
    while (*lpPtr != 0)
    {
        dwLength = wcslen(lpPtr);
        if (0 == _wcsicmp(lpPtr, lpDeviceId))
            return TRUE;

        lpPtr += (dwLength + 1);
    }

    return FALSE;
}


static VOID
AppendDeviceId(LPWSTR lpDeviceIdList,
               LPDWORD lpDeviceIdListSize,
               LPWSTR lpDeviceId)
{
    DWORD dwLen;
    DWORD dwPos;

    dwLen = wcslen(lpDeviceId);
    dwPos = (*lpDeviceIdListSize / sizeof(WCHAR)) - 1;

    wcscpy(&lpDeviceIdList[dwPos], lpDeviceId);

    dwPos += (dwLen + 1);

    lpDeviceIdList[dwPos] = 0;

    *lpDeviceIdListSize = dwPos * sizeof(WCHAR);
}


/* Function 34 */
DWORD PNP_AddID(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    LPWSTR pszID,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hDeviceKey;
    LPWSTR pszSubKey;
    DWORD dwDeviceIdListSize;
    DWORD dwNewDeviceIdSize;
    WCHAR * pszDeviceIdList = NULL;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_AddID() called\n");
    DPRINT("  DeviceInstance: %S\n", pszDeviceID);
    DPRINT("  DeviceId: %S\n", pszID);
    DPRINT("  Flags: %lx\n", ulFlags);

    if (RegOpenKeyExW(hEnumKey,
                      pszDeviceID,
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &hDeviceKey) != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the device key!\n");
        return CR_INVALID_DEVNODE;
    }

    pszSubKey = (ulFlags & CM_ADD_ID_COMPATIBLE) ? L"CompatibleIDs" : L"HardwareID";

    if (RegQueryValueExW(hDeviceKey,
                         pszSubKey,
                         NULL,
                         NULL,
                         NULL,
                         &dwDeviceIdListSize) != ERROR_SUCCESS)
    {
        DPRINT("Failed to query the desired ID string!\n");
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    dwNewDeviceIdSize = lstrlenW(pszDeviceID);
    if (!dwNewDeviceIdSize)
    {
        ret = CR_INVALID_POINTER;
        goto Done;
    }

    dwDeviceIdListSize += (dwNewDeviceIdSize + 2) * sizeof(WCHAR);

    pszDeviceIdList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwDeviceIdListSize);
    if (!pszDeviceIdList)
    {
        DPRINT("Failed to allocate memory for the desired ID string!\n");
        ret = CR_OUT_OF_MEMORY;
        goto Done;
    }

    if (RegQueryValueExW(hDeviceKey,
                         pszSubKey,
                         NULL,
                         NULL,
                         (LPBYTE)pszDeviceIdList,
                         &dwDeviceIdListSize) != ERROR_SUCCESS)
    {
        DPRINT("Failed to query the desired ID string!\n");
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    /* Check whether the device ID is already in use */
    if (CheckForDeviceId(pszDeviceIdList, pszDeviceID))
    {
        DPRINT("Device ID was found in the ID string!\n");
        ret = CR_SUCCESS;
        goto Done;
    }

    /* Append the Device ID */
    AppendDeviceId(pszDeviceIdList, &dwDeviceIdListSize, pszID);

    if (RegSetValueExW(hDeviceKey,
                       pszSubKey,
                       0,
                       REG_MULTI_SZ,
                       (LPBYTE)pszDeviceIdList,
                       dwDeviceIdListSize) != ERROR_SUCCESS)
    {
        DPRINT("Failed to set the desired ID string!\n");
        ret = CR_REGISTRY_ERROR;
    }

Done:
    RegCloseKey(hDeviceKey);
    if (pszDeviceIdList)
        HeapFree(GetProcessHeap(), 0, pszDeviceIdList);

    DPRINT("PNP_AddID() done (returns %lx)\n", ret);

    return ret;
}


/* Function 35 */
DWORD PNP_RegisterDriver(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 36 */
DWORD PNP_QueryRemove(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    PPNP_VETO_TYPE pVetoType,
    LPWSTR pszVetoName,
    DWORD ulNameLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 37 */
DWORD PNP_RequestDeviceEject(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    PPNP_VETO_TYPE pVetoType,
    LPWSTR pszVetoName,
    DWORD ulNameLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}

/* Function 38 */
CONFIGRET
PNP_IsDockStationPresent(handle_t hBinding,
                         BOOL *Present)
{
    HKEY hKey;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwSize;
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT1("PNP_IsDockStationPresent() called\n");

    *Present = FALSE;

    if (RegOpenKeyExW(HKEY_CURRENT_CONFIG,
                      L"CurrentDockInfo",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return CR_REGISTRY_ERROR;

    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey,
                         L"DockingState",
                         NULL,
                         &dwType,
                         (LPBYTE)&dwValue,
                         &dwSize) != ERROR_SUCCESS)
        ret = CR_REGISTRY_ERROR;

    RegCloseKey(hKey);

    if (ret == CR_SUCCESS)
    {
        if (dwType != REG_DWORD || dwSize != sizeof(DWORD))
        {
            ret = CR_REGISTRY_ERROR;
        }
        else if (dwValue != 0)
        {
            *Present = TRUE;
        }
    }

    DPRINT1("PNP_IsDockStationPresent() done (returns %lx)\n", ret);

    return ret;
}


/* Function 39 */
DWORD PNP_RequestEjectPC(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 40 */
DWORD PNP_HwProfFlags(
    handle_t hBinding,
    DWORD ulAction,
    LPWSTR pDeviceID,
    DWORD ulConfig,
    DWORD *pulValue,
    PPNP_VETO_TYPE pVetoType,
    LPWSTR pszVetoName,
    DWORD ulNameLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 41 */
DWORD PNP_GetHwProfInfo(
    handle_t hBinding,
    DWORD ulIndex,
    HWPROFILEINFO *pHWProfileInfo,
    DWORD ulProfileInfoSize,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 42 */
DWORD PNP_AddEmptyLogConf(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulPriority,
    DWORD *pulLogConfTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 43 */
DWORD PNP_FreeLogConf(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfType,
    DWORD ulLogConfTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 44 */
DWORD PNP_GetFirstLogConf(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfType,
    DWORD *pulLogConfTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 45 */
DWORD PNP_GetNextLogConf(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfType,
    DWORD ulCurrentTag,
    DWORD *pulNextTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 46 */
DWORD PNP_GetLogConfPriority(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulType,
    DWORD ulTag,
    DWORD *pPriority,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 47 */
DWORD PNP_AddResDes(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID ResourceID,
    DWORD *pulResourceTag,
    BYTE *ResourceData,
    PNP_RPC_BUFFER_SIZE ResourceLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 48 */
DWORD PNP_FreeResDes(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID ResourceID,
    DWORD ulResourceTag,
    DWORD *pulPreviousResType,
    DWORD *pulPreviousResTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 49 */
DWORD PNP_GetNextResDes(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID ResourceID,
    DWORD ulResourceTag,
    DWORD *pulNextResType,
    DWORD *pulNextResTag,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 50 */
DWORD PNP_GetResDesData(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID ResourceID,
    DWORD ulResourceTag,
    BYTE *Buffer,
    PNP_RPC_BUFFER_SIZE BufferLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 51 */
DWORD PNP_GetResDesDataSize(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID ResourceID,
    DWORD ulResourceTag,
    DWORD *pulSize,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 52 */
DWORD PNP_ModifyResDes(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulLogConfTag,
    DWORD ulLogConfType,
    RESOURCEID CurrentResourceID,
    RESOURCEID NewResourceID,
    DWORD ulResourceTag,
    BYTE *ResourceData,
    PNP_RPC_BUFFER_SIZE ResourceLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 53 */
DWORD PNP_DetectResourceConflict(
    handle_t hBinding,
    LPWSTR pDeviceID,
    RESOURCEID ResourceID,
    BYTE *ResourceData,
    PNP_RPC_BUFFER_SIZE ResourceLen,
    BOOL *pbConflictDetected,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 54 */
DWORD PNP_QueryResConfList(
    handle_t hBinding,
    LPWSTR pDeviceID,
    RESOURCEID ResourceID,
    BYTE *ResourceData,
    PNP_RPC_BUFFER_SIZE ResourceLen,
    BYTE *Buffer,
    PNP_RPC_BUFFER_SIZE BufferLen,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 55 */
DWORD PNP_SetHwProf(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 56 */
DWORD PNP_QueryArbitratorFreeData(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 57 */
DWORD PNP_QueryArbitratorFreeSize(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 58 */
CONFIGRET
PNP_RunDetection(
    handle_t hBinding,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 59 */
DWORD PNP_RegisterNotification(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 60 */
DWORD PNP_UnregisterNotification(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 61 */
DWORD PNP_GetCustomDevProp(
    handle_t hBinding,
    LPWSTR pDeviceID,
    LPWSTR CustomPropName,
    DWORD *pulRegDataType,
    BYTE *Buffer,
    PNP_RPC_STRING_LEN *pulTransferLen,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 62 */
DWORD PNP_GetVersionInternal(
    handle_t hBinding,
    WORD *pwVersion)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 63 */
DWORD PNP_GetBlockedDriverInfo(
    handle_t hBinding,
    BYTE *Buffer,
    PNP_RPC_BUFFER_SIZE *pulTransferLen,
    PNP_RPC_BUFFER_SIZE *pulLength,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 64 */
DWORD PNP_GetServerSideDeviceInstallFlags(
    handle_t hBinding,
    DWORD *pulSSDIFlags,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 65 */
DWORD PNP_GetObjectPropKeys(
    handle_t hBinding,
    LPWSTR ObjectName,
    DWORD ObjectType,
    LPWSTR PropertyCultureName,
    PNP_PROP_COUNT *PropertyCount,
    PNP_PROP_COUNT *TransferLen,
    DEVPROPKEY *PropertyKeys,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 66 */
DWORD PNP_GetObjectProp(
    handle_t hBinding,
    LPWSTR ObjectName,
    DWORD ObjectType,
    LPWSTR PropertyCultureName,
    const DEVPROPKEY *PropertyKey,
    DEVPROPTYPE *PropertyType,
    PNP_PROP_SIZE *PropertySize,
    PNP_PROP_SIZE *TransferLen,
    BYTE *PropertyBuffer,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 67 */
DWORD PNP_SetObjectProp(
    handle_t hBinding,
    LPWSTR ObjectName,
    DWORD ObjectType,
    LPWSTR PropertyCultureName,
    const DEVPROPKEY *PropertyKey,
    DEVPROPTYPE PropertyType,
    PNP_PROP_SIZE PropertySize,
    BYTE *PropertyBuffer,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 68 */
DWORD PNP_InstallDevInst(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 69 */
DWORD PNP_ApplyPowerSettings(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 70 */
DWORD PNP_DriverStoreAddDriverPackage(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 71 */
DWORD PNP_DriverStoreDeleteDriverPackage(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 72 */
DWORD PNP_RegisterServiceNotification(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 73 */
DWORD PNP_SetActiveService(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 74 */
DWORD PNP_DeleteServiceDevices(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


typedef BOOL (WINAPI *PDEV_INSTALL_W)(HWND, HINSTANCE, LPCWSTR, INT);

static BOOL
InstallDevice(PCWSTR DeviceInstance, BOOL ShowWizard)
{
    PLUGPLAY_CONTROL_STATUS_DATA PlugPlayData;
    HMODULE hNewDev = NULL;
    PDEV_INSTALL_W DevInstallW;
    NTSTATUS Status;
    BOOL DeviceInstalled = FALSE;

    DPRINT("InstallDevice(%S, %d)\n", DeviceInstance, ShowWizard);

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         DeviceInstance);
    PlugPlayData.Operation = 0; /* Get status */

    /* Get device status */
    Status = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtPlugPlayControl('%S') failed with status 0x%08lx\n", DeviceInstance, Status);
        return FALSE;
    }

    if ((PlugPlayData.DeviceStatus & (DNF_STARTED | DNF_START_FAILED)) != 0)
    {
        /* Device is already started, or disabled due to some problem. Don't install it */
        DPRINT("No need to install '%S'\n", DeviceInstance);
        return TRUE;
    }

    /* Install device */
    SetEnvironmentVariableW(L"USERPROFILE", L"."); /* FIXME: why is it needed? */

    hNewDev = LoadLibraryW(L"newdev.dll");
    if (!hNewDev)
    {
        DPRINT1("Unable to load newdev.dll\n");
        goto cleanup;
    }

    DevInstallW = (PDEV_INSTALL_W)GetProcAddress(hNewDev, (LPCSTR)"DevInstallW");
    if (!DevInstallW)
    {
        DPRINT1("'DevInstallW' not found in newdev.dll\n");
        goto cleanup;
    }

    if (!DevInstallW(NULL, NULL, DeviceInstance, ShowWizard ? SW_SHOWNOACTIVATE : SW_HIDE))
    {
        DPRINT1("DevInstallW('%S') failed\n", DeviceInstance);
        goto cleanup;
    }

    DeviceInstalled = TRUE;

cleanup:
    if (hNewDev != NULL)
        FreeLibrary(hNewDev);

    return DeviceInstalled;
}


static LONG
ReadRegSzKey(
    IN HKEY hKey,
    IN LPCWSTR pszKey,
    OUT LPWSTR* pValue)
{
    LONG rc;
    DWORD dwType;
    DWORD cbData = 0;
    LPWSTR Value;

    if (!pValue)
        return ERROR_INVALID_PARAMETER;

    *pValue = NULL;
    rc = RegQueryValueExW(hKey, pszKey, NULL, &dwType, NULL, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;
    if (dwType != REG_SZ)
        return ERROR_FILE_NOT_FOUND;
    Value = HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
    if (!Value)
        return ERROR_NOT_ENOUGH_MEMORY;
    rc = RegQueryValueExW(hKey, pszKey, NULL, NULL, (LPBYTE)Value, &cbData);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Value);
        return rc;
    }
    /* NULL-terminate the string */
    Value[cbData / sizeof(WCHAR)] = '\0';

    *pValue = Value;
    return ERROR_SUCCESS;
}


static BOOL
SetupIsActive(VOID)
{
    HKEY hKey = NULL;
    DWORD regType, active, size;
    LONG rc;
    BOOL ret = FALSE;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    size = sizeof(DWORD);
    rc = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, &regType, (LPBYTE)&active, &size);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (regType != REG_DWORD || size != sizeof(DWORD))
        goto cleanup;

    ret = (active != 0);

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);

    DPRINT("System setup in progress? %S\n", ret ? L"YES" : L"NO");

    return ret;
}


static BOOL
IsConsoleBoot(VOID)
{
    HKEY ControlKey = NULL;
    LPWSTR SystemStartOptions = NULL;
    LPWSTR CurrentOption, NextOption; /* Pointers into SystemStartOptions */
    BOOL ConsoleBoot = FALSE;
    LONG rc;

    rc = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control",
        0,
        KEY_QUERY_VALUE,
        &ControlKey);

    rc = ReadRegSzKey(ControlKey, L"SystemStartOptions", &SystemStartOptions);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    /* Check for CMDCONS in SystemStartOptions */
    CurrentOption = SystemStartOptions;
    while (CurrentOption)
    {
        NextOption = wcschr(CurrentOption, L' ');
        if (NextOption)
            *NextOption = L'\0';
        if (_wcsicmp(CurrentOption, L"CONSOLE") == 0)
        {
            DPRINT("Found %S. Switching to console boot\n", CurrentOption);
            ConsoleBoot = TRUE;
            goto cleanup;
        }
        CurrentOption = NextOption ? NextOption + 1 : NULL;
    }

cleanup:
    if (ControlKey != NULL)
        RegCloseKey(ControlKey);
    HeapFree(GetProcessHeap(), 0, SystemStartOptions);
    return ConsoleBoot;
}


/* Loop to install all queued devices installations */
static DWORD WINAPI
DeviceInstallThread(LPVOID lpParameter)
{
#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
    PSLIST_ENTRY ListEntry;
#else
    PLIST_ENTRY ListEntry;
#endif
    DeviceInstallParams* Params;
    BOOL showWizard;

    UNREFERENCED_PARAMETER(lpParameter);

    WaitForSingleObject(hInstallEvent, INFINITE);

    showWizard = !SetupIsActive() && !IsConsoleBoot();

    while (TRUE)
    {
#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
        ListEntry = InterlockedPopEntrySList(&DeviceInstallListHead);
#else
        if ((BOOL)IsListEmpty(&DeviceInstallListHead))
            ListEntry = NULL;
        else
            ListEntry = RemoveHeadList(&DeviceInstallListHead);
#endif
        if (ListEntry == NULL)
        {
            SetEvent(hNoPendingInstalls);
            WaitForSingleObject(hDeviceInstallListNotEmpty, INFINITE);
        }
        else
        {
            ResetEvent(hNoPendingInstalls);
            Params = CONTAINING_RECORD(ListEntry, DeviceInstallParams, ListEntry);
            InstallDevice(Params->DeviceIds, showWizard);
        }
    }

    return 0;
}


static DWORD WINAPI
PnpEventThread(LPVOID lpParameter)
{
    PPLUGPLAY_EVENT_BLOCK PnpEvent;
    ULONG PnpEventSize;
    NTSTATUS Status;
    RPC_STATUS RpcStatus;

    UNREFERENCED_PARAMETER(lpParameter);

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
            HeapFree(GetProcessHeap(), 0, PnpEvent);
            PnpEvent = HeapAlloc(GetProcessHeap(), 0, PnpEventSize);
            if (PnpEvent == NULL)
                return ERROR_OUTOFMEMORY;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtGetPlugPlayEvent() failed (Status %lx)\n", Status);
            break;
        }

        /* Process the pnp event */
        DPRINT("Received PnP Event\n");
        if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ENUMERATED, &RpcStatus))
        {
            DeviceInstallParams* Params;
            DWORD len;
            DWORD DeviceIdLength;

            DPRINT("Device arrival event: %S\n", PnpEvent->TargetDevice.DeviceIds);

            DeviceIdLength = lstrlenW(PnpEvent->TargetDevice.DeviceIds);
            if (DeviceIdLength)
            {
                /* Queue device install (will be dequeued by DeviceInstallThread */
                len = FIELD_OFFSET(DeviceInstallParams, DeviceIds) + (DeviceIdLength + 1) * sizeof(WCHAR);
                Params = HeapAlloc(GetProcessHeap(), 0, len);
                if (Params)
                {
                    wcscpy(Params->DeviceIds, PnpEvent->TargetDevice.DeviceIds);
#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
                    InterlockedPushEntrySList(&DeviceInstallListHead, &Params->ListEntry);
#else
                    InsertTailList(&DeviceInstallListHead, &Params->ListEntry);
#endif
                    SetEvent(hDeviceInstallListNotEmpty);
                }
            }
        }
        else
        {
            DPRINT1("Unknown event, GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                PnpEvent->EventGuid.Data1, PnpEvent->EventGuid.Data2, PnpEvent->EventGuid.Data3,
                PnpEvent->EventGuid.Data4[0], PnpEvent->EventGuid.Data4[1], PnpEvent->EventGuid.Data4[2],
                PnpEvent->EventGuid.Data4[3], PnpEvent->EventGuid.Data4[4], PnpEvent->EventGuid.Data4[5],
                PnpEvent->EventGuid.Data4[6], PnpEvent->EventGuid.Data4[7]);
        }

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

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

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

    hThread = CreateThread(NULL,
                           0,
                           DeviceInstallThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    DPRINT("ServiceMain() done\n");
}


int
wmain(int argc, WCHAR *argv[])
{
    DWORD dwError;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("Umpnpmgr: main() started\n");

    hInstallEvent = CreateEvent(NULL, TRUE, SetupIsActive()/*FALSE*/, NULL);
    if (hInstallEvent == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Install Event! (Error %lu)\n", dwError);
        return dwError;
    }

    hDeviceInstallListNotEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hDeviceInstallListNotEmpty == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Event! (Error %lu)\n", dwError);
        return dwError;
    }

    hNoPendingInstalls = CreateEventW(NULL,
                                      TRUE,
                                      FALSE,
                                      L"Global\\PnP_No_Pending_Install_Events");
    if (hNoPendingInstalls == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Event! (Error %lu)\n", dwError);
        return dwError;
    }

#ifdef HAVE_SLIST_ENTRY_IMPLEMENTED
    InitializeSListHead(&DeviceInstallListHead);
#else
    InitializeListHead(&DeviceInstallListHead);
#endif

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
