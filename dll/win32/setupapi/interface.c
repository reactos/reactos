/*
 * SetupAPI interface-related functions
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
 *           2005-2006 Herv� Poussineau (hpoussin@reactos.org)
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
    IN struct DeviceInfo* deviceInfo,
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
    deviceInterface->InterfaceClassGuid = *pInterfaceGuid;

    *pDeviceInterface = deviceInterface;
    return TRUE;
}

BOOL
DestroyDeviceInterface(
    struct DeviceInterface* deviceInterface)
{
    return HeapFree(GetProcessHeap(), 0, deviceInterface);
}

// SETUPDI_EnumerateInterfaces
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
    struct DeviceInfo *deviceInfo;

    hInterfaceKey = INVALID_HANDLE_VALUE;
    hDeviceInstanceKey = NULL;
    hReferenceKey = NULL;

    /* Open registry key related to this interface */
    hInterfaceKey = SetupDiOpenClassRegKeyExW(InterfaceGuid, KEY_ENUMERATE_SUB_KEYS, DIOCR_INTERFACE, MachineName, NULL);
    if (hInterfaceKey == INVALID_HANDLE_VALUE)
    {
        /* Key doesn't exist. Let's keep it empty */
        rc = ERROR_SUCCESS;
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
            READ_CONTROL,
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
            if (!CreateDeviceInfo(list, InstancePath, &ClassGuid, &deviceInfo))
            {
                rc = GetLastError();
                goto cleanup;
            }
            TRACE("Adding device %s to list\n", debugstr_w(InstancePath));
            InsertTailList(&list->ListHead, &deviceInfo->ListEntry);

            /* Step 2. Create an interface list for this element */
            HeapFree(GetProcessHeap(), 0, pSymbolicLink);
            pSymbolicLink = HeapAlloc(GetProcessHeap(), 0, dwLength + sizeof(WCHAR));
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
#if 0
                if (OnlyPresentInterfaces)
                {
                    DestroyDeviceInterface(interfaceInfo);
                    continue;
                }
                else
                    interfaceInfo->Flags |= SPINT_REMOVED;
#endif
            }
            else
            {
                dwLength = sizeof(DWORD);
                if (RegQueryValueExW(hControlKey, Linked, NULL, &dwRegType, (LPBYTE)&LinkedValue, &dwLength) == ERROR_SUCCESS
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

static LPWSTR
CreateSymbolicLink(
    IN LPGUID InterfaceGuid,
    IN LPCWSTR ReferenceString,
    IN struct DeviceInfo *devInfo)
{
    DWORD Length, Index, Offset;
    LPWSTR Key;

    Length = wcslen(devInfo->instanceId) + 4 /* prepend ##?# */ + 41 /* #{GUID} + */ + 1 /* zero byte */;

    Key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length * sizeof(WCHAR));
    if (!Key)
        return NULL;

    wcscpy(Key, L"##?#");
    wcscat(Key, devInfo->instanceId);

    for(Index = 4; Index < Length; Index++)
    {
        if (Key[Index] == L'\\')
        {
            Key[Index] = L'#';
        }
    }

    wcscat(Key, L"#");

    Offset = wcslen(Key);
    pSetupStringFromGuid(InterfaceGuid, Key + Offset, Length - Offset);

    return Key;
}


static BOOL
InstallOneInterface(
    IN LPGUID InterfaceGuid,
    IN LPCWSTR ReferenceString,
    IN LPCWSTR InterfaceSection,
    IN UINT InterfaceFlags,
    IN HINF hInf,
    IN HDEVINFO DeviceInfoSet,
    IN struct DeviceInfo *devInfo)
{
    HKEY hKey, hRefKey;
    LPWSTR Path;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    struct DeviceInterface *DevItf = NULL;

    if (InterfaceFlags != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("Need to InstallOneInterface(%s %s %s %u) hInf %p DeviceInfoSet %p devInfo %p instanceId %s\n", debugstr_guid(InterfaceGuid),
        debugstr_w(ReferenceString), debugstr_w(InterfaceSection), InterfaceFlags, hInf, DeviceInfoSet, devInfo, debugstr_w(devInfo->instanceId));


    Path = CreateSymbolicLink(InterfaceGuid, ReferenceString, devInfo);
    if (!Path)
        return FALSE;

    CreateDeviceInterface(devInfo, Path, InterfaceGuid, &DevItf);
    HeapFree(GetProcessHeap(), 0, Path);
    if (!DevItf)
    {
        return FALSE;
    }

    DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
    // copy_device_iface_data(&DeviceInterfaceData, DevItf);
    DeviceInterfaceData.InterfaceClassGuid = DevItf->InterfaceClassGuid;
    DeviceInterfaceData.Flags = DevItf->Flags;
    DeviceInterfaceData.Reserved = (ULONG_PTR)DevItf;

    hKey = SetupDiCreateDeviceInterfaceRegKeyW(DeviceInfoSet, &DeviceInterfaceData, 0, KEY_ALL_ACCESS, NULL, 0);
    HeapFree(GetProcessHeap(), 0, DevItf);
    if (hKey == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (ReferenceString)
    {
        Path = HeapAlloc(GetProcessHeap(), 0, (wcslen(ReferenceString) + 2) * sizeof(WCHAR));
        if (!Path)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        wcscpy(Path, L"#");
        wcscat(Path, ReferenceString);

        if (RegCreateKeyExW(hKey, Path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hRefKey, NULL) != ERROR_SUCCESS)
        {
            ERR("failed to create key %s %lx\n", debugstr_w(Path), GetLastError());
            HeapFree(GetProcessHeap(), 0, Path);
            return FALSE;
        }

        RegCloseKey(hKey);
        hKey = hRefKey;
        HeapFree(GetProcessHeap(), 0, Path);
    }

    if (RegCreateKeyExW(hKey, L"Device Parameters", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hRefKey, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    return SetupInstallFromInfSectionW(NULL, /* FIXME */ hInf, InterfaceSection, SPINST_REGISTRY, hRefKey, NULL, 0, NULL, NULL, NULL, NULL);
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
    else if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData && DeviceInfoData->Reserved == 0)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DeviceInfo *devInfo;
        struct DriverInfoElement *SelectedDriver = NULL;
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

        devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!Result)
            goto cleanup;

        SelectedDriver = (struct DriverInfoElement *)InstallParams.ClassInstallReserved;
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
            {
                /* ReferenceString is optional */
                InterfaceSection = ReferenceString;
                ReferenceString = NULL;
            }

            ret = SetupGetIntField(
                &ContextInterface,
                (ReferenceString ? 4 : 3), /* Field index */
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
            ret = InstallOneInterface(&InterfaceGuid, ReferenceString, InterfaceSection, InterfaceFlags, SelectedDriver->InfFileDetails->hInf, DeviceInfoSet, devInfo);

cleanup:
            MyFree(InterfaceGuidString);
            if (ReferenceString)
                MyFree(ReferenceString);
            MyFree(InterfaceSection);
            InterfaceGuidString = ReferenceString = InterfaceSection = NULL;
            Result = SetupFindNextMatchLineW(&ContextInterface, AddInterface, &ContextInterface);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

HKEY WINAPI
SetupDiOpenDeviceInterfaceRegKey(
    IN HDEVINFO  DeviceInfoSet, IN PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData, IN DWORD  Reserved, IN REGSAM  samDesired)
{
    HKEY hKey = INVALID_HANDLE_VALUE, hDevKey;
    struct DeviceInfoSet * list;

    TRACE("%p %p %p 0x%08x 0x%08x)\n", DeviceInfoSet, DeviceInterfaceData, Reserved, samDesired);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInterfaceData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInterfaceData && DeviceInterfaceData->Reserved == 0)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInterfaceData && DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DeviceInterface *DevItf;
        LPWSTR Path, Guid, Slash;
        DWORD Length;
        DevItf = (struct DeviceInterface *)DeviceInterfaceData->Reserved;

        Length = wcslen(DevItf->SymbolicLink);

        Path = HeapAlloc(GetProcessHeap(), 0, (Length+2) * sizeof(WCHAR));
        if (!Path)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        wcscpy(Path, DevItf->SymbolicLink);

        Guid = wcsrchr(Path, '}');
        Slash = wcsrchr(Path, '\\');
        if (!Guid || !Slash)
        {
            HeapFree(GetProcessHeap(), 0, Path);
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        if ((ULONG_PTR)Slash > (ULONG_PTR)Guid)
        {
            /* Create an extra slash */
            memmove(Slash+1, Slash, (wcslen(Slash) + 1) * sizeof(WCHAR));
            Slash[1] = L'#';
        }

        Guid = Path;
        while((ULONG_PTR)Guid < (ULONG_PTR)Slash)
        {
            if (*Guid == L'\\')
                *Guid = L'#';

            Guid++;
        }

        hKey = SetupDiOpenClassRegKeyExW(&DeviceInterfaceData->InterfaceClassGuid, samDesired, DIOCR_INTERFACE, NULL, NULL);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            if (RegOpenKeyExW(hKey, Path, 0, samDesired, &hDevKey) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                hKey = hDevKey;
            }
            else
            {
                RegCloseKey(hKey);
                hKey = INVALID_HANDLE_VALUE;
            }
        }

        HeapFree(GetProcessHeap(), 0, Path);
    }

    return hKey;
}

/***********************************************************************
 *		SetupDiDeleteDeviceInterfaceData (SETUPAPI.@)
 */
BOOL
WINAPI
SetupDiDeleteDeviceInterfaceData(
    HDEVINFO DeviceInfoSet,
    PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("SetupDiDeleteDeviceInterfaceData(%p %p) stub\n",
          DeviceInfoSet, DeviceInterfaceData);
    return TRUE;
}
