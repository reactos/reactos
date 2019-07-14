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
 * FILE:             base/services/umpnpmgr/rpcserver.c
 * PURPOSE:          RPC server
 * PROGRAMMER:       Eric Kohl (eric.kohl@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Colin Finck (colin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

static WCHAR szRootDeviceId[] = L"HTREE\\ROOT\\0";


/* FUNCTIONS *****************************************************************/

DWORD WINAPI
RpcServerThread(LPVOID lpParameter)
{
    RPC_STATUS Status;
    BOOLEAN RegisteredProtSeq = FALSE;

    UNREFERENCED_PARAMETER(lpParameter);

    DPRINT("RpcServerThread() called\n");

#if 0
    /* 2k/XP/2k3-compatible protocol sequence/endpoint */
    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    20,
                                    L"\\pipe\\ntsvcs",
                                    NULL);  // Security descriptor
    if (Status == RPC_S_OK)
        RegisteredProtSeq = TRUE;
    else
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
#endif

    /* Vista/7-compatible protocol sequence/endpoint */
    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    20,
                                    L"\\pipe\\plugplay",
                                    NULL);  // Security descriptor
    if (Status == RPC_S_OK)
        RegisteredProtSeq = TRUE;
    else
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);

    /* Make sure there's a usable endpoint */
    if (RegisteredProtSeq == FALSE)
        return 0;

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


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
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
        case STATUS_NOT_IMPLEMENTED:
            return CR_CALL_NOT_IMPLEMENTED;

        case STATUS_INVALID_PARAMETER:
            return CR_INVALID_DATA;

        case STATUS_NO_SUCH_DEVICE:
            return CR_NO_SUCH_DEVINST;

        case STATUS_ACCESS_DENIED:
            return CR_ACCESS_DENIED;

        case STATUS_BUFFER_TOO_SMALL:
            return CR_BUFFER_SMALL;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return CR_NO_SUCH_VALUE;

        default:
            return CR_FAILURE;
    }
}


static VOID
SplitDeviceInstanceID(IN LPWSTR pszDeviceInstanceID,
                      OUT LPWSTR pszEnumerator,
                      OUT LPWSTR pszDevice,
                      OUT LPWSTR pszInstance)
{
    WCHAR szLocalDeviceInstanceID[MAX_DEVICE_ID_LEN];
    LPWSTR lpEnumerator = NULL;
    LPWSTR lpDevice = NULL;
    LPWSTR lpInstance = NULL;
    LPWSTR ptr;

    wcscpy(szLocalDeviceInstanceID, pszDeviceInstanceID);

    *pszEnumerator = 0;
    *pszDevice = 0;
    *pszInstance = 0;

    lpEnumerator = szLocalDeviceInstanceID;

    ptr = wcschr(lpEnumerator, L'\\');
    if (ptr != NULL)
    {
        *ptr = 0;
        lpDevice = ++ptr;

        ptr = wcschr(lpDevice, L'\\');
        if (ptr != NULL)
        {
            *ptr = 0;
            lpInstance = ++ptr;
        }
    }

    if (lpEnumerator != NULL)
        wcscpy(pszEnumerator, lpEnumerator);

    if (lpDevice != NULL)
        wcscpy(pszDevice, lpDevice);

    if (lpInstance != NULL)
        wcscpy(pszInstance, lpInstance);
}


static
CONFIGRET
GetDeviceStatus(
    _In_ LPWSTR pDeviceID,
    _Out_ DWORD *pulStatus,
    _Out_ DWORD *pulProblem)
{
    PLUGPLAY_CONTROL_STATUS_DATA PlugPlayData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT("GetDeviceStatus(%S %p %p)\n",
           pDeviceID, pulStatus, pulProblem);

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

    return ret;
}


/* PUBLIC FUNCTIONS **********************************************************/

/* Function 0 */
DWORD
WINAPI
PNP_Disconnect(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);
    return CR_SUCCESS;
}


/* Function 1 */
DWORD
WINAPI
PNP_Connect(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);
    return CR_SUCCESS;
}


/* Function 2 */
DWORD
WINAPI
PNP_GetVersion(
    handle_t hBinding,
    WORD *pVersion)
{
    UNREFERENCED_PARAMETER(hBinding);

    *pVersion = 0x0400;
    return CR_SUCCESS;
}


/* Function 3 */
DWORD
WINAPI
PNP_GetGlobalState(
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
DWORD
WINAPI
PNP_InitDetection(
    handle_t hBinding)
{
    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_InitDetection() called\n");
    return CR_SUCCESS;
}


/* Function 5 */
DWORD
WINAPI
PNP_ReportLogOn(
    handle_t hBinding,
    BOOL Admin,
    DWORD ProcessId)
{
    DWORD ReturnValue = CR_FAILURE;
    HANDLE hProcess;

    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(Admin);

    DPRINT("PNP_ReportLogOn(%u, %u) called\n", Admin, ProcessId);

    /* Get the users token */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessId);

    if (!hProcess)
    {
        DPRINT1("OpenProcess failed with error %u\n", GetLastError());
        goto cleanup;
    }

    if (hUserToken)
    {
        CloseHandle(hUserToken);
        hUserToken = NULL;
    }

    if (!OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY, &hUserToken))
    {
        DPRINT1("OpenProcessToken failed with error %u\n", GetLastError());
        goto cleanup;
    }

    /* Trigger the installer thread */
    if (hInstallEvent)
        SetEvent(hInstallEvent);

    ReturnValue = CR_SUCCESS;

cleanup:
    if (hProcess)
        CloseHandle(hProcess);

    return ReturnValue;
}


/* Function 6 */
DWORD
WINAPI
PNP_ValidateDeviceInstance(
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
DWORD
WINAPI
PNP_GetRootDeviceInstance(
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
DWORD
WINAPI
PNP_GetRelatedDeviceInstance(
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
DWORD
WINAPI
PNP_EnumerateSubKeys(
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


static
CONFIGRET
GetRelationsInstanceList(
    _In_ PWSTR pszDevice,
    _In_ DWORD ulFlags,
    _Inout_ PWSTR pszBuffer,
    _Inout_ PDWORD pulLength)
{
    PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA PlugPlayData;
    NTSTATUS Status;
    CONFIGRET ret = CR_SUCCESS;

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pszDevice);

    if (ulFlags & CM_GETIDLIST_FILTER_BUSRELATIONS)
    {
        PlugPlayData.Relations = 3;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_POWERRELATIONS)
    {
        PlugPlayData.Relations = 2;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_REMOVALRELATIONS)
    {
        PlugPlayData.Relations = 1;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_EJECTRELATIONS)
    {
        PlugPlayData.Relations = 0;
    }

    PlugPlayData.BufferSize = *pulLength * sizeof(WCHAR);
    PlugPlayData.Buffer = pszBuffer;

    Status = NtPlugPlayControl(PlugPlayControlQueryDeviceRelations,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulLength = PlugPlayData.BufferSize / sizeof(WCHAR);
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    return ret;
}


static
CONFIGRET
GetServiceInstanceList(
    _In_ PWSTR pszService,
    _Inout_ PWSTR pszBuffer,
    _Inout_ PDWORD pulLength)
{
    WCHAR szPathBuffer[512];
    WCHAR szName[16];
    HKEY hServicesKey = NULL, hServiceKey = NULL, hEnumKey = NULL;
    DWORD dwValues, dwSize, dwIndex, dwUsedLength, dwPathLength;
    DWORD dwError;
    PWSTR pPtr;
    CONFIGRET ret = CR_SUCCESS;

    /* Open the device key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the services key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    dwError = RegOpenKeyExW(hServicesKey,
                            pszService,
                            0,
                            KEY_READ,
                            &hServiceKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the service key (Error %lu)\n", dwError);
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    dwError = RegOpenKeyExW(hServiceKey,
                            L"Enum",
                            0,
                            KEY_READ,
                            &hEnumKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the service enum key (Error %lu)\n", dwError);
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    /* Retrieve the number of device instances */
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hEnumKey,
                               L"Count",
                               NULL,
                               NULL,
                               (LPBYTE)&dwValues,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("RegQueryValueExW failed (Error %lu)\n", dwError);
        dwValues = 1;
    }

    DPRINT("dwValues %lu\n", dwValues);

    dwUsedLength = 0;
    pPtr = pszBuffer;

    for (dwIndex = 0; dwIndex < dwValues; dwIndex++)
    {
        wsprintf(szName, L"%lu", dwIndex);

        dwSize = sizeof(szPathBuffer);
        dwError = RegQueryValueExW(hEnumKey,
                                   szName,
                                   NULL,
                                   NULL,
                                   (LPBYTE)szPathBuffer,
                                   &dwSize);
        if (dwError != ERROR_SUCCESS)
            break;

        DPRINT("Path: %S\n", szPathBuffer);

        dwPathLength = wcslen(szPathBuffer) + 1;
        if (dwUsedLength + dwPathLength + 1 > *pulLength)
        {
            ret = CR_BUFFER_SMALL;
            break;
        }

        wcscpy(pPtr, szPathBuffer);
        dwUsedLength += dwPathLength;
        pPtr += dwPathLength;

        *pPtr = UNICODE_NULL;
    }

Done:
    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    if (hServicesKey != NULL)
        RegCloseKey(hServicesKey);

    if (ret == CR_SUCCESS)
        *pulLength = dwUsedLength + 1;
    else
        *pulLength = 0;

    return ret;
}


static
CONFIGRET
GetDeviceInstanceList(
    _In_ PWSTR pszDevice,
    _Inout_ PWSTR pszBuffer,
    _Inout_ PDWORD pulLength)
{
    WCHAR szInstanceBuffer[MAX_DEVICE_ID_LEN];
    WCHAR szPathBuffer[512];
    HKEY hDeviceKey;
    DWORD dwInstanceLength, dwPathLength, dwUsedLength;
    DWORD dwIndex, dwError;
    PWSTR pPtr;
    CONFIGRET ret = CR_SUCCESS;

    /* Open the device key */
    dwError = RegOpenKeyExW(hEnumKey,
                            pszDevice,
                            0,
                            KEY_ENUMERATE_SUB_KEYS,
                            &hDeviceKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the device key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    dwUsedLength = 0;
    pPtr = pszBuffer;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwInstanceLength = MAX_DEVICE_ID_LEN;
        dwError = RegEnumKeyExW(hDeviceKey,
                                dwIndex,
                                szInstanceBuffer,
                                &dwInstanceLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (dwError != ERROR_SUCCESS)
            break;

        wsprintf(szPathBuffer, L"%s\\%s", pszDevice, szInstanceBuffer);
        DPRINT("Path: %S\n", szPathBuffer);

        dwPathLength = wcslen(szPathBuffer) + 1;
        if (dwUsedLength + dwPathLength + 1 > *pulLength)
        {
            ret = CR_BUFFER_SMALL;
            break;
        }

        wcscpy(pPtr, szPathBuffer);
        dwUsedLength += dwPathLength;
        pPtr += dwPathLength;

        *pPtr = UNICODE_NULL;
    }

    RegCloseKey(hDeviceKey);

    if (ret == CR_SUCCESS)
        *pulLength = dwUsedLength + 1;
    else
        *pulLength = 0;

    return ret;
}


CONFIGRET
GetEnumeratorInstanceList(
    _In_ PWSTR pszEnumerator,
    _Inout_ PWSTR pszBuffer,
    _Inout_ PDWORD pulLength)
{
    WCHAR szDeviceBuffer[MAX_DEVICE_ID_LEN];
    WCHAR szPathBuffer[512];
    HKEY hEnumeratorKey;
    PWSTR pPtr;
    DWORD dwIndex, dwDeviceLength, dwUsedLength, dwRemainingLength, dwPathLength;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    /* Open the enumerator key */
    dwError = RegOpenKeyExW(hEnumKey,
                            pszEnumerator,
                            0,
                            KEY_ENUMERATE_SUB_KEYS,
                            &hEnumeratorKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the enumerator key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    dwUsedLength = 0;
    dwRemainingLength = *pulLength;
    pPtr = pszBuffer;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwDeviceLength = MAX_DEVICE_ID_LEN;
        dwError = RegEnumKeyExW(hEnumeratorKey,
                                dwIndex,
                                szDeviceBuffer,
                                &dwDeviceLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (dwError != ERROR_SUCCESS)
            break;

        wsprintf(szPathBuffer, L"%s\\%s", pszEnumerator, szDeviceBuffer);
        DPRINT("Path: %S\n", szPathBuffer);

        dwPathLength = dwRemainingLength;
        ret = GetDeviceInstanceList(szPathBuffer,
                                    pPtr,
                                    &dwPathLength);
        if (ret != CR_SUCCESS)
            break;

        dwUsedLength += dwPathLength - 1;
        dwRemainingLength += dwPathLength - 1;
        pPtr += dwPathLength - 1;
    }

    RegCloseKey(hEnumeratorKey);

    if (ret == CR_SUCCESS)
        *pulLength = dwUsedLength + 1;
    else
        *pulLength = 0;

    return ret;
}


static
CONFIGRET
GetAllInstanceList(
    _Inout_ PWSTR pszBuffer,
    _Inout_ PDWORD pulLength)
{
    WCHAR szEnumeratorBuffer[MAX_DEVICE_ID_LEN];
    PWSTR pPtr;
    DWORD dwIndex, dwEnumeratorLength, dwUsedLength, dwRemainingLength, dwPathLength;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    dwUsedLength = 0;
    dwRemainingLength = *pulLength;
    pPtr = pszBuffer;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwEnumeratorLength = MAX_DEVICE_ID_LEN;
        dwError = RegEnumKeyExW(hEnumKey,
                                dwIndex,
                                szEnumeratorBuffer,
                                &dwEnumeratorLength,
                                NULL, NULL, NULL, NULL);
        if (dwError != ERROR_SUCCESS)
            break;

        dwPathLength = dwRemainingLength;
        ret = GetEnumeratorInstanceList(szEnumeratorBuffer,
                                        pPtr,
                                        &dwPathLength);
        if (ret != CR_SUCCESS)
            break;

        dwUsedLength += dwPathLength - 1;
        dwRemainingLength += dwPathLength - 1;
        pPtr += dwPathLength - 1;
    }

    if (ret == CR_SUCCESS)
        *pulLength = dwUsedLength + 1;
    else
        *pulLength = 0;

    return ret;
}


/* Function 10 */
DWORD
WINAPI
PNP_GetDeviceList(
    handle_t hBinding,
    LPWSTR pszFilter,
    LPWSTR Buffer,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    WCHAR szEnumerator[MAX_DEVICE_ID_LEN];
    WCHAR szDevice[MAX_DEVICE_ID_LEN];
    WCHAR szInstance[MAX_DEVICE_ID_LEN];
    CONFIGRET ret = CR_SUCCESS;

    DPRINT("PNP_GetDeviceList() called\n");

    if (ulFlags & ~CM_GETIDLIST_FILTER_BITS)
        return CR_INVALID_FLAG;

    if (pulLength == NULL)
        return CR_INVALID_POINTER;

    if ((ulFlags != CM_GETIDLIST_FILTER_NONE) &&
        (pszFilter == NULL))
        return CR_INVALID_POINTER;

    if (ulFlags &
        (CM_GETIDLIST_FILTER_BUSRELATIONS |
         CM_GETIDLIST_FILTER_POWERRELATIONS |
         CM_GETIDLIST_FILTER_REMOVALRELATIONS |
         CM_GETIDLIST_FILTER_EJECTRELATIONS))
    {
        ret = GetRelationsInstanceList(pszFilter,
                                       ulFlags,
                                       Buffer,
                                       pulLength);
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_SERVICE)
    {
        ret = GetServiceInstanceList(pszFilter,
                                     Buffer,
                                     pulLength);
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_ENUMERATOR)
    {
        SplitDeviceInstanceID(pszFilter,
                              szEnumerator,
                              szDevice,
                              szInstance);

        if (*szEnumerator != UNICODE_NULL && *szDevice != UNICODE_NULL)
        {
            ret = GetDeviceInstanceList(pszFilter,
                                        Buffer,
                                        pulLength);
        }
        else
        {
            ret = GetEnumeratorInstanceList(pszFilter,
                                            Buffer,
                                            pulLength);
        }
    }
    else /* CM_GETIDLIST_FILTER_NONE */
    {
        ret = GetAllInstanceList(Buffer,
                                 pulLength);
    }

    return ret;
}


static
CONFIGRET
GetRelationsInstanceListSize(
    _In_ PWSTR pszDevice,
    _In_ DWORD ulFlags,
    _Inout_ PDWORD pulLength)
{
    PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA PlugPlayData;
    NTSTATUS Status;
    CONFIGRET ret = CR_SUCCESS;

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pszDevice);

    if (ulFlags & CM_GETIDLIST_FILTER_BUSRELATIONS)
    {
        PlugPlayData.Relations = 3;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_POWERRELATIONS)
    {
        PlugPlayData.Relations = 2;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_REMOVALRELATIONS)
    {
        PlugPlayData.Relations = 1;
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_EJECTRELATIONS)
    {
        PlugPlayData.Relations = 0;
    }

    PlugPlayData.BufferSize = 0;
    PlugPlayData.Buffer = NULL;

    Status = NtPlugPlayControl(PlugPlayControlQueryDeviceRelations,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulLength = PlugPlayData.BufferSize / sizeof(WCHAR);
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    return ret;
}


static
CONFIGRET
GetServiceInstanceListSize(
    _In_ PWSTR pszService,
    _Out_ PDWORD pulLength)
{
    HKEY hServicesKey = NULL, hServiceKey = NULL, hEnumKey = NULL;
    DWORD dwValues, dwMaxValueLength, dwSize;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    /* Open the device key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the services key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    dwError = RegOpenKeyExW(hServicesKey,
                            pszService,
                            0,
                            KEY_READ,
                            &hServiceKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the service key (Error %lu)\n", dwError);
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    dwError = RegOpenKeyExW(hServiceKey,
                            L"Enum",
                            0,
                            KEY_READ,
                            &hEnumKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the service enum key (Error %lu)\n", dwError);
        ret = CR_REGISTRY_ERROR;
        goto Done;
    }

    /* Retrieve the number of device instances */
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hEnumKey,
                               L"Count",
                               NULL,
                               NULL,
                               (LPBYTE)&dwValues,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("RegQueryValueExW failed (Error %lu)\n", dwError);
        dwValues = 1;
    }

    /* Retrieve the maximum instance name length */
    dwError = RegQueryInfoKeyW(hEnumKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &dwMaxValueLength,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("RegQueryInfoKeyW failed (Error %lu)\n", dwError);
        dwMaxValueLength = MAX_DEVICE_ID_LEN;
    }

    DPRINT("dwValues %lu  dwMaxValueLength %lu\n", dwValues, dwMaxValueLength / sizeof(WCHAR));

    /* Return the largest possible buffer size */
    *pulLength = dwValues * dwMaxValueLength / sizeof(WCHAR) + 2;

Done:
    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    if (hServicesKey != NULL)
        RegCloseKey(hServicesKey);

    return ret;
}


static
CONFIGRET
GetDeviceInstanceListSize(
    _In_ LPCWSTR pszDevice,
    _Out_ PULONG pulLength)
{
    HKEY hDeviceKey;
    DWORD dwSubKeys, dwMaxSubKeyLength;
    DWORD dwError;

    /* Open the device key */
    dwError = RegOpenKeyExW(hEnumKey,
                            pszDevice,
                            0,
                            KEY_READ,
                            &hDeviceKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the device key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    /* Retrieve the number of device instances and the maximum name length */
    dwError = RegQueryInfoKeyW(hDeviceKey,
                               NULL,
                               NULL,
                               NULL,
                               &dwSubKeys,
                               &dwMaxSubKeyLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("RegQueryInfoKeyW failed (Error %lu)\n", dwError);
        dwSubKeys = 0;
        dwMaxSubKeyLength = 0;
    }

    /* Close the device key */
    RegCloseKey(hDeviceKey);

    /* Return the largest possible buffer size */
    *pulLength = dwSubKeys * (wcslen(pszDevice) + 1 + dwMaxSubKeyLength + 1);

    return CR_SUCCESS;
}


static
CONFIGRET
GetEnumeratorInstanceListSize(
    _In_ LPCWSTR pszEnumerator,
    _Out_ PULONG pulLength)
{
    WCHAR szDeviceBuffer[MAX_DEVICE_ID_LEN];
    WCHAR szPathBuffer[512];
    HKEY hEnumeratorKey;
    DWORD dwIndex, dwDeviceLength, dwBufferLength;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    *pulLength = 0;

    /* Open the enumerator key */
    dwError = RegOpenKeyExW(hEnumKey,
                            pszEnumerator,
                            0,
                            KEY_ENUMERATE_SUB_KEYS,
                            &hEnumeratorKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Failed to open the enumerator key (Error %lu)\n", dwError);
        return CR_REGISTRY_ERROR;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
        dwDeviceLength = MAX_DEVICE_ID_LEN;
        dwError = RegEnumKeyExW(hEnumeratorKey,
                                dwIndex,
                                szDeviceBuffer,
                                &dwDeviceLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (dwError != ERROR_SUCCESS)
            break;

        wsprintf(szPathBuffer, L"%s\\%s", pszEnumerator, szDeviceBuffer);
        DPRINT("Path: %S\n", szPathBuffer);

        ret = GetDeviceInstanceListSize(szPathBuffer, &dwBufferLength);
        if (ret != CR_SUCCESS)
        {
            *pulLength = 0;
            break;
        }

        *pulLength += dwBufferLength;
    }

    /* Close the enumerator key */
    RegCloseKey(hEnumeratorKey);

    return ret;
}


static
CONFIGRET
GetAllInstanceListSize(
    _Out_ PULONG pulLength)
{
    WCHAR szEnumeratorBuffer[MAX_DEVICE_ID_LEN];
    DWORD dwIndex, dwEnumeratorLength, dwBufferLength;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwEnumeratorLength = MAX_DEVICE_ID_LEN;
        dwError = RegEnumKeyExW(hEnumKey,
                                dwIndex,
                                szEnumeratorBuffer,
                                &dwEnumeratorLength,
                                NULL, NULL, NULL, NULL);
        if (dwError != ERROR_SUCCESS)
            break;

        /* Get the size of all device instances for the enumerator */
        ret = GetEnumeratorInstanceListSize(szEnumeratorBuffer,
                                            &dwBufferLength);
        if (ret != CR_SUCCESS)
            break;

        *pulLength += dwBufferLength;
    }

    return ret;
}


/* Function 11 */
DWORD
WINAPI
PNP_GetDeviceListSize(
    handle_t hBinding,
    LPWSTR pszFilter,
    PNP_RPC_BUFFER_SIZE *pulLength,
    DWORD ulFlags)
{
    WCHAR szEnumerator[MAX_DEVICE_ID_LEN];
    WCHAR szDevice[MAX_DEVICE_ID_LEN];
    WCHAR szInstance[MAX_DEVICE_ID_LEN];
    CONFIGRET ret = CR_SUCCESS;

    DPRINT("PNP_GetDeviceListSize(%p %S %p 0x%lx)\n",
           hBinding, pszFilter, pulLength, ulFlags);

    if (ulFlags & ~CM_GETIDLIST_FILTER_BITS)
        return CR_INVALID_FLAG;

    if (pulLength == NULL)
        return CR_INVALID_POINTER;

    if ((ulFlags != CM_GETIDLIST_FILTER_NONE) &&
        (pszFilter == NULL))
        return CR_INVALID_POINTER;

    *pulLength = 0;

    if (ulFlags &
        (CM_GETIDLIST_FILTER_BUSRELATIONS |
         CM_GETIDLIST_FILTER_POWERRELATIONS |
         CM_GETIDLIST_FILTER_REMOVALRELATIONS |
         CM_GETIDLIST_FILTER_EJECTRELATIONS))
    {
        ret = GetRelationsInstanceListSize(pszFilter,
                                           ulFlags,
                                           pulLength);
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_SERVICE)
    {
        ret = GetServiceInstanceListSize(pszFilter,
                                         pulLength);
    }
    else if (ulFlags & CM_GETIDLIST_FILTER_ENUMERATOR)
    {
        SplitDeviceInstanceID(pszFilter,
                              szEnumerator,
                              szDevice,
                              szInstance);

        if (*szEnumerator != UNICODE_NULL && *szDevice != UNICODE_NULL)
        {
            ret = GetDeviceInstanceListSize(pszFilter,
                                            pulLength);
        }
        else
        {
            ret = GetEnumeratorInstanceListSize(pszFilter,
                                                pulLength);
        }
    }
    else /* CM_GETIDLIST_FILTER_NONE */
    {
        ret = GetAllInstanceListSize(pulLength);
    }

    /* Add one character for the terminating double UNICODE_NULL */
    if (ret == CR_SUCCESS)
        (*pulLength) += 1;

    return ret;
}


/* Function 12 */
DWORD
WINAPI
PNP_GetDepth(
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
DWORD
WINAPI
PNP_GetDeviceRegProp(
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
    HKEY hKey = NULL;
    LONG lError;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetDeviceRegProp() called\n");

    if (pulTransferLen == NULL || pulLength == NULL)
    {
        ret = CR_INVALID_POINTER;
        goto done;
    }

    if (ulFlags != 0)
    {
        ret = CR_INVALID_FLAG;
        goto done;
    }

    /* FIXME: Check pDeviceID */

    if (*pulLength < *pulTransferLen)
        *pulLength = *pulTransferLen;

    *pulTransferLen = 0;

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
            PlugPlayData.Property = PNP_PROPERTY_PHYSICAL_DEVICE_OBJECT_NAME;
            break;

        case CM_DRP_CAPABILITIES:
            lpValueName = L"Capabilities";
            break;

        case CM_DRP_UI_NUMBER:
            PlugPlayData.Property = PNP_PROPERTY_UI_NUMBER;
            break;

        case CM_DRP_UPPERFILTERS:
            lpValueName = L"UpperFilters";
            break;

        case CM_DRP_LOWERFILTERS:
            lpValueName = L"LowerFilters";
            break;

        case CM_DRP_BUSTYPEGUID:
            PlugPlayData.Property = PNP_PROPERTY_BUSTYPEGUID;
            break;

        case CM_DRP_LEGACYBUSTYPE:
            PlugPlayData.Property = PNP_PROPERTY_LEGACYBUSTYPE;
            break;

        case CM_DRP_BUSNUMBER:
            PlugPlayData.Property = PNP_PROPERTY_BUSNUMBER;
            break;

        case CM_DRP_ENUMERATOR_NAME:
            PlugPlayData.Property = PNP_PROPERTY_ENUMERATOR_NAME;
            break;

        case CM_DRP_SECURITY:
            lpValueName = L"Security";
            break;

        case CM_DRP_DEVTYPE:
            lpValueName = L"DeviceType";
            break;

        case CM_DRP_EXCLUSIVE:
            lpValueName = L"Exclusive";
            break;

        case CM_DRP_CHARACTERISTICS:
            lpValueName = L"DeviceCharacteristics";
            break;

        case CM_DRP_ADDRESS:
            PlugPlayData.Property = PNP_PROPERTY_ADDRESS;
            break;

        case CM_DRP_UI_NUMBER_DESC_FORMAT:
            lpValueName = L"UINumberDescFormat";
            break;

        case CM_DRP_DEVICE_POWER_DATA:
            PlugPlayData.Property = PNP_PROPERTY_POWER_DATA;
            break;

        case CM_DRP_REMOVAL_POLICY:
            PlugPlayData.Property = PNP_PROPERTY_REMOVAL_POLICY;
            break;

        case CM_DRP_REMOVAL_POLICY_HW_DEFAULT:
            PlugPlayData.Property = PNP_PROPERTY_REMOVAL_POLICY_HARDWARE_DEFAULT;
            break;

        case CM_DRP_REMOVAL_POLICY_OVERRIDE:
            lpValueName = L"RemovalPolicy";
            break;

        case CM_DRP_INSTALL_STATE:
            PlugPlayData.Property = PNP_PROPERTY_INSTALL_STATE;
            break;

#if (WINVER >= _WIN32_WINNT_WS03)
        case CM_DRP_LOCATION_PATHS:
            PlugPlayData.Property = PNP_PROPERTY_LOCATION_PATHS;
            break;
#endif

#if (WINVER >= _WIN32_WINNT_WIN7)
        case CM_DRP_BASE_CONTAINERID:
            PlugPlayData.Property = PNP_PROPERTY_CONTAINERID;
            break;
#endif

        default:
            ret = CR_INVALID_PROPERTY;
            goto done;
    }

    DPRINT("Value name: %S\n", lpValueName);

    if (lpValueName)
    {
        /* Retrieve information from the Registry */
        lError = RegOpenKeyExW(hEnumKey,
                               pDeviceID,
                               0,
                               KEY_QUERY_VALUE,
                               &hKey);
        if (lError != ERROR_SUCCESS)
        {
            hKey = NULL;
            *pulLength = 0;
            ret = CR_INVALID_DEVNODE;
            goto done;
        }

        lError = RegQueryValueExW(hKey,
                                  lpValueName,
                                  NULL,
                                  pulRegDataType,
                                  Buffer,
                                  pulLength);
        if (lError != ERROR_SUCCESS)
        {
            if (lError == ERROR_MORE_DATA)
            {
                ret = CR_BUFFER_SMALL;
            }
            else
            {
                *pulLength = 0;
                ret = CR_NO_SUCH_VALUE;
            }
        }
    }
    else
    {
        /* Retrieve information from the Device Node */
        RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                             pDeviceID);
        PlugPlayData.Buffer = Buffer;
        PlugPlayData.BufferSize = *pulLength;

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

done:
    if (pulTransferLen)
        *pulTransferLen = (ret == CR_SUCCESS) ? *pulLength : 0;

    if (hKey != NULL)
        RegCloseKey(hKey);

    DPRINT("PNP_GetDeviceRegProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 14 */
DWORD
WINAPI
PNP_SetDeviceRegProp(
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

        case CM_DRP_SECURITY:
            lpValueName = L"Security";
            break;

        case CM_DRP_DEVTYPE:
            lpValueName = L"DeviceType";
            break;

        case CM_DRP_EXCLUSIVE:
            lpValueName = L"Exclusive";
            break;

        case CM_DRP_CHARACTERISTICS:
            lpValueName = L"DeviceCharacteristics";
            break;

        case CM_DRP_UI_NUMBER_DESC_FORMAT:
            lpValueName = L"UINumberDescFormat";
            break;

        case CM_DRP_REMOVAL_POLICY_OVERRIDE:
            lpValueName = L"RemovalPolicy";
            break;

        default:
            return CR_INVALID_PROPERTY;
    }

    DPRINT("Value name: %S\n", lpValueName);

    if (RegOpenKeyExW(hEnumKey,
                      pDeviceId,
                      0,
                      KEY_SET_VALUE,
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
DWORD
WINAPI
PNP_GetClassInstance(
    handle_t hBinding,
    LPWSTR pDeviceId,
    LPWSTR pszClassInstance,
    PNP_RPC_STRING_LEN ulLength)
{
    WCHAR szClassGuid[40];
    WCHAR szClassInstance[5];
    HKEY hDeviceClassKey = NULL;
    HKEY hClassInstanceKey;
    ULONG ulTransferLength, ulDataLength;
    DWORD dwDataType, dwDisposition, i;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    DPRINT("PNP_GetClassInstance(%p %S %p %lu)\n",
           hBinding, pDeviceId, pszClassInstance, ulLength);

    ulTransferLength = ulLength;
    ret = PNP_GetDeviceRegProp(hBinding,
                               pDeviceId,
                               CM_DRP_DRIVER,
                               &dwDataType,
                               (BYTE *)pszClassInstance,
                               &ulTransferLength,
                               &ulLength,
                               0);
    if (ret == CR_SUCCESS)
        return ret;

    ulTransferLength = sizeof(szClassGuid);
    ulDataLength = sizeof(szClassGuid);
    ret = PNP_GetDeviceRegProp(hBinding,
                               pDeviceId,
                               CM_DRP_CLASSGUID,
                               &dwDataType,
                               (BYTE *)szClassGuid,
                               &ulTransferLength,
                               &ulDataLength,
                               0);
    if (ret != CR_SUCCESS)
    {
        DPRINT1("PNP_GetDeviceRegProp() failed (Error %lu)\n", ret);
        goto done;
    }

    dwError = RegOpenKeyExW(hClassKey,
                            szClassGuid,
                            0,
                            KEY_READ,
                            &hDeviceClassKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %lu)\n", dwError);
        ret = CR_FAILURE;
        goto done;
    }

    for (i = 0; i < 10000; i++)
    {
        wsprintf(szClassInstance, L"%04lu", i);

        dwError = RegCreateKeyExW(hDeviceClassKey,
                                  szClassInstance,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hClassInstanceKey,
                                  &dwDisposition);
        if (dwError == ERROR_SUCCESS)
        {
            RegCloseKey(hClassInstanceKey);

            if (dwDisposition == REG_CREATED_NEW_KEY)
            {
                wsprintf(pszClassInstance,
                         L"%s\\%s",
                         szClassGuid,
                         szClassInstance);

                ulDataLength = (wcslen(pszClassInstance) + 1) * sizeof(WCHAR);
                ret = PNP_SetDeviceRegProp(hBinding,
                                           pDeviceId,
                                           CM_DRP_DRIVER,
                                           REG_SZ,
                                           (BYTE *)pszClassInstance,
                                           ulDataLength,
                                           0);
                if (ret != CR_SUCCESS)
                {
                    DPRINT1("PNP_SetDeviceRegProp() failed (Error %lu)\n", ret);
                    RegDeleteKeyW(hDeviceClassKey,
                                  szClassInstance);
                }

                break;
            }
        }
    }

done:
    if (hDeviceClassKey != NULL)
        RegCloseKey(hDeviceClassKey);

    return ret;
}


/* Function 16 */
DWORD
WINAPI
PNP_CreateKey(
    handle_t hBinding,
    LPWSTR pszSubKey,
    DWORD samDesired,
    DWORD ulFlags)
{
    HKEY hKey = 0;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        pszSubKey,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL))
        return CR_REGISTRY_ERROR;

    /* FIXME: Set security key */

    RegCloseKey(hKey);

    return CR_SUCCESS;
}


/* Function 17 */
DWORD
WINAPI
PNP_DeleteRegistryKey(
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
DWORD
WINAPI
PNP_GetClassCount(
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
DWORD
WINAPI
PNP_GetClassName(
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
    if (lstrlenW(pszClassGuid) + 1 < sizeof(szKeyName)/sizeof(WCHAR)-(lstrlenW(szKeyName) * sizeof(WCHAR)))
        lstrcatW(szKeyName, pszClassGuid);
    else
        return CR_INVALID_DATA;

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
DWORD
WINAPI
PNP_DeleteClassKey(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetClassName(%S, %lx) called\n", pszClassGuid, ulFlags);

    if (ulFlags & CM_DELETE_CLASS_SUBKEYS)
    {
        if (SHDeleteKeyW(hClassKey, pszClassGuid) != ERROR_SUCCESS)
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
DWORD
WINAPI
PNP_GetInterfaceDeviceAlias(
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
DWORD
WINAPI
PNP_GetInterfaceDeviceList(
    handle_t hBinding,
    GUID *InterfaceGuid,
    LPWSTR pszDeviceID,
    BYTE *Buffer,
    PNP_RPC_BUFFER_SIZE *pulLength,
    DWORD ulFlags)
{
    NTSTATUS Status;
    PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA PlugPlayData;
    DWORD ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pszDeviceID);

    PlugPlayData.Flags = ulFlags;
    PlugPlayData.FilterGuid = InterfaceGuid;
    PlugPlayData.Buffer = Buffer;
    PlugPlayData.BufferSize = *pulLength;

    Status = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulLength = PlugPlayData.BufferSize;
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetInterfaceDeviceListSize() done (returns %lx)\n", ret);
    return ret;
}


/* Function 23 */
DWORD
WINAPI
PNP_GetInterfaceDeviceListSize(
    handle_t hBinding,
    PNP_RPC_BUFFER_SIZE *pulLen,
    GUID *InterfaceGuid,
    LPWSTR pszDeviceID,
    DWORD ulFlags)
{
    NTSTATUS Status;
    PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA PlugPlayData;
    DWORD ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetInterfaceDeviceListSize() called\n");

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance,
                         pszDeviceID);

    PlugPlayData.FilterGuid = InterfaceGuid;
    PlugPlayData.Buffer = NULL;
    PlugPlayData.BufferSize = 0;
    PlugPlayData.Flags = ulFlags;

    Status = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                               (PVOID)&PlugPlayData,
                               sizeof(PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA));
    if (NT_SUCCESS(Status))
    {
        *pulLen = PlugPlayData.BufferSize;
    }
    else
    {
        ret = NtStatusToCrError(Status);
    }

    DPRINT("PNP_GetInterfaceDeviceListSize() done (returns %lx)\n", ret);
    return ret;
}


/* Function 24 */
DWORD
WINAPI
PNP_RegisterDeviceClassAssociation(
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
DWORD
WINAPI
PNP_UnregisterDeviceClassAssociation(
    handle_t hBinding,
    LPWSTR pszInterfaceDevice,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 26 */
DWORD
WINAPI
PNP_GetClassRegProp(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    DWORD ulProperty,
    DWORD *pulRegDataType,
    BYTE *Buffer,
    PNP_RPC_STRING_LEN *pulTransferLen,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hInstKey = NULL;
    HKEY hPropKey = NULL;
    LONG lError;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetClassRegProp() called\n");

    if (pulTransferLen == NULL || pulLength == NULL)
    {
        ret = CR_INVALID_POINTER;
        goto done;
    }

    if (ulFlags != 0)
    {
        ret = CR_INVALID_FLAG;
        goto done;
    }

    if (*pulLength < *pulTransferLen)
        *pulLength = *pulTransferLen;

    *pulTransferLen = 0;

    switch (ulProperty)
    {
        case CM_CRP_SECURITY:
            lpValueName = L"Security";
            break;

        case CM_CRP_DEVTYPE:
            lpValueName = L"DeviceType";
            break;

        case CM_CRP_EXCLUSIVE:
            lpValueName = L"Exclusive";
            break;

        case CM_CRP_CHARACTERISTICS:
            lpValueName = L"DeviceCharacteristics";
            break;

        default:
            ret = CR_INVALID_PROPERTY;
            goto done;
    }

    DPRINT("Value name: %S\n", lpValueName);

    lError = RegOpenKeyExW(hClassKey,
                           pszClassGuid,
                           0,
                           KEY_READ,
                           &hInstKey);
    if (lError != ERROR_SUCCESS)
    {
        *pulLength = 0;
        ret = CR_NO_SUCH_REGISTRY_KEY;
        goto done;
    }

    lError = RegOpenKeyExW(hInstKey,
                           L"Properties",
                           0,
                           KEY_READ,
                           &hPropKey);
    if (lError != ERROR_SUCCESS)
    {
        *pulLength = 0;
        ret = CR_NO_SUCH_REGISTRY_KEY;
        goto done;
    }

    lError = RegQueryValueExW(hPropKey,
                              lpValueName,
                              NULL,
                              pulRegDataType,
                              Buffer,
                              pulLength);
    if (lError != ERROR_SUCCESS)
    {
        if (lError == ERROR_MORE_DATA)
        {
            ret = CR_BUFFER_SMALL;
        }
        else
        {
            *pulLength = 0;
            ret = CR_NO_SUCH_VALUE;
        }
    }

done:
    if (ret == CR_SUCCESS)
        *pulTransferLen = *pulLength;

    if (hPropKey != NULL)
        RegCloseKey(hPropKey);

    if (hInstKey != NULL)
        RegCloseKey(hInstKey);

    DPRINT("PNP_GetClassRegProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 27 */
DWORD
WINAPI
PNP_SetClassRegProp(
    handle_t hBinding,
    LPWSTR pszClassGuid,
    DWORD ulProperty,
    DWORD ulDataType,
    BYTE *Buffer,
    PNP_PROP_SIZE ulLength,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpValueName = NULL;
    HKEY hInstKey = 0;
    HKEY hPropKey = 0;
    LONG lError;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_SetClassRegProp() called\n");

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    switch (ulProperty)
    {
        case CM_CRP_SECURITY:
            lpValueName = L"Security";
            break;

        case CM_CRP_DEVTYPE:
            lpValueName = L"DeviceType";
            break;

        case CM_CRP_EXCLUSIVE:
            lpValueName = L"Exclusive";
            break;

        case CM_CRP_CHARACTERISTICS:
            lpValueName = L"DeviceCharacteristics";
            break;

        default:
            return CR_INVALID_PROPERTY;
    }

    lError = RegOpenKeyExW(hClassKey,
                           pszClassGuid,
                           0,
                           KEY_WRITE,
                           &hInstKey);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_NO_SUCH_REGISTRY_KEY;
        goto done;
    }

    /* FIXME: Set security descriptor */
    lError = RegCreateKeyExW(hInstKey,
                             L"Properties",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hPropKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    if (ulLength == 0)
    {
        if (RegDeleteValueW(hPropKey,
                            lpValueName))
            ret = CR_REGISTRY_ERROR;
    }
    else
    {
        if (RegSetValueExW(hPropKey,
                           lpValueName,
                           0,
                           ulDataType,
                           Buffer,
                           ulLength))
            ret = CR_REGISTRY_ERROR;
    }

done:
    if (hPropKey != NULL)
        RegCloseKey(hPropKey);

    if (hInstKey != NULL)
        RegCloseKey(hInstKey);

    return ret;
}


static CONFIGRET
CreateDeviceInstance(LPWSTR pszDeviceID)
{
    WCHAR szEnumerator[MAX_DEVICE_ID_LEN];
    WCHAR szDevice[MAX_DEVICE_ID_LEN];
    WCHAR szInstance[MAX_DEVICE_ID_LEN];
    HKEY hKeyEnumerator;
    HKEY hKeyDevice;
    HKEY hKeyInstance;
    HKEY hKeyControl;
    LONG lError;

    /* Split the instance ID */
    SplitDeviceInstanceID(pszDeviceID,
                          szEnumerator,
                          szDevice,
                          szInstance);

    /* Open or create the enumerator key */
    lError = RegCreateKeyExW(hEnumKey,
                             szEnumerator,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyEnumerator,
                             NULL);
    if (lError != ERROR_SUCCESS)
    {
        return CR_REGISTRY_ERROR;
    }

    /* Open or create the device key */
    lError = RegCreateKeyExW(hKeyEnumerator,
                             szDevice,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyDevice,
                             NULL);

    /* Close the enumerator key */
    RegCloseKey(hKeyEnumerator);

    if (lError != ERROR_SUCCESS)
    {
        return CR_REGISTRY_ERROR;
    }

    /* Try to open the instance key and fail if it exists */
    lError = RegOpenKeyExW(hKeyDevice,
                           szInstance,
                           0,
                           KEY_SET_VALUE,
                           &hKeyInstance);
    if (lError == ERROR_SUCCESS)
    {
        DPRINT1("Instance %S already exists!\n", szInstance);
        RegCloseKey(hKeyInstance);
        RegCloseKey(hKeyDevice);
        return CR_ALREADY_SUCH_DEVINST;
    }

    /* Create a new instance key */
    lError = RegCreateKeyExW(hKeyDevice,
                             szInstance,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyInstance,
                             NULL);

    /* Close the device key */
    RegCloseKey(hKeyDevice);

    if (lError != ERROR_SUCCESS)
    {
        return CR_REGISTRY_ERROR;
    }

    /* Create the 'Control' sub key */
    lError = RegCreateKeyExW(hKeyInstance,
                             L"Control",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyControl,
                             NULL);
    if (lError == ERROR_SUCCESS)
    {
        RegCloseKey(hKeyControl);
    }

    RegCloseKey(hKeyInstance);

    return (lError == ERROR_SUCCESS) ? CR_SUCCESS : CR_REGISTRY_ERROR;
}


/* Function 28 */
DWORD
WINAPI
PNP_CreateDevInst(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    LPWSTR pszParentDeviceID,
    PNP_RPC_STRING_LEN ulLength,
    DWORD ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    DPRINT("PNP_CreateDevInst: %S\n", pszDeviceID);

    if (ulFlags & CM_CREATE_DEVNODE_GENERATE_ID)
    {
        WCHAR szGeneratedInstance[MAX_DEVICE_ID_LEN];
        DWORD dwInstanceNumber;

        /* Generated ID is: Root\<Device ID>\<Instance number> */
        dwInstanceNumber = 0;
        do
        {
            swprintf(szGeneratedInstance, L"Root\\%ls\\%04lu",
                     pszDeviceID, dwInstanceNumber);

            /* Try to create a device instance with this ID */
            ret = CreateDeviceInstance(szGeneratedInstance);

            dwInstanceNumber++;
        }
        while (ret == CR_ALREADY_SUCH_DEVINST);

        if (ret == CR_SUCCESS)
        {
            /* pszDeviceID is an out parameter too for generated IDs */
            if (wcslen(szGeneratedInstance) > ulLength)
            {
                ret = CR_BUFFER_SMALL;
            }
            else
            {
                wcscpy(pszDeviceID, szGeneratedInstance);
            }
        }
    }
    else
    {
        /* Create the device instance */
        ret = CreateDeviceInstance(pszDeviceID);
    }

    DPRINT("PNP_CreateDevInst() done (returns %lx)\n", ret);

    return ret;
}


static CONFIGRET
MoveDeviceInstance(LPWSTR pszDeviceInstanceDestination,
                   LPWSTR pszDeviceInstanceSource)
{
    DPRINT("MoveDeviceInstance: not implemented\n");
    /* FIXME */
    return CR_CALL_NOT_IMPLEMENTED;
}


static CONFIGRET
SetupDeviceInstance(LPWSTR pszDeviceInstance,
                    DWORD ulFlags)
{
    DPRINT("SetupDeviceInstance: not implemented\n");
    /* FIXME */
    return CR_CALL_NOT_IMPLEMENTED;
}


static CONFIGRET
EnableDeviceInstance(LPWSTR pszDeviceInstance)
{
    PLUGPLAY_CONTROL_RESET_DEVICE_DATA ResetDeviceData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT("Enable device instance %S\n", pszDeviceInstance);

    RtlInitUnicodeString(&ResetDeviceData.DeviceInstance, pszDeviceInstance);
    Status = NtPlugPlayControl(PlugPlayControlResetDevice, &ResetDeviceData, sizeof(PLUGPLAY_CONTROL_RESET_DEVICE_DATA));
    if (!NT_SUCCESS(Status))
        ret = NtStatusToCrError(Status);

    return ret;
}


static CONFIGRET
DisableDeviceInstance(LPWSTR pszDeviceInstance)
{
    DPRINT("DisableDeviceInstance: not implemented\n");
    /* FIXME */
    return CR_CALL_NOT_IMPLEMENTED;
}


static CONFIGRET
ReenumerateDeviceInstance(
    _In_ LPWSTR pszDeviceInstance,
    _In_ ULONG ulFlags)
{
    PLUGPLAY_CONTROL_ENUMERATE_DEVICE_DATA EnumerateDeviceData;
    CONFIGRET ret = CR_SUCCESS;
    NTSTATUS Status;

    DPRINT1("ReenumerateDeviceInstance(%S 0x%08lx)\n",
           pszDeviceInstance, ulFlags);

    if (ulFlags & ~CM_REENUMERATE_BITS)
        return CR_INVALID_FLAG;

    if (ulFlags & CM_REENUMERATE_RETRY_INSTALLATION)
    {
        DPRINT1("CM_REENUMERATE_RETRY_INSTALLATION not implemented!\n");
    }

    RtlInitUnicodeString(&EnumerateDeviceData.DeviceInstance,
                         pszDeviceInstance);
    EnumerateDeviceData.Flags = 0;

    Status = NtPlugPlayControl(PlugPlayControlEnumerateDevice,
                               &EnumerateDeviceData,
                               sizeof(PLUGPLAY_CONTROL_ENUMERATE_DEVICE_DATA));
    if (!NT_SUCCESS(Status))
        ret = NtStatusToCrError(Status);

    return ret;
}


/* Function 29 */
DWORD
WINAPI
PNP_DeviceInstanceAction(
    handle_t hBinding,
    DWORD ulAction,
    DWORD ulFlags,
    LPWSTR pszDeviceInstance1,
    LPWSTR pszDeviceInstance2)
{
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_DeviceInstanceAction() called\n");

    switch (ulAction)
    {
        case PNP_DEVINST_MOVE:
            ret = MoveDeviceInstance(pszDeviceInstance1,
                                     pszDeviceInstance2);
            break;

        case PNP_DEVINST_SETUP:
            ret = SetupDeviceInstance(pszDeviceInstance1,
                                      ulFlags);
            break;

        case PNP_DEVINST_ENABLE:
            ret = EnableDeviceInstance(pszDeviceInstance1);
            break;

        case PNP_DEVINST_DISABLE:
            ret = DisableDeviceInstance(pszDeviceInstance1);
            break;

        case PNP_DEVINST_REENUMERATE:
            ret = ReenumerateDeviceInstance(pszDeviceInstance1,
                                            ulFlags);
            break;

        default:
            DPRINT1("Unknown device action %lu: not implemented\n", ulAction);
            ret = CR_CALL_NOT_IMPLEMENTED;
    }

    DPRINT("PNP_DeviceInstanceAction() done (returns %lx)\n", ret);

    return ret;
}


/* Function 30 */
DWORD
WINAPI
PNP_GetDeviceStatus(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD *pulStatus,
    DWORD *pulProblem,
    DWORD ulFlags)
{
    UNREFERENCED_PARAMETER(hBinding);
    UNREFERENCED_PARAMETER(ulFlags);

    DPRINT("PNP_GetDeviceStatus(%p %S %p %p)\n",
           hBinding, pDeviceID, pulStatus, pulProblem, ulFlags);

    return GetDeviceStatus(pDeviceID, pulStatus, pulProblem);
}


/* Function 31 */
DWORD
WINAPI
PNP_SetDeviceProblem(
    handle_t hBinding,
    LPWSTR pDeviceID,
    DWORD ulProblem,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 32 */
DWORD
WINAPI
PNP_DisableDevInst(
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
DWORD
WINAPI
PNP_UninstallDevInst(
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
DWORD
WINAPI
PNP_AddID(
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
DWORD
WINAPI
PNP_RegisterDriver(
    handle_t hBinding,
    LPWSTR pszDeviceID,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 36 */
DWORD
WINAPI
PNP_QueryRemove(
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
DWORD
WINAPI
PNP_RequestDeviceEject(
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
WINAPI
PNP_IsDockStationPresent(
    handle_t hBinding,
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
DWORD
WINAPI
PNP_RequestEjectPC(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 40 */
DWORD
WINAPI
PNP_HwProfFlags(
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
    CONFIGRET ret = CR_SUCCESS;
    WCHAR szKeyName[MAX_PATH];
    HKEY hKey;
    HKEY hDeviceKey;
    DWORD dwSize;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_HwProfFlags() called\n");

    if (ulConfig == 0)
    {
        wcscpy(szKeyName,
               L"System\\CurrentControlSet\\HardwareProfiles\\Current\\System\\CurrentControlSet\\Enum");
    }
    else
    {
        swprintf(szKeyName,
                 L"System\\CurrentControlSet\\HardwareProfiles\\%04lu\\System\\CurrentControlSet\\Enum",
                 ulConfig);
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      szKeyName,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
        return CR_REGISTRY_ERROR;

    if (ulAction == PNP_GET_HWPROFFLAGS)
    {
         if (RegOpenKeyExW(hKey,
                           pDeviceID,
                           0,
                           KEY_QUERY_VALUE,
                           &hDeviceKey) != ERROR_SUCCESS)
         {
            *pulValue = 0;
         }
         else
         {
             dwSize = sizeof(DWORD);
             if (RegQueryValueExW(hDeviceKey,
                                  L"CSConfigFlags",
                                  NULL,
                                  NULL,
                                  (LPBYTE)pulValue,
                                  &dwSize) != ERROR_SUCCESS)
             {
                 *pulValue = 0;
             }

             RegCloseKey(hDeviceKey);
         }
    }
    else if (ulAction == PNP_SET_HWPROFFLAGS)
    {
        /* FIXME: not implemented yet */
        ret = CR_CALL_NOT_IMPLEMENTED;
    }

    RegCloseKey(hKey);

    return ret;
}


/* Function 41 */
DWORD
WINAPI
PNP_GetHwProfInfo(
    handle_t hBinding,
    DWORD ulIndex,
    HWPROFILEINFO *pHWProfileInfo,
    DWORD ulProfileInfoSize,
    DWORD ulFlags)
{
    WCHAR szProfileName[5];
    HKEY hKeyConfig = NULL;
    HKEY hKeyProfiles = NULL;
    HKEY hKeyProfile = NULL;
    DWORD dwDisposition;
    DWORD dwSize;
    LONG lError;
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetHwProfInfo() called\n");

    if (ulProfileInfoSize == 0)
    {
        ret = CR_INVALID_DATA;
        goto done;
    }

    if (ulFlags != 0)
    {
        ret = CR_INVALID_FLAG;
        goto done;
    }

    /* Initialize the profile information */
    pHWProfileInfo->HWPI_ulHWProfile = 0;
    pHWProfileInfo->HWPI_szFriendlyName[0] = 0;
    pHWProfileInfo->HWPI_dwFlags = 0;

    /* Open the 'IDConfigDB' key */
    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Control\\IDConfigDB",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_QUERY_VALUE,
                             NULL,
                             &hKeyConfig,
                             &dwDisposition);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    /* Open the 'Hardware Profiles' subkey */
    lError = RegCreateKeyExW(hKeyConfig,
                             L"Hardware Profiles",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                             NULL,
                             &hKeyProfiles,
                             &dwDisposition);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    if (ulIndex == (ULONG)-1)
    {
        dwSize = sizeof(ULONG);
        lError = RegQueryValueExW(hKeyConfig,
                                  L"CurrentConfig",
                                  NULL,
                                  NULL,
                                  (LPBYTE)&pHWProfileInfo->HWPI_ulHWProfile,
                                  &dwSize);
        if (lError != ERROR_SUCCESS)
        {
            pHWProfileInfo->HWPI_ulHWProfile = 0;
            ret = CR_REGISTRY_ERROR;
            goto done;
        }
    }
    else
    {
        /* FIXME: not implemented yet */
        ret = CR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

    swprintf(szProfileName, L"%04lu", pHWProfileInfo->HWPI_ulHWProfile);

    lError = RegOpenKeyExW(hKeyProfiles,
                           szProfileName,
                           0,
                           KEY_QUERY_VALUE,
                           &hKeyProfile);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    dwSize = sizeof(pHWProfileInfo->HWPI_szFriendlyName);
    lError = RegQueryValueExW(hKeyProfile,
                              L"FriendlyName",
                              NULL,
                              NULL,
                              (LPBYTE)&pHWProfileInfo->HWPI_szFriendlyName,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

done:
    if (hKeyProfile != NULL)
        RegCloseKey(hKeyProfile);

    if (hKeyProfiles != NULL)
        RegCloseKey(hKeyProfiles);

    if (hKeyConfig != NULL)
        RegCloseKey(hKeyConfig);

    return ret;
}


/* Function 42 */
DWORD
WINAPI
PNP_AddEmptyLogConf(
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
DWORD
WINAPI
PNP_FreeLogConf(
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
DWORD
WINAPI
PNP_GetFirstLogConf(
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
DWORD
WINAPI
PNP_GetNextLogConf(
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
DWORD
WINAPI
PNP_GetLogConfPriority(
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
DWORD
WINAPI
PNP_AddResDes(
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
DWORD
WINAPI
PNP_FreeResDes(
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
DWORD
WINAPI
PNP_GetNextResDes(
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
DWORD
WINAPI
PNP_GetResDesData(
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
DWORD
WINAPI
PNP_GetResDesDataSize(
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
DWORD
WINAPI
PNP_ModifyResDes(
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
DWORD
WINAPI
PNP_DetectResourceConflict(
    handle_t hBinding,
    LPWSTR pDeviceID,
    RESOURCEID ResourceID,
    BYTE *ResourceData,
    PNP_RPC_BUFFER_SIZE ResourceLen,
    BOOL *pbConflictDetected,
    DWORD ulFlags)
{
    DPRINT("PNP_DetectResourceConflict()\n");

    if (pbConflictDetected != NULL)
        *pbConflictDetected = FALSE;

    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 54 */
DWORD
WINAPI
PNP_QueryResConfList(
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
DWORD
WINAPI
PNP_SetHwProf(
    handle_t hBinding,
    DWORD ulHardwareProfile,
    DWORD ulFlags)
{
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 56 */
DWORD
WINAPI
PNP_QueryArbitratorFreeData(
    handle_t hBinding,
    BYTE *pData,
    DWORD DataLen,
    LPWSTR pDeviceID,
    RESOURCEID ResourceID,
    DWORD ulFlags)
{
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 57 */
DWORD
WINAPI
PNP_QueryArbitratorFreeSize(
    handle_t hBinding,
    DWORD *pulSize,
    LPWSTR pDeviceID,
    RESOURCEID ResourceID,
    DWORD ulFlags)
{
    if (pulSize != NULL)
        *pulSize = 0;

    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 58 */
CONFIGRET
WINAPI
PNP_RunDetection(
    handle_t hBinding,
    DWORD ulFlags)
{
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 59 */
DWORD
WINAPI
PNP_RegisterNotification(
    handle_t hBinding,
    DWORD ulUnknown2,
    LPWSTR pszName,
    BYTE *pNotificationFilter,
    DWORD ulNotificationFilterSize,
    DWORD ulFlags,
    DWORD *pulNotify,
    DWORD ulUnknown8,
    DWORD *pulUnknown9)
{
    PDEV_BROADCAST_DEVICEINTERFACE_W pBroadcastDeviceInterface;
    PDEV_BROADCAST_HANDLE pBroadcastDeviceHandle;
#if 0
    PNOTIFY_DATA pNotifyData;
#endif

    DPRINT1("PNP_RegisterNotification(%p %lx '%S' %p %lu 0x%lx %p %lx %p)\n",
           hBinding, ulUnknown2, pszName, pNotificationFilter,
           ulNotificationFilterSize, ulFlags, pulNotify, ulUnknown8, pulUnknown9);

    if (pNotificationFilter == NULL ||
        pulNotify == NULL ||
        pulUnknown9 == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~0x7)
        return CR_INVALID_FLAG;

    if ((ulNotificationFilterSize < sizeof(DEV_BROADCAST_HDR)) ||
        (((PDEV_BROADCAST_HDR)pNotificationFilter)->dbch_size < sizeof(DEV_BROADCAST_HDR)))
        return CR_INVALID_DATA;

    if (((PDEV_BROADCAST_HDR)pNotificationFilter)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
        DPRINT1("DBT_DEVTYP_DEVICEINTERFACE\n");
        pBroadcastDeviceInterface = (PDEV_BROADCAST_DEVICEINTERFACE_W)pNotificationFilter;

        if ((ulNotificationFilterSize < sizeof(DEV_BROADCAST_DEVICEINTERFACE_W)) ||
            (pBroadcastDeviceInterface->dbcc_size < sizeof(DEV_BROADCAST_DEVICEINTERFACE_W)))
            return CR_INVALID_DATA;
    }
    else if (((PDEV_BROADCAST_HDR)pNotificationFilter)->dbch_devicetype == DBT_DEVTYP_HANDLE)
    {
        DPRINT1("DBT_DEVTYP_HANDLE\n");
        pBroadcastDeviceHandle = (PDEV_BROADCAST_HANDLE)pNotificationFilter;

        if ((ulNotificationFilterSize < sizeof(DEV_BROADCAST_HANDLE)) ||
            (pBroadcastDeviceHandle->dbch_size < sizeof(DEV_BROADCAST_HANDLE)))
            return CR_INVALID_DATA;

        if (ulFlags & DEVICE_NOTIFY_ALL_INTERFACE_CLASSES)
            return CR_INVALID_FLAG;
    }
    else
    {
        DPRINT1("Invalid device type %lu\n", ((PDEV_BROADCAST_HDR)pNotificationFilter)->dbch_devicetype);
        return CR_INVALID_DATA;
    }


#if 0
    pNotifyData = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NOTIFY_DATA));
    if (pNotifyData == NULL)
        return CR_OUT_OF_MEMORY;

    *pulNotify = (DWORD)pNotifyData;
#endif

    *pulNotify = 1;

    return CR_SUCCESS;
}


/* Function 60 */
DWORD
WINAPI
PNP_UnregisterNotification(
    handle_t hBinding,
    DWORD ulNotify)
{
    DPRINT1("PNP_UnregisterNotification(%p 0x%lx)\n",
           hBinding, ulNotify);

#if 0
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
#endif

    return CR_SUCCESS;
}


/* Function 61 */
DWORD
WINAPI
PNP_GetCustomDevProp(
    handle_t hBinding,
    LPWSTR pDeviceID,
    LPWSTR CustomPropName,
    DWORD *pulRegDataType,
    BYTE *Buffer,
    PNP_RPC_STRING_LEN *pulTransferLen,
    PNP_RPC_STRING_LEN *pulLength,
    DWORD ulFlags)
{
    HKEY hDeviceKey = NULL;
    HKEY hParamKey = NULL;
    LONG lError;
    CONFIGRET ret = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    DPRINT("PNP_GetCustomDevProp() called\n");

    if (pulTransferLen == NULL || pulLength == NULL)
    {
        ret = CR_INVALID_POINTER;
        goto done;
    }

    if (ulFlags & ~CM_CUSTOMDEVPROP_BITS)
    {
        ret = CR_INVALID_FLAG;
        goto done;
    }

    if (*pulLength < *pulTransferLen)
        *pulLength = *pulTransferLen;

    *pulTransferLen = 0;

    lError = RegOpenKeyExW(hEnumKey,
                           pDeviceID,
                           0,
                           KEY_READ,
                           &hDeviceKey);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    lError = RegOpenKeyExW(hDeviceKey,
                           L"Device Parameters",
                           0,
                           KEY_READ,
                           &hParamKey);
    if (lError != ERROR_SUCCESS)
    {
        ret = CR_REGISTRY_ERROR;
        goto done;
    }

    lError = RegQueryValueExW(hParamKey,
                              CustomPropName,
                              NULL,
                              pulRegDataType,
                              Buffer,
                              pulLength);
    if (lError != ERROR_SUCCESS)
    {
        if (lError == ERROR_MORE_DATA)
        {
            ret = CR_BUFFER_SMALL;
        }
        else
        {
            *pulLength = 0;
            ret = CR_NO_SUCH_VALUE;
        }
    }

done:
    if (ret == CR_SUCCESS)
        *pulTransferLen = *pulLength;

    if (hParamKey != NULL)
        RegCloseKey(hParamKey);

    if (hDeviceKey != NULL)
        RegCloseKey(hDeviceKey);

    DPRINT("PNP_GetCustomDevProp() done (returns %lx)\n", ret);

    return ret;
}


/* Function 62 */
DWORD
WINAPI
PNP_GetVersionInternal(
    handle_t hBinding,
    WORD *pwVersion)
{
    UNREFERENCED_PARAMETER(hBinding);

    *pwVersion = 0x501;
    return CR_SUCCESS;
}


/* Function 63 */
DWORD
WINAPI
PNP_GetBlockedDriverInfo(
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
DWORD
WINAPI
PNP_GetServerSideDeviceInstallFlags(
    handle_t hBinding,
    DWORD *pulSSDIFlags,
    DWORD ulFlags)
{
    UNREFERENCED_PARAMETER(hBinding);

    DPRINT1("PNP_GetServerSideDeviceInstallFlags(%p %p %lu)\n",
            hBinding, pulSSDIFlags, ulFlags);

    if (pulSSDIFlags == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    /* FIXME */
    *pulSSDIFlags = 0;

    return CR_SUCCESS;
}


/* Function 65 */
DWORD
WINAPI
PNP_GetObjectPropKeys(
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
DWORD
WINAPI
PNP_GetObjectProp(
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
DWORD
WINAPI
PNP_SetObjectProp(
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
DWORD
WINAPI
PNP_InstallDevInst(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 69 */
DWORD
WINAPI
PNP_ApplyPowerSettings(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 70 */
DWORD
WINAPI
PNP_DriverStoreAddDriverPackage(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 71 */
DWORD
WINAPI
PNP_DriverStoreDeleteDriverPackage(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 72 */
DWORD
WINAPI
PNP_RegisterServiceNotification(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 73 */
DWORD
WINAPI
PNP_SetActiveService(
    handle_t hBinding,
    LPWSTR pszFilter,
    DWORD ulFlags)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}


/* Function 74 */
DWORD
WINAPI
PNP_DeleteServiceDevices(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return CR_CALL_NOT_IMPLEMENTED;
}
