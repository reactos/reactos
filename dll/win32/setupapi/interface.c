/*
 * SetupAPI interface-related functions
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
 *           2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR AddInterface[]  = {'A','d','d','I','n','t','e','r','f','a','c','e',0};
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR Control[]  = {'C','o','n','t','r','o','l',0};
static const WCHAR DeviceInstance[]  = {'D','e','v','i','c','e','I','n','s','t','a','n','c','e',0};
static const WCHAR DotInterfaces[]  = {'.','I','n','t','e','r','f','a','c','e','s',0};
static const WCHAR Linked[]  = {'L','i','n','k','e','d',0};
static const WCHAR SymbolicLink[]  = {'S','y','m','b','o','l','i','c','L','i','n','k',0};

static BOOL
CreateDeviceInterface(
    IN struct DeviceInfoElement* deviceInfo,
    IN LPCWSTR SymbolicLink,
    IN LPCGUID pInterfaceGuid,
    OUT struct DeviceInterface **pDeviceInterface)
{
    struct DeviceInterface *deviceInterface;

    *pDeviceInterface = NULL;

    deviceInterface = HeapAlloc(GetProcessHeap(), 0,
        FIELD_OFFSET(struct DeviceInterface, SymbolicLink) + (strlenW(SymbolicLink) + 1) * sizeof(WCHAR));
    if (!deviceInterface)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    deviceInterface->DeviceInfo = deviceInfo;
    strcpyW(deviceInterface->SymbolicLink, SymbolicLink);
    deviceInterface->Flags = 0; /* Flags will be updated later */
    memcpy(&deviceInterface->InterfaceClassGuid, pInterfaceGuid, sizeof(GUID));

    *pDeviceInterface = deviceInterface;
    return TRUE;
}

BOOL
DestroyDeviceInterface(
    struct DeviceInterface* deviceInterface)
{
    return HeapFree(GetProcessHeap(), 0, deviceInterface);
}

LONG
SETUP_CreateInterfaceList(
    struct DeviceInfoSet *list,
    PCWSTR MachineName,
    CONST GUID *InterfaceGuid,
    PCWSTR DeviceInstanceW /* OPTIONAL */,
    BOOL OnlyPresentInterfaces)
{
    HKEY hInterfaceKey;      /* HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID} */
    HKEY hDeviceInstanceKey; /* HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID}\##?#{InstancePath} */
    HKEY hReferenceKey;      /* HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID}\##?#{InstancePath}\#{ReferenceString} */
    HKEY hControlKey;        /* HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID}\##?#{InstancePath}\#{ReferenceString}\Control */
    HKEY hEnumKey;           /* HKLM\SYSTEM\CurrentControlSet\Enum */
    HKEY hKey;               /* HKLM\SYSTEM\CurrentControlSet\Enum\{Instance\Path} */
    LONG rc;
    WCHAR KeyBuffer[max(MAX_PATH, MAX_GUID_STRING_LEN) + 1];
    PWSTR pSymbolicLink = NULL;
    PWSTR InstancePath = NULL;
    DWORD i, j;
    DWORD dwLength, dwInstancePathLength;
    DWORD dwRegType;
    DWORD LinkedValue;
    GUID ClassGuid;
    struct DeviceInfoElement *deviceInfo;

    hInterfaceKey = INVALID_HANDLE_VALUE;
    hDeviceInstanceKey = NULL;
    hReferenceKey = NULL;

    /* Open registry key related to this interface */
    hInterfaceKey = SetupDiOpenClassRegKeyExW(InterfaceGuid, KEY_ENUMERATE_SUB_KEYS, DIOCR_INTERFACE, MachineName, NULL);
    if (hInterfaceKey == INVALID_HANDLE_VALUE)
    {
        rc = GetLastError();
        goto cleanup;
    }

    /* Enumerate sub keys of hInterfaceKey */
    i = 0;
    while (TRUE)
    {
        dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
        rc = RegEnumKeyExW(hInterfaceKey, i, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
        if (rc == ERROR_NO_MORE_ITEMS)
            break;
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        i++;

        /* Open sub key */
        if (hDeviceInstanceKey != NULL)
            RegCloseKey(hDeviceInstanceKey);
        rc = RegOpenKeyExW(hInterfaceKey, KeyBuffer, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hDeviceInstanceKey);
        if (rc != ERROR_SUCCESS)
            goto cleanup;

        /* Read DeviceInstance */
        rc = RegQueryValueExW(hDeviceInstanceKey, DeviceInstance, NULL, &dwRegType, NULL, &dwInstancePathLength);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        if (dwRegType != REG_SZ)
        {
            rc = ERROR_GEN_FAILURE;
            goto cleanup;
        }
        HeapFree(GetProcessHeap(), 0, InstancePath);
        InstancePath = HeapAlloc(GetProcessHeap(), 0, dwInstancePathLength + sizeof(WCHAR));
        if (!InstancePath)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        rc = RegQueryValueExW(hDeviceInstanceKey, DeviceInstance, NULL, NULL, (LPBYTE)InstancePath, &dwInstancePathLength);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        InstancePath[dwInstancePathLength / sizeof(WCHAR)] = '\0';
        TRACE("DeviceInstance %s\n", debugstr_w(InstancePath));

        if (DeviceInstanceW)
        {
            /* Check if device enumerator is not the right one */
            if (strcmpW(DeviceInstanceW, InstancePath) != 0)
                continue;
        }

        /* Find class GUID associated to the device instance */
        rc = RegOpenKeyExW(
            list->HKLM,
            REGSTR_PATH_SYSTEMENUM,
            0, /* Options */
            0,
            &hEnumKey);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        rc = RegOpenKeyExW(
            hEnumKey,
            InstancePath,
            0, /* Options */
            KEY_QUERY_VALUE,
            &hKey);
        RegCloseKey(hEnumKey);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        dwLength = sizeof(KeyBuffer) - sizeof(WCHAR);
        rc = RegQueryValueExW(hKey, ClassGUID, NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
        RegCloseKey(hKey);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        KeyBuffer[dwLength / sizeof(WCHAR)] = '\0';
        KeyBuffer[37] = '\0'; /* Replace the } by a NULL character */
        if (UuidFromStringW(&KeyBuffer[1], &ClassGuid) != RPC_S_OK)
        {
            rc = ERROR_GEN_FAILURE;
            goto cleanup;
        }
        TRACE("ClassGUID %s\n", debugstr_guid(&ClassGuid));

        /* If current device doesn't match the list GUID (if any), skip this entry */
        if (!IsEqualIID(&list->ClassGuid, &GUID_NULL) && !IsEqualIID(&list->ClassGuid, &ClassGuid))
            continue;

        /* Enumerate subkeys of hDeviceInstanceKey (ie "#ReferenceString" in IoRegisterDeviceInterface). Skip entries that don't start with '#' */
        j = 0;
        while (TRUE)
        {
            struct DeviceInterface *interfaceInfo;

            dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
            rc = RegEnumKeyExW(hDeviceInstanceKey, j, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
            if (rc == ERROR_NO_MORE_ITEMS)
                break;
            if (rc != ERROR_SUCCESS)
                goto cleanup;
            j++;
            if (KeyBuffer[0] != '#')
                /* This entry doesn't represent an interesting entry */
                continue;

            /* Open sub key */
            if (hReferenceKey != NULL)
                RegCloseKey(hReferenceKey);
            rc = RegOpenKeyExW(hDeviceInstanceKey, KeyBuffer, 0, KEY_QUERY_VALUE, &hReferenceKey);
            if (rc != ERROR_SUCCESS)
                goto cleanup;

            /* Read SymbolicLink value */
            rc = RegQueryValueExW(hReferenceKey, SymbolicLink, NULL, &dwRegType, NULL, &dwLength);
            if (rc != ERROR_SUCCESS )
                goto cleanup;
            if (dwRegType != REG_SZ)
            {
                rc = ERROR_GEN_FAILURE;
                goto cleanup;
            }

            /* We have found a device */
            /* Step 1. Create a device info element */
            if (!CreateDeviceInfoElement(list, InstancePath, &ClassGuid, &deviceInfo))
            {
                rc = GetLastError();
                goto cleanup;
            }
            TRACE("Adding device %s to list\n", debugstr_w(InstancePath));
            InsertTailList(&list->ListHead, &deviceInfo->ListEntry);

            /* Step 2. Create an interface list for this element */
            HeapFree(GetProcessHeap(), 0, pSymbolicLink);
            pSymbolicLink = HeapAlloc(GetProcessHeap(), 0, (dwLength + 1) * sizeof(WCHAR));
            if (!pSymbolicLink)
            {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            rc = RegQueryValueExW(hReferenceKey, SymbolicLink, NULL, NULL, (LPBYTE)pSymbolicLink, &dwLength);
            pSymbolicLink[dwLength / sizeof(WCHAR)] = '\0';
            if (rc != ERROR_SUCCESS)
                goto cleanup;
            if (!CreateDeviceInterface(deviceInfo, pSymbolicLink, InterfaceGuid, &interfaceInfo))
            {
                rc = GetLastError();
                goto cleanup;
            }

            /* Step 3. Update flags */
            if (KeyBuffer[1] == '\0')
                interfaceInfo->Flags |= SPINT_DEFAULT;
            rc = RegOpenKeyExW(hReferenceKey, Control, 0, KEY_QUERY_VALUE, &hControlKey);
            if (rc != ERROR_SUCCESS)
            {
                if (OnlyPresentInterfaces)
                {
                    DestroyDeviceInterface(interfaceInfo);
                    continue;
                }
                else
                    interfaceInfo->Flags |= SPINT_REMOVED;
            }
            else
            {
                dwLength = sizeof(DWORD);
                if (RegQueryValueExW(hControlKey, Linked, NULL, &dwRegType, (LPBYTE)&LinkedValue, &dwLength)
                    && dwRegType == REG_DWORD && LinkedValue)
                    interfaceInfo->Flags |= SPINT_ACTIVE;
                RegCloseKey(hControlKey);
            }

            TRACE("Adding interface %s to list\n", debugstr_w(pSymbolicLink));
            InsertTailList(&deviceInfo->InterfaceListHead, &interfaceInfo->ListEntry);
        }
    }
    rc = ERROR_SUCCESS;

cleanup:
    if (hReferenceKey != NULL)
        RegCloseKey(hReferenceKey);
    if (hDeviceInstanceKey != NULL)
        RegCloseKey(hDeviceInstanceKey);
    if (hInterfaceKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hInterfaceKey);
    HeapFree(GetProcessHeap(), 0, InstancePath);
    HeapFree(GetProcessHeap(), 0, pSymbolicLink);
    return rc;
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiEnumDeviceInterfaces(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN CONST GUID *InterfaceClassGuid,
    IN DWORD MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    BOOL ret = FALSE;

    TRACE("%p, %p, %s, %ld, %p\n", DeviceInfoSet, DeviceInfoData,
     debugstr_guid(InterfaceClassGuid), MemberIndex, DeviceInterfaceData);

    if (!DeviceInterfaceData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)DeviceInfoSet;

        if (list->magic == SETUP_DEV_INFO_SET_MAGIC)
        {
            PLIST_ENTRY ItemList = list->ListHead.Flink;
            BOOL Found = FALSE;
            while (ItemList != &list->ListHead && !Found)
            {
                PLIST_ENTRY InterfaceListEntry;
                struct DeviceInfoElement *DevInfo = CONTAINING_RECORD(ItemList, struct DeviceInfoElement, ListEntry);
                if (DeviceInfoData && (struct DeviceInfoElement *)DeviceInfoData->Reserved != DevInfo)
                {
                    /* We are not searching for this element */
                    ItemList = ItemList->Flink;
                    continue;
                }
                InterfaceListEntry = DevInfo->InterfaceListHead.Flink;
                while (InterfaceListEntry != &DevInfo->InterfaceListHead && !Found)
                {
                    struct DeviceInterface *DevItf = CONTAINING_RECORD(InterfaceListEntry, struct DeviceInterface, ListEntry);
                    if (!IsEqualIID(&DevItf->InterfaceClassGuid, InterfaceClassGuid))
                    {
                        InterfaceListEntry = InterfaceListEntry->Flink;
                        continue;
                    }
                    if (MemberIndex-- == 0)
                    {
                        /* return this item */
                        memcpy(&DeviceInterfaceData->InterfaceClassGuid,
                            &DevItf->InterfaceClassGuid,
                            sizeof(GUID));
                        DeviceInterfaceData->Flags = DevItf->Flags;
                        DeviceInterfaceData->Reserved = (ULONG_PTR)DevItf;
                        Found = TRUE;
                    }
                    InterfaceListEntry = InterfaceListEntry->Flink;
                }
                ItemList = ItemList->Flink;
            }
            if (!Found)
                SetLastError(ERROR_NO_MORE_ITEMS);
            else
                ret = TRUE;
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInterfaceDetailW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData OPTIONAL,
    IN DWORD DeviceInterfaceDetailDataSize,
    OUT PDWORD RequiredSize OPTIONAL,
    OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p %lu %p %p\n", DeviceInfoSet,
        DeviceInterfaceData, DeviceInterfaceDetailData,
        DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (!DeviceInfoSet || !DeviceInterfaceData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInterfaceDetailData == NULL && DeviceInterfaceDetailDataSize != 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInterfaceDetailData != NULL && DeviceInterfaceDetailDataSize < FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + sizeof(WCHAR))
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct DeviceInterface *deviceInterface = (struct DeviceInterface *)DeviceInterfaceData->Reserved;
        LPCWSTR devName = deviceInterface->SymbolicLink;
        DWORD sizeRequired = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) +
            (lstrlenW(devName) + 1) * sizeof(WCHAR);

        if (sizeRequired > DeviceInterfaceDetailDataSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            if (RequiredSize)
                *RequiredSize = sizeRequired;
        }
        else
        {
            strcpyW(DeviceInterfaceDetailData->DevicePath, devName);
            TRACE("DevicePath is %s\n", debugstr_w(DeviceInterfaceDetailData->DevicePath));
            if (DeviceInfoData)
            {
                memcpy(&DeviceInfoData->ClassGuid,
                    &deviceInterface->DeviceInfo->ClassGuid,
                    sizeof(GUID));
                DeviceInfoData->DevInst = deviceInterface->DeviceInfo->dnDevInst;
                DeviceInfoData->Reserved = (ULONG_PTR)deviceInterface->DeviceInfo;
            }
            ret = TRUE;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInterfaceDetailA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData OPTIONAL,
    IN DWORD DeviceInterfaceDetailDataSize,
    OUT PDWORD RequiredSize OPTIONAL,
    OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailDataW = NULL;
    DWORD sizeW = 0, sizeA;
    BOOL ret = FALSE;

    TRACE("%p %p %p %lu %p %p\n", DeviceInfoSet,
        DeviceInterfaceData, DeviceInterfaceDetailData,
        DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInterfaceDetailData == NULL && DeviceInterfaceDetailDataSize != 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInterfaceDetailData != NULL && DeviceInterfaceDetailDataSize < FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath) + 1)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        if (DeviceInterfaceDetailData != NULL)
        {
            sizeW = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)
                + (DeviceInterfaceDetailDataSize - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath)) * sizeof(WCHAR);
            DeviceInterfaceDetailDataW = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, sizeW);
            if (!DeviceInterfaceDetailDataW)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (!DeviceInterfaceDetailData || (DeviceInterfaceDetailData && DeviceInterfaceDetailDataW))
        {
            DeviceInterfaceDetailDataW->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            ret = SetupDiGetDeviceInterfaceDetailW(
                DeviceInfoSet,
                DeviceInterfaceData,
                DeviceInterfaceDetailDataW,
                sizeW,
                &sizeW,
                DeviceInfoData);
            sizeA = (sizeW - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)) / sizeof(WCHAR)
                + FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath);
            if (RequiredSize)
                *RequiredSize = sizeA;
            if (ret && DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize <= sizeA)
            {
                if (!WideCharToMultiByte(
                    CP_ACP, 0,
                    DeviceInterfaceDetailDataW->DevicePath, -1,
                    DeviceInterfaceDetailData->DevicePath, DeviceInterfaceDetailDataSize - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath),
                    NULL, NULL))
                {
                    ret = FALSE;
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailDataW);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiOpenDeviceInterfaceA(
    IN HDEVINFO DeviceInfoSet,
    IN PCSTR DevicePath,
    IN DWORD OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL)
{
    LPWSTR DevicePathW = NULL;
    BOOL bResult;

    TRACE("%p %s %08lx %p\n", DeviceInfoSet, debugstr_a(DevicePath), OpenFlags, DeviceInterfaceData);

    DevicePathW = MultiByteToUnicode(DevicePath, CP_ACP);
    if (DevicePathW == NULL)
        return FALSE;

    bResult = SetupDiOpenDeviceInterfaceW(DeviceInfoSet,
        DevicePathW, OpenFlags, DeviceInterfaceData);

    MyFree(DevicePathW);

    return bResult;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiOpenDeviceInterfaceW(
    IN HDEVINFO DeviceInfoSet,
    IN PCWSTR DevicePath,
    IN DWORD OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL)
{
    FIXME("%p %s %08lx %p\n",
        DeviceInfoSet, debugstr_w(DevicePath), OpenFlags, DeviceInterfaceData);
    return FALSE;
}

static BOOL
InstallOneInterface(
    IN LPGUID InterfaceGuid,
    IN LPCWSTR ReferenceString,
    IN LPCWSTR InterfaceSection,
    IN UINT InterfaceFlags)
{
    if (InterfaceFlags != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    FIXME("Need to InstallOneInterface(%s %s %s %u)\n", debugstr_guid(InterfaceGuid),
        debugstr_w(ReferenceString), debugstr_w(InterfaceSection), InterfaceFlags);
    return TRUE;
}

/***********************************************************************
 *		SetupDiInstallDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiInstallDeviceInterfaces(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *list = NULL;
    BOOL ret = FALSE;

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DriverInfoElement *SelectedDriver;
        SP_DEVINSTALL_PARAMS_W InstallParams;
        WCHAR SectionName[MAX_PATH];
        DWORD SectionNameLength = 0;
        INFCONTEXT ContextInterface;
        LPWSTR InterfaceGuidString = NULL;
        LPWSTR ReferenceString = NULL;
        LPWSTR InterfaceSection = NULL;
        INT InterfaceFlags;
        GUID InterfaceGuid;
        BOOL Result;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!Result)
            goto cleanup;

        SelectedDriver = (struct DriverInfoElement *)InstallParams.Reserved;
        if (SelectedDriver == NULL)
        {
            SetLastError(ERROR_NO_DRIVER_SELECTED);
            ret = FALSE;
            goto cleanup;
        }

        /* Get .Interfaces section name */
        Result = SetupDiGetActualSectionToInstallW(
            SelectedDriver->InfFileDetails->hInf,
            SelectedDriver->Details.SectionName,
            SectionName, MAX_PATH, &SectionNameLength, NULL);
        if (!Result || SectionNameLength > MAX_PATH - strlenW(DotInterfaces) - 1)
            goto cleanup;
        strcatW(SectionName, DotInterfaces);

        ret = TRUE;
        Result = SetupFindFirstLineW(
            SelectedDriver->InfFileDetails->hInf,
            SectionName,
            AddInterface,
            &ContextInterface);
        while (ret && Result)
        {
            ret = GetStringField(&ContextInterface, 1, &InterfaceGuidString);
            if (!ret)
                goto cleanup;
            else if (strlenW(InterfaceGuidString) != MAX_GUID_STRING_LEN - 1)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ret = FALSE;
                goto cleanup;
            }

            InterfaceGuidString[MAX_GUID_STRING_LEN - 2] = '\0'; /* Replace the } by a NULL character */
            if (UuidFromStringW(&InterfaceGuidString[1], &InterfaceGuid) != RPC_S_OK)
            {
                /* Bad GUID, skip the entry */
                SetLastError(ERROR_INVALID_PARAMETER);
                ret = FALSE;
                goto cleanup;
            }

            ret = GetStringField(&ContextInterface, 2, &ReferenceString);
            if (!ret)
                goto cleanup;

            ret = GetStringField(&ContextInterface, 3, &InterfaceSection);
            if (!ret)
                goto cleanup;

            ret = SetupGetIntField(
                &ContextInterface,
                4, /* Field index */
                &InterfaceFlags);
            if (!ret)
            {
                if (GetLastError() == ERROR_INVALID_PARAMETER)
                {
                    /* The field may be empty. Ignore the error */
                    InterfaceFlags = 0;
                    ret = TRUE;
                }
                else
                    goto cleanup;
            }

            /* Install Interface */
            ret = InstallOneInterface(&InterfaceGuid, ReferenceString, InterfaceSection, InterfaceFlags);

cleanup:
            MyFree(InterfaceGuidString);
            MyFree(ReferenceString);
            MyFree(InterfaceSection);
            InterfaceGuidString = ReferenceString = InterfaceSection = NULL;
            Result = SetupFindNextMatchLineW(&ContextInterface, AddInterface, &ContextInterface);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}
