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


/* Function 2 */
CONFIGRET
PNP_GetVersion(handle_t BindingHandle,
               unsigned short *Version)
{
    UNREFERENCED_PARAMETER(BindingHandle);

    *Version = 0x0400;
    return CR_SUCCESS;
}


/* Function 3 */
CONFIGRET
PNP_GetGlobalState(handle_t BindingHandle,
                   unsigned long *State,
                   unsigned long Flags)
{
    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

    *State = CM_GLOBAL_STATE_CAN_DO_UI | CM_GLOBAL_STATE_SERVICES_AVAILABLE;
    return CR_SUCCESS;
}


/* Function 4 */
CONFIGRET
PNP_InitDetection(handle_t BindingHandle)
{
    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT("PNP_InitDetection() called\n");
    return CR_SUCCESS;
}


/* Function 5 */
CONFIGRET
PNP_ReportLogOn(handle_t BindingHandle,
                unsigned long Admin,
                unsigned long ProcessId)
{
    HANDLE hProcess;

    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT1("PNP_ReportLogOn(%lu, %lu) called\n", Admin, ProcessId);

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
    if (hInstallEvent != NULL)
        SetEvent(hInstallEvent);

    return CR_SUCCESS;
}


/* Function 6 */
CONFIGRET
PNP_ValidateDeviceInstance(handle_t BindingHandle,
                           wchar_t *DeviceInstance,
                           unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hDeviceKey = NULL;

    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT("PNP_ValidateDeviceInstance(%S %lx) called\n",
           DeviceInstance, Flags);

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

    DPRINT("PNP_ValidateDeviceInstance() done (returns %lx)\n", ret);

    return ret;
}


/* Function 7 */
CONFIGRET
PNP_GetRootDeviceInstance(handle_t BindingHandle,
                          wchar_t *DeviceInstance,
                          unsigned long Length)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);

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


/* Function 8 */
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

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_GetRelatedDeviceInstance() called\n");
    DPRINT("  Relationship %ld\n", Relationship);
    DPRINT("  DeviceId %S\n", DeviceId);

    RtlInitUnicodeString(&PlugPlayData.TargetDeviceInstance,
                         DeviceId);

    PlugPlayData.Relation = Relationship;

    PlugPlayData.RelatedDeviceInstanceLength = Length;
    PlugPlayData.RelatedDeviceInstance = RelatedDeviceId;

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

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_EnumerateSubKeys() called\n");

    switch (Branch)
    {
        case PNP_BRANCH_ENUM:
            hKey = hEnumKey;
            break;

        case PNP_BRANCH_CLASS:
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
        ret = (dwError == ERROR_NO_MORE_ITEMS) ? CR_NO_SUCH_VALUE : CR_FAILURE;
    }
    else
    {
        (*RequiredLength)++;
    }

    DPRINT("PNP_EnumerateSubKeys() done (returns %lx)\n", ret);

    return ret;
}


/* Function 11 */
CONFIGRET
PNP_GetDeviceListSize(handle_t BindingHandle,
                      wchar_t *Filter,
                      unsigned long *Length,
                      DWORD Flags)
{
    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Filter);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_GetDeviceListSize() called\n");

    /* FIXME */
    *Length = 2;

    return CR_SUCCESS;
}


/* Function 12 */
CONFIGRET
PNP_GetDepth(handle_t BindingHandle,
             wchar_t *DeviceInstance,
             unsigned long *Depth,
             DWORD Flags)
{
    PLUGPLAY_CONTROL_DEPTH_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

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
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetDepth() done (returns %lx)\n", ret);

    return ret;
}


/* Function 13 */
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

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

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
            ret = NtStatusToCrError(Status);
        }
    }

    DPRINT("PNP_GetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 14 */
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

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

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


/* Function 15 */
CONFIGRET
PNP_GetClassInstance(handle_t BindingHandle,
                     wchar_t *DeviceId, /* in */
                     wchar_t *Buffer, /* out */
                     unsigned long Length)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceId);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);

    DPRINT("PNP_Get_Class_Instance() called\n");

    DPRINT("PNP_Get_Class_Instance() done (returns %lx)\n", ret);

    return ret;
}


/* Function 16 */
CONFIGRET
PNP_CreateKey(handle_t BindingHandle,
              wchar_t *SubKey,
              unsigned long samDesired,
              unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(SubKey);
    UNREFERENCED_PARAMETER(samDesired);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_CreateKey() called\n");

    DPRINT("PNP_CreateKey() done (returns %lx)\n", ret);

    return ret;
}


/* Function 17 */
CONFIGRET
PNP_DeleteRegistryKey(handle_t BindingHandle,
                      wchar_t *DeviceId,
                      wchar_t *ParentKey,
                      wchar_t *ChildKey,
                      unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceId);
    UNREFERENCED_PARAMETER(ParentKey);
    UNREFERENCED_PARAMETER(ChildKey);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_DeleteRegistryKey() called\n");

    DPRINT("PNP_DeleteRegistryKey() done (returns %lx)\n", ret);

    return ret;
}


/* Function 18 */
#if 0
CONFIGRET
PNP_GetClassCount(handle_t BindingHandle,
                  unsigned long *ClassCount,
                  unsigned long Flags)
{
    HANDLE hKey = NULL;
    DWORD dwError;

    UNREFERENCED_PARAMETER(BindingHandle);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            pszRegPathClass,
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return CR_INVALID_DATA;

    dwError = RegQueryInfoKeyW(hKey,
                               NULL,
                               NULL,
                               NULL,
                               &ClassCount,
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
#endif


/* Function 19 */
CONFIGRET
PNP_GetClassName(handle_t BindingHandle,
                 wchar_t *ClassGuid,
                 wchar_t *Buffer,
                 unsigned long *Length,
                 unsigned long Flags)
{
    WCHAR szKeyName[MAX_PATH];
    CONFIGRET ret = CR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwSize;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_GetClassName() called\n");

    lstrcpyW(szKeyName, L"System\\CurrentControlSet\\Control\\Class");
    lstrcatW(szKeyName, L"\\");
    if(lstrlenW(ClassGuid) < sizeof(szKeyName)/sizeof(WCHAR)-lstrlenW(szKeyName))
        lstrcatW(szKeyName, ClassGuid);
    else return CR_INVALID_DATA;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      szKeyName,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey))
        return CR_REGISTRY_ERROR;

    dwSize = *Length * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
                         L"Class",
                         NULL,
                         NULL,
                         (LPBYTE)Buffer,
                         &dwSize))
    {
        *Length = 0;
        ret = CR_REGISTRY_ERROR;
    }
    else
    {
        *Length = dwSize / sizeof(WCHAR);
    }

    RegCloseKey(hKey);

    DPRINT("PNP_GetClassName() done (returns %lx)\n", ret);

    return ret;
}


/* Function 20 */
CONFIGRET
PNP_DeleteClassKey(handle_t BindingHandle,
                   wchar_t *ClassGuid,
                   unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT("PNP_GetClassName(%S, %lx) called\n", ClassGuid, Flags);

    if (Flags & CM_DELETE_CLASS_SUBKEYS)
    {
        if (RegDeleteTreeW(hClassKey, ClassGuid) != ERROR_SUCCESS)
            ret = CR_REGISTRY_ERROR;
    }
    else
    {
        if (RegDeleteKeyW(hClassKey, ClassGuid) != ERROR_SUCCESS)
            ret = CR_REGISTRY_ERROR;
    }

    DPRINT("PNP_DeleteClassKey() done (returns %lx)\n", ret);

    return ret;
}


/* Function 28 */
CONFIGRET
PNP_CreateDevInst(handle_t BindingHandle,
                  wchar_t *DeviceId,       /* [in, out, string, size_is(Length)] */
                  wchar_t *ParentDeviceId, /* [in, string] */
                  unsigned long Length,    /* [in] */
                  unsigned long Flags)     /* [in] */
{
    CONFIGRET ret = CR_CALL_NOT_IMPLEMENTED;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceId);
    UNREFERENCED_PARAMETER(ParentDeviceId);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT1("PNP_CreateDevInst() called\n");

    DPRINT1("PNP_CreateDevInst() done (returns %lx)\n", ret);

    return ret;
}


/* Function 29 */
CONFIGRET
PNP_DeviceInstanceAction(handle_t BindingHandle,
                         unsigned long MajorAction,
                         unsigned long MinorAction,
                         wchar_t *DeviceInstance1,
                         wchar_t *DeviceInstance2)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(MinorAction);
    UNREFERENCED_PARAMETER(DeviceInstance1);
    UNREFERENCED_PARAMETER(DeviceInstance2);

    DPRINT("PNP_DeviceInstanceAction() called\n");

    switch (MajorAction)
    {
        case 2:
            DPRINT("Move device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case 3:
            DPRINT("Setup device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case 4:
            DPRINT("Enable device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case 5:
            DPRINT("Disable device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        case 7:
            DPRINT("Reenumerate device instance\n");
            /* FIXME */
            ret = CR_CALL_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("Unknown function %lu\n", MajorAction);
            ret = CR_CALL_NOT_IMPLEMENTED;
    }

    DPRINT("PNP_DeviceInstanceAction() done (returns %lx)\n", ret);

    return ret;
}


/* Function 30 */
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

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

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
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetDeviceStatus() done (returns %lx)\n", ret);

    return ret;
}


/* Function 31 */
CONFIGRET
PNP_SetDeviceProblem(handle_t BindingHandle,
                     wchar_t *DeviceInstance,
                     unsigned long Problem,
                     DWORD Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(Problem);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT1("PNP_SetDeviceProblem() called\n");

    /* FIXME */

    DPRINT1("PNP_SetDeviceProblem() done (returns %lx)\n", ret);

    return ret;
}


/* Function 33 */
CONFIGRET
PNP_UninstallDevInst(handle_t BindingHandle,
                     wchar_t *DeviceInstance,
                     DWORD Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT1("PNP_UninstallDevInst() called\n");

    /* FIXME */

    DPRINT1("PNP_UninstallDevInst() done (returns %lx)\n", ret);

    return ret;
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
CONFIGRET
PNP_AddID(handle_t BindingHandle,
          wchar_t *DeviceInstance,
          wchar_t *DeviceId,
          DWORD Flags)
{
    CONFIGRET ret = CR_SUCCESS;
    HKEY hDeviceKey;
    LPWSTR pszSubKey;
    DWORD dwDeviceIdListSize;
    WCHAR szDeviceIdList[512];

    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT("PNP_AddID() called\n");
    DPRINT("  DeviceInstance: %S\n", DeviceInstance);
    DPRINT("  DeviceId: %S\n", DeviceId);
    DPRINT("  Flags: %lx\n", Flags);

    if (RegOpenKeyExW(hEnumKey,
                      DeviceInstance,
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &hDeviceKey) != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the device key!\n");
        return CR_INVALID_DEVNODE;
    }

    pszSubKey = (Flags & CM_ADD_ID_COMPATIBLE) ? L"CompatibleIDs" : L"HardwareID";

    dwDeviceIdListSize = 512 * sizeof(WCHAR);
    if (RegQueryValueExW(hDeviceKey,
                         pszSubKey,
                         NULL,
                         NULL,
                         (LPBYTE)szDeviceIdList,
                         &dwDeviceIdListSize) != ERROR_SUCCESS)
    {
        DPRINT("Failed to query the desired ID string!\n");
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    /* Check whether the device ID is already in use */
    if (CheckForDeviceId(szDeviceIdList, DeviceId))
    {
        DPRINT("Device ID was found in the ID string!\n");
        ret = CR_SUCCESS;
        goto Done;
    }

    /* Append the Device ID */
    AppendDeviceId(szDeviceIdList, &dwDeviceIdListSize, DeviceId);

    if (RegSetValueExW(hDeviceKey,
                       pszSubKey,
                       0,
                       REG_MULTI_SZ,
                       (LPBYTE)szDeviceIdList,
                       dwDeviceIdListSize) != ERROR_SUCCESS)
    {
        DPRINT("Failed to set the desired ID string!\n");
        ret = CR_REGISTRY_ERROR;
    }

Done:
    RegCloseKey(hDeviceKey);

    DPRINT("PNP_AddID() done (returns %lx)\n", ret);

    return ret;
}


/* Function 38 */
CONFIGRET
PNP_IsDockStationPresent(handle_t BindingHandle,
                         unsigned long *Present)
{
    HKEY hKey;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwSize;
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);

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
CONFIGRET
PNP_RequestEjectPC(handle_t BindingHandle)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);

    DPRINT1("PNP_RequestEjectPC() called\n");

    ret = CR_FAILURE; /* FIXME */

    DPRINT1("PNP_RequestEjectPC() done (returns %lx)\n", ret);

    return ret;
}


/* Function 40 */
CONFIGRET
PNP_HwProfFlags(handle_t BindingHandle,
                unsigned long Action,
                wchar_t *DeviceId,
                unsigned long ProfileId,
                unsigned long *Value, // out
                unsigned long Flags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Action);
    UNREFERENCED_PARAMETER(DeviceId);
    UNREFERENCED_PARAMETER(ProfileId);
    UNREFERENCED_PARAMETER(Value);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT1("PNP_HwProfFlags() called\n");

    ret = CR_CALL_NOT_IMPLEMENTED; /* FIXME */

    DPRINT1("PNP_HwProfFlags() done (returns %lx)\n", ret);

    return ret;
}


/* Function 42 */
CONFIGRET
PNP_AddEmptyLogConf(handle_t BindingHandle,
                    wchar_t *DeviceInstance,
                    unsigned long ulPriority,
                    unsigned long *pulLogConfTag,
                    unsigned long ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(ulPriority);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT1("PNP_AddEmptyLogConf() called\n");

    *pulLogConfTag = 0; /* FIXME */

    DPRINT1("PNP_AddEmptyLogConf() done (returns %lx)\n", ret);

    return ret;
}


/* Function 43 */
CONFIGRET
PNP_FreeLogConf(handle_t BindingHandle,
                wchar_t *DeviceInstance,
                unsigned long ulType,
                unsigned long ulLogConfTag,
                unsigned long ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(ulType);
    UNREFERENCED_PARAMETER(ulLogConfTag);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT1("PNP_FreeLogConf() called\n");


    DPRINT1("PNP_FreeLogConf() done (returns %lx)\n", ret);

    return ret;
}


/* Function 44 */
CONFIGRET
PNP_GetFirstLogConf(handle_t BindingHandle,
                    wchar_t *DeviceInstance,
                    unsigned long ulPriority,
                    unsigned long *pulLogConfTag,
                    unsigned long ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(ulPriority);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT1("PNP_GetFirstLogConf() called\n");

    *pulLogConfTag = 0; /* FIXME */

    DPRINT1("PNP_GetFirstLogConf() done (returns %lx)\n", ret);

    return ret;
}


/* Function 45 */
CONFIGRET
PNP_GetNextLogConf(handle_t BindingHandle,
                   wchar_t *DeviceInstance,
                   unsigned long ulLogConfType,
                   unsigned long ulCurrentTag,
                   unsigned long *pulNextTag,
                   unsigned long ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(ulLogConfType);
    UNREFERENCED_PARAMETER(ulCurrentTag);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT1("PNP_GetNextLogConf() called\n");

    *pulNextTag = 0; /* FIXME */

    DPRINT1("PNP_GetNextLogConf() done (returns %lx)\n", ret);

    return ret;
}


/* Function 46 */
CONFIGRET
PNP_GetLogConfPriority(handle_t BindingHandle,
                       wchar_t *DeviceInstance,
                       unsigned long ulLogConfType,
                       unsigned long ulCurrentTag,
                       unsigned long *pPriority,
                       unsigned long ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(DeviceInstance);
    UNREFERENCED_PARAMETER(ulLogConfType);
    UNREFERENCED_PARAMETER(ulCurrentTag);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT1("PNP_GetLogConfPriority() called\n");

    *pPriority = 0; /* FIXME */

    DPRINT1("PNP_GetLogConfPriority() done (returns %lx)\n", ret);

    return ret;
}


/* Function 58 */
CONFIGRET
PNP_RunDetection(handle_t BindingHandle,
                 unsigned long Flags)
{
    UNREFERENCED_PARAMETER(BindingHandle);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("PNP_RunDetection() called\n");
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

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         DeviceInstance);
    PlugPlayData.Operation = 0; /* Get status */

    /* Get device status */
    Status = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    if (!NT_SUCCESS(Status))
        return FALSE;

    if ((PlugPlayData.DeviceStatus & (DNF_STARTED | DNF_START_FAILED)) != 0)
        /* Device is already started, or disabled due to some problem. Don't install it */
        return TRUE;

    /* Install device */
    SetEnvironmentVariableW(L"USERPROFILE", L"."); /* FIXME: why is it needed? */

    hNewDev = LoadLibraryW(L"newdev.dll");
    if (!hNewDev)
        goto cleanup;

    DevInstallW = (PDEV_INSTALL_W)GetProcAddress(hNewDev, (LPCSTR)"DevInstallW");
    if (!DevInstallW)
        goto cleanup;

    if (!DevInstallW(NULL, NULL, DeviceInstance, ShowWizard ? SW_SHOWNOACTIVATE : SW_HIDE))
        goto cleanup;

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
        if (wcsicmp(CurrentOption, L"CONSOLE") == 0)
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

    showWizard = !SetupIsActive() && !IsConsoleBoot();

    SetEnvironmentVariable(L"USERPROFILE", L"."); /* FIXME: why is it needed? */

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

        /* Process the pnp event */
        DPRINT("Received PnP Event\n");
        if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ARRIVAL, &RpcStatus))
        {
            DeviceInstallParams* Params;
            DWORD len;

            DPRINT("Device arrival event: %S\n", PnpEvent->TargetDevice.DeviceIds);

            /* Queue device install (will be dequeued by DeviceInstallThread */
            len = FIELD_OFFSET(DeviceInstallParams, DeviceIds)
                + wcslen(PnpEvent->TargetDevice.DeviceIds) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
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
        else
        {
            DPRINT1("Unknown event\n");
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

    hNoPendingInstalls = CreateEventW(NULL,
                                      TRUE,
                                      FALSE,
                                      L"Global\\PnP_No_Pending_Install_Events");

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
                           DeviceInstallThread,
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

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("Umpnpmgr: main() started\n");

    hInstallEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
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
