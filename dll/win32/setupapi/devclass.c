/*
 * SetupAPI device class-related functions
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

#include <wingdi.h>
#include <shellapi.h>

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR ClassInstall32[]  = {'C','l','a','s','s','I','n','s','t','a','l','l','3','2',0};
static const WCHAR DotServices[]  = {'.','S','e','r','v','i','c','e','s',0};
static const WCHAR InterfaceInstall32[]  = {'I','n','t','e','r','f','a','c','e','I','n','s','t','a','l','l','3','2',0};
static const WCHAR SetupapiDll[]  = {'s','e','t','u','p','a','p','i','.','d','l','l',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};

typedef BOOL
(WINAPI* PROPERTY_PAGE_PROVIDER) (
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE fAddFunc,
    IN LPARAM lParam);
typedef BOOL
(*UPDATE_CLASS_PARAM_HANDLER) (
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize);

static BOOL
SETUP_PropertyChangeHandler(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize);

static BOOL
SETUP_PropertyAddPropertyAdvancedHandler(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize);

typedef struct _INSTALL_PARAMS_DATA
{
    DI_FUNCTION Function;
    UPDATE_CLASS_PARAM_HANDLER UpdateHandler;
    ULONG ParamsSize;
    LONG FieldOffset;
} INSTALL_PARAMS_DATA;

#define ADD_PARAM_HANDLER(Function, UpdateHandler, ParamsType, ParamsField) \
    { Function, UpdateHandler, sizeof(ParamsType), FIELD_OFFSET(struct ClassInstallParams, ParamsField) },

static const INSTALL_PARAMS_DATA InstallParamsData[] = {
    ADD_PARAM_HANDLER(DIF_PROPERTYCHANGE, SETUP_PropertyChangeHandler, SP_PROPCHANGE_PARAMS, PropChangeParams)
    ADD_PARAM_HANDLER(DIF_ADDPROPERTYPAGE_ADVANCED, SETUP_PropertyAddPropertyAdvancedHandler, SP_ADDPROPERTYPAGE_DATA, AddPropertyPageData)
};
#undef ADD_PARAM_HANDLER

#define UNKNOWN_ICON_INDEX 18

/***********************************************************************
 *		SetupDiDestroyClassImageList(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiDestroyClassImageList(
    IN PSP_CLASSIMAGELIST_DATA ClassImageListData)
{
    struct ClassImageList *list;
    BOOL ret = FALSE;

    TRACE("%p\n", ClassImageListData);

    if (!ClassImageListData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if ((list = (struct ClassImageList *)ClassImageListData->Reserved) == NULL)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (list->magic != SETUP_CLASS_IMAGE_LIST_MAGIC)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        //DestroyIcon()
        //ImageList_Destroy();
        FIXME("Stub %p\n", ClassImageListData);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SETUP_CreateDevicesListFromEnumerator
 *
 * PARAMS
 *   list [IO] Device info set to fill with discovered devices.
 *   pClassGuid [I] If specified, only devices which belong to this class will be added.
 *   Enumerator [I] Location to search devices to add.
 *   hEnumeratorKey [I] Registry key corresponding to Enumerator key. Must have KEY_ENUMERATE_SUB_KEYS right.
 *
 * RETURNS
 *   Success: ERROR_SUCCESS.
 *   Failure: an error code.
 */
static LONG
SETUP_CreateDevicesListFromEnumerator(
    IN OUT struct DeviceInfoSet *list,
    IN CONST GUID *pClassGuid OPTIONAL,
    IN LPCWSTR Enumerator,
    IN HKEY hEnumeratorKey) /* handle to Enumerator registry key */
{
    HKEY hDeviceIdKey = NULL, hInstanceIdKey;
    WCHAR KeyBuffer[MAX_PATH];
    WCHAR InstancePath[MAX_PATH];
    LPWSTR pEndOfInstancePath; /* Pointer into InstancePath buffer */
    struct DeviceInfo *deviceInfo;
    DWORD i = 0, j;
    DWORD dwLength, dwRegType;
    DWORD rc;

    /* Enumerate device IDs (subkeys of hEnumeratorKey) */
    while (TRUE)
    {
        dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
        rc = RegEnumKeyExW(hEnumeratorKey, i, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
        if (rc == ERROR_NO_MORE_ITEMS)
            break;
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        i++;

        /* Open device id sub key */
        if (hDeviceIdKey != NULL)
            RegCloseKey(hDeviceIdKey);
        rc = RegOpenKeyExW(hEnumeratorKey, KeyBuffer, 0, KEY_ENUMERATE_SUB_KEYS, &hDeviceIdKey);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
        strcpyW(InstancePath, Enumerator);
        strcatW(InstancePath, BackSlash);
        strcatW(InstancePath, KeyBuffer);
        strcatW(InstancePath, BackSlash);
        pEndOfInstancePath = &InstancePath[strlenW(InstancePath)];

        /* Enumerate instance IDs (subkeys of hDeviceIdKey) */
        j = 0;
        while (TRUE)
        {
            GUID KeyGuid;

            dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
            rc = RegEnumKeyExW(hDeviceIdKey, j, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
            if (rc == ERROR_NO_MORE_ITEMS)
                break;
            if (rc != ERROR_SUCCESS)
                goto cleanup;
            j++;

            /* Open instance id sub key */
            rc = RegOpenKeyExW(hDeviceIdKey, KeyBuffer, 0, KEY_QUERY_VALUE, &hInstanceIdKey);
            if (rc != ERROR_SUCCESS)
                goto cleanup;
            *pEndOfInstancePath = '\0';
            strcatW(InstancePath, KeyBuffer);

            /* Read ClassGUID value */
            dwLength = sizeof(KeyBuffer) - sizeof(WCHAR);
            rc = RegQueryValueExW(hInstanceIdKey, ClassGUID, NULL, &dwRegType, (LPBYTE)KeyBuffer, &dwLength);
            RegCloseKey(hInstanceIdKey);
            if (rc == ERROR_FILE_NOT_FOUND)
            {
                if (pClassGuid)
                    /* Skip this bad entry as we can't verify it */
                    continue;
                /* Set a default GUID for this device */
                memcpy(&KeyGuid, &GUID_NULL, sizeof(GUID));
            }
            else if (rc != ERROR_SUCCESS)
            {
                goto cleanup;
            }
            else if (dwRegType != REG_SZ || dwLength < MAX_GUID_STRING_LEN * sizeof(WCHAR))
            {
                rc = ERROR_GEN_FAILURE;
                goto cleanup;
            }
            else
            {
                KeyBuffer[MAX_GUID_STRING_LEN - 2] = '\0'; /* Replace the } by a NULL character */
                if (UuidFromStringW(&KeyBuffer[1], &KeyGuid) != RPC_S_OK)
                    /* Bad GUID, skip the entry */
                    continue;
            }

            if (pClassGuid && !IsEqualIID(&KeyGuid, pClassGuid))
            {
                /* Skip this entry as it is not the right device class */
                continue;
            }

            /* Add the entry to the list */
            if (!CreateDeviceInfo(list, InstancePath, &KeyGuid, &deviceInfo))
            {
                rc = GetLastError();
                goto cleanup;
            }
            TRACE("Adding '%s' to device info set %p\n", debugstr_w(InstancePath), list);
            InsertTailList(&list->ListHead, &deviceInfo->ListEntry);
        }
    }

    rc = ERROR_SUCCESS;

cleanup:
    if (hDeviceIdKey != NULL)
        RegCloseKey(hDeviceIdKey);
    return rc;
}

LONG
SETUP_CreateDevicesList(
    IN OUT struct DeviceInfoSet *list,
    IN PCWSTR MachineName OPTIONAL,
    IN CONST GUID *Class OPTIONAL,
    IN PCWSTR Enumerator OPTIONAL)
{
    HKEY HKLM = HKEY_LOCAL_MACHINE;
    HKEY hEnumKey = NULL;
    HKEY hEnumeratorKey = NULL;
    WCHAR KeyBuffer[MAX_PATH];
    DWORD i;
    DWORD dwLength;
    DWORD rc;

    if (Class && IsEqualIID(Class, &GUID_NULL))
        Class = NULL;

    /* Open Enum key (if applicable) */
    if (MachineName != NULL)
    {
        rc = RegConnectRegistryW(MachineName, HKEY_LOCAL_MACHINE, &HKLM);
        if (rc != ERROR_SUCCESS)
            goto cleanup;
    }

    rc = RegOpenKeyExW(
        HKLM,
        REGSTR_PATH_SYSTEMENUM,
        0,
        KEY_ENUMERATE_SUB_KEYS,
        &hEnumKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    /* If enumerator is provided, call directly SETUP_CreateDevicesListFromEnumerator.
     * Else, enumerate all enumerators and call SETUP_CreateDevicesListFromEnumerator
     * for each one.
     */
    if (Enumerator)
    {
        rc = RegOpenKeyExW(
            hEnumKey,
            Enumerator,
            0,
            KEY_ENUMERATE_SUB_KEYS,
            &hEnumeratorKey);
        if (rc != ERROR_SUCCESS)
        {
            if (rc == ERROR_FILE_NOT_FOUND)
                rc = ERROR_INVALID_DATA;
            goto cleanup;
        }
        rc = SETUP_CreateDevicesListFromEnumerator(list, Class, Enumerator, hEnumeratorKey);
    }
    else
    {
        /* Enumerate enumerators */
        i = 0;
        while (TRUE)
        {
            dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
            rc = RegEnumKeyExW(hEnumKey, i, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
            if (rc == ERROR_NO_MORE_ITEMS)
                break;
            else if (rc != ERROR_SUCCESS)
                goto cleanup;
            i++;

            /* Open sub key */
            if (hEnumeratorKey != NULL)
                RegCloseKey(hEnumeratorKey);
            rc = RegOpenKeyExW(hEnumKey, KeyBuffer, 0, KEY_ENUMERATE_SUB_KEYS, &hEnumeratorKey);
            if (rc != ERROR_SUCCESS)
                goto cleanup;

            /* Call SETUP_CreateDevicesListFromEnumerator */
            rc = SETUP_CreateDevicesListFromEnumerator(list, Class, KeyBuffer, hEnumeratorKey);
            if (rc != ERROR_SUCCESS)
                goto cleanup;
        }
        rc = ERROR_SUCCESS;
    }

cleanup:
    if (HKLM != HKEY_LOCAL_MACHINE)
        RegCloseKey(HKLM);
    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);
    if (hEnumeratorKey != NULL)
        RegCloseKey(hEnumeratorKey);
    return rc;
}

static BOOL
SETUP_GetIconIndex(
    IN HKEY hClassKey,
    OUT PINT ImageIndex)
{
    LPWSTR Buffer = NULL;
    DWORD dwRegType, dwLength;
    LONG rc;
    BOOL ret = FALSE;

    /* Read icon registry key */
    rc = RegQueryValueExW(hClassKey, REGSTR_VAL_INSICON, NULL, &dwRegType, NULL, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    } else if (dwRegType != REG_SZ)
    {
        SetLastError(ERROR_INVALID_INDEX);
        goto cleanup;
    }
    Buffer = MyMalloc(dwLength + sizeof(WCHAR));
    if (!Buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    rc = RegQueryValueExW(hClassKey, REGSTR_VAL_INSICON, NULL, NULL, (LPBYTE)Buffer, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    /* make sure the returned buffer is NULL-terminated */
    Buffer[dwLength / sizeof(WCHAR)] = 0;

    /* Transform icon value to a INT */
    *ImageIndex = atoiW(Buffer);
    ret = TRUE;

cleanup:
    MyFree(Buffer);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassImageIndex (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassImageIndex(
    IN PSP_CLASSIMAGELIST_DATA ClassImageListData,
    IN CONST GUID *ClassGuid,
    OUT PINT ImageIndex)
{
    struct ClassImageList *list;
    BOOL ret = FALSE;

    TRACE("%p %s %p\n", ClassImageListData, debugstr_guid(ClassGuid), ImageIndex);

    if (!ClassImageListData || !ClassGuid || !ImageIndex)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if ((list = (struct ClassImageList *)ClassImageListData->Reserved) == NULL)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (list->magic != SETUP_CLASS_IMAGE_LIST_MAGIC)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!ImageIndex)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        DWORD i;

        for (i = 0; i < list->NumberOfGuids; i++)
        {
            if (IsEqualIID(ClassGuid, &list->Guids[i]))
                break;
        }

        if (i == list->NumberOfGuids || list->IconIndexes[i] < 0)
            SetLastError(ERROR_FILE_NOT_FOUND);
        else
        {
            *ImageIndex = list->IconIndexes[i];
            ret = TRUE;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassImageList(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassImageList(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData)
{
    return SetupDiGetClassImageListExW(ClassImageListData, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassImageListExA(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassImageListExA(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData,
    IN PCSTR MachineName OPTIONAL,
    IN PVOID Reserved)
{
    PWSTR MachineNameW = NULL;
    BOOL ret;

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return FALSE;
    }

    ret = SetupDiGetClassImageListExW(ClassImageListData, MachineNameW, Reserved);

    MyFree(MachineNameW);

    return ret;
}

static BOOL WINAPI 
SETUP_GetClassIconInfo(IN CONST GUID *ClassGuid, OUT PINT OutIndex, OUT LPWSTR *OutDllName)
{
    LPWSTR Buffer = NULL;
    INT iconIndex = -UNKNOWN_ICON_INDEX;
    HKEY hKey = INVALID_HANDLE_VALUE;
    BOOL ret = FALSE;

    if (ClassGuid)
    {
        hKey = SetupDiOpenClassRegKey(ClassGuid, KEY_QUERY_VALUE);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            SETUP_GetIconIndex(hKey, &iconIndex);
        }
    }

    if (iconIndex > 0)
    {
        /* Look up icon in dll specified by Installer32 or EnumPropPages32 key */
        PWCHAR Comma;
        LONG rc;
        DWORD dwRegType, dwLength;
        rc = RegQueryValueExW(hKey, REGSTR_VAL_INSTALLER_32, NULL, &dwRegType, NULL, &dwLength);
        if (rc == ERROR_SUCCESS && dwRegType == REG_SZ)
        {
            Buffer = MyMalloc(dwLength + sizeof(WCHAR));
            if (Buffer == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
            rc = RegQueryValueExW(hKey, REGSTR_VAL_INSTALLER_32, NULL, NULL, (LPBYTE)Buffer, &dwLength);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            /* make sure the returned buffer is NULL-terminated */
            Buffer[dwLength / sizeof(WCHAR)] = 0;
        }
        else if
            (ERROR_SUCCESS == (rc = RegQueryValueExW(hKey, REGSTR_VAL_ENUMPROPPAGES_32, NULL, &dwRegType, NULL, &dwLength))
            && dwRegType == REG_SZ)
        {
            Buffer = MyMalloc(dwLength + sizeof(WCHAR));
            if (Buffer == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
            rc = RegQueryValueExW(hKey, REGSTR_VAL_ENUMPROPPAGES_32, NULL, NULL, (LPBYTE)Buffer, &dwLength);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            /* make sure the returned buffer is NULL-terminated */
            Buffer[dwLength / sizeof(WCHAR)] = 0;
        }
        else
        {
            /* Unable to find where to load the icon */
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto cleanup;
        }
        Comma = strchrW(Buffer, ',');
        if (!Comma)
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }
        *Comma = '\0';
        *OutDllName = Buffer;
    }
    else
    {
        /* Look up icon in setupapi.dll */
        iconIndex = -iconIndex;
        *OutDllName = NULL;
    }

    *OutIndex = iconIndex;
    ret = TRUE;

    TRACE("Icon index %d, dll name %s\n", iconIndex, debugstr_w(*OutDllName ? *OutDllName : SetupapiDll));

cleanup:

    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);

    if (Buffer && !ret)
        MyFree(Buffer);

    return ret;
}


/***********************************************************************
 *		SetupDiGetClassImageListExW(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassImageListExW(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData,
    IN PCWSTR MachineName OPTIONAL,
    IN PVOID Reserved)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p\n", ClassImageListData, debugstr_w(MachineName), Reserved);

    if (!ClassImageListData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (Reserved)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct ClassImageList *list = NULL;
        HDC hDC;
        DWORD RequiredSize;
        DWORD ilMask, bkColor;
        HICON hIcon;
        DWORD size;
        INT i, bpp;
        UINT idx;

        /* Get list of all class GUIDs in given computer */
        ret = SetupDiBuildClassInfoListExW(
            0,
            NULL,
            0,
            &RequiredSize,
            MachineName,
            NULL);
        if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto cleanup;

        size = sizeof(struct ClassImageList)
            + (sizeof(GUID) + sizeof(INT)) * RequiredSize;
        list = HeapAlloc(GetProcessHeap(), 0, size);
        if (!list)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        list->magic = SETUP_CLASS_IMAGE_LIST_MAGIC;
        list->NumberOfGuids = RequiredSize;
        list->Guids = (GUID*)(list + 1);
        list->IconIndexes = (INT*)((ULONG_PTR)(list + 1) + sizeof(GUID) * RequiredSize);

        ret = SetupDiBuildClassInfoListExW(
            0,
            list->Guids,
            list->NumberOfGuids,
            &RequiredSize,
            MachineName,
            NULL);
        if (!ret)
            goto cleanup;
        else if (RequiredSize != list->NumberOfGuids)
        {
            /* Hm. Class list changed since last call. Ignore
             * this case as it should be very rare */
            SetLastError(ERROR_GEN_FAILURE);
            ret = FALSE;
            goto cleanup;
        }

        /* Prepare a HIMAGELIST */
        InitCommonControls();

        hDC = GetDC(NULL);
        if (!hDC)
            goto cleanup;

        bpp = GetDeviceCaps(hDC, BITSPIXEL);
        ReleaseDC(NULL, hDC);

        if (bpp <= 4)
            ilMask = ILC_COLOR4;
        else if (bpp <= 8)
            ilMask = ILC_COLOR8;
        else if (bpp <= 16)
            ilMask = ILC_COLOR16;
        else if (bpp <= 24)
            ilMask = ILC_COLOR24;
        else if (bpp <= 32)
            ilMask = ILC_COLOR32;
        else
            ilMask = ILC_COLOR;

        ilMask |= ILC_MASK;

        ClassImageListData->ImageList = ImageList_Create(16, 16, ilMask, 100, 10);
        if (!ClassImageListData->ImageList)
            goto cleanup;

        ClassImageListData->Reserved = (ULONG_PTR)list;

        /* For some reason, Windows sets the list background to COLOR_WINDOW */
        bkColor = GetSysColor(COLOR_WINDOW);
        ImageList_SetBkColor(ClassImageListData->ImageList, bkColor);

        /* Now, we "simply" need to load icons associated with all class guids,
         * and put their index in the image list in the IconIndexes array */
        for (i = 0; i < list->NumberOfGuids; i++)
        {
            INT miniIconIndex;
            LPWSTR DllName = NULL;

            if (SETUP_GetClassIconInfo(&list->Guids[i], &miniIconIndex, &DllName))
            {
                if (DllName && ExtractIconExW(DllName, -miniIconIndex, NULL, &hIcon, 1) == 1)
                {
                    list->IconIndexes[i] = ImageList_AddIcon(ClassImageListData->ImageList, hIcon);
                }
                else if(!DllName)
                {
                    hIcon = LoadImage(hInstance, MAKEINTRESOURCE(miniIconIndex), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
                    list->IconIndexes[i] = ImageList_AddIcon(ClassImageListData->ImageList, hIcon);
                }

                if(hIcon)
                    DestroyIcon(hIcon);
                else
                    list->IconIndexes[i] = -1;

                if(DllName)
                    MyFree(DllName);
            }
            else
            {
                list->IconIndexes[i] = -1; /* Special value to indicate that the icon is unavailable */
            }
        }

        /* Finally, add the overlay icons to the image list */
        for (i = 0; i <= 2; i++)
        {
            hIcon = LoadImage(hInstance, MAKEINTRESOURCE(500 + i), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            if (hIcon)
            {
                idx = ImageList_AddIcon(ClassImageListData->ImageList, hIcon);
                if (idx != -1)
                    ImageList_SetOverlayImage(ClassImageListData->ImageList, idx, i + 1);
                DestroyIcon(hIcon);
            }
        }

        ret = TRUE;

cleanup:
        if (!ret)
        {
            if (ClassImageListData->Reserved)
                SetupDiDestroyClassImageList(ClassImageListData);
            else if (list)
                MyFree(list);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassInstallParamsA(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassInstallParamsA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    FIXME("SetupDiGetClassInstallParamsA(%p %p %p %lu %p) Stub\n",
        DeviceInfoSet, DeviceInfoData, ClassInstallParams, ClassInstallParamsSize, RequiredSize);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetClassInstallParamsW(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassInstallParamsW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    FIXME("SetupDiGetClassInstallParamsW(%p %p %p %lu %p) Stub\n",
        DeviceInfoSet, DeviceInfoData, ClassInstallParams, ClassInstallParamsSize, RequiredSize);
    return FALSE;
}

/***********************************************************************
 *		SetupDiLoadClassIcon(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiLoadClassIcon(
    IN CONST GUID *ClassGuid,
    OUT HICON *LargeIcon OPTIONAL,
    OUT PINT MiniIconIndex OPTIONAL)
{
    INT iconIndex = 0;
    LPWSTR DllName = NULL;
    BOOL ret = FALSE;
    HICON hIcon = NULL;

    if (LargeIcon)
    {
        if(!SETUP_GetClassIconInfo(ClassGuid, &iconIndex, &DllName))
            return FALSE;

        if (DllName && ExtractIconExW(DllName, -iconIndex, &hIcon, NULL, 1) == 1 && hIcon != NULL)
        {
            ret = TRUE;
        }
        else
        {
            /* load the default unknown device icon if ExtractIcon failed */
            if(DllName)
                iconIndex = UNKNOWN_ICON_INDEX;

            hIcon = LoadImage(hInstance, MAKEINTRESOURCE(iconIndex), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

            if(!LargeIcon)
                goto cleanup;
        }
    }

    if (MiniIconIndex)
        *MiniIconIndex = iconIndex;

    ret = TRUE;
    *LargeIcon = hIcon;

cleanup:

    if(DllName)
        MyFree(DllName);

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiInstallClassExW (SETUPAPI.@)
 */
HKEY
SETUP_CreateClassKey(HINF hInf);
BOOL WINAPI
SetupDiInstallClassExW(
    IN HWND hwndParent OPTIONAL,
    IN PCWSTR InfFileName OPTIONAL,
    IN DWORD Flags,
    IN HSPFILEQ FileQueue OPTIONAL,
    IN CONST GUID *InterfaceClassGuid OPTIONAL,
    IN PVOID Reserved1,
    IN PVOID Reserved2)
{
    BOOL ret = FALSE;

    TRACE("%p %s 0x%lx %p %s %p %p\n", hwndParent, debugstr_w(InfFileName), Flags,
        FileQueue, debugstr_guid(InterfaceClassGuid), Reserved1, Reserved2);

    if (!InfFileName)
    {
        FIXME("Case not implemented: InfFileName NULL\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else if (Flags & ~(DI_NOVCP | DI_NOBROWSE | DI_FORCECOPY | DI_QUIETINSTALL))
    {
        TRACE("Unknown flags: 0x%08lx\n", Flags & ~(DI_NOVCP | DI_NOBROWSE | DI_FORCECOPY | DI_QUIETINSTALL));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if ((Flags & DI_NOVCP) && FileQueue == NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (Reserved1 != NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (Reserved2 != NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
        SP_DEVINSTALL_PARAMS_W InstallParams;
        WCHAR SectionName[MAX_PATH];
        HINF hInf = INVALID_HANDLE_VALUE;
        HKEY hRootKey = INVALID_HANDLE_VALUE;
        PVOID callback_context = NULL;

        hDeviceInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
        if (hDeviceInfo == INVALID_HANDLE_VALUE)
            goto cleanup;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParamsW(hDeviceInfo, NULL, &InstallParams))
            goto cleanup;

        InstallParams.Flags &= ~(DI_NOVCP | DI_NOBROWSE | DI_QUIETINSTALL);
        InstallParams.Flags |= Flags & (DI_NOVCP | DI_NOBROWSE | DI_QUIETINSTALL);
        if (Flags & DI_NOVCP)
            InstallParams.FileQueue = FileQueue;
        if (!SetupDiSetDeviceInstallParamsW(hDeviceInfo, NULL, &InstallParams))
            goto cleanup;

        /* Open the .inf file */
        hInf = SetupOpenInfFileW(
            InfFileName,
            NULL,
            INF_STYLE_WIN4,
            NULL);
        if (hInf == INVALID_HANDLE_VALUE)
            goto cleanup;

        /* Try to append a layout file */
        SetupOpenAppendInfFileW(NULL, hInf, NULL);

        if (InterfaceClassGuid)
        {
            /* Retrieve the actual section name */
            ret = SetupDiGetActualSectionToInstallW(
                hInf,
                InterfaceInstall32,
                SectionName,
                MAX_PATH,
                NULL,
                NULL);
            if (!ret)
                goto cleanup;

            /* Open registry key related to this interface */
            /* FIXME: What happens if the key doesn't exist? */
            hRootKey = SetupDiOpenClassRegKeyExW(InterfaceClassGuid, KEY_ENUMERATE_SUB_KEYS, DIOCR_INTERFACE, NULL, NULL);
            if (hRootKey == INVALID_HANDLE_VALUE)
                goto cleanup;

            /* SetupDiCreateDeviceInterface??? */
            FIXME("Installing an interface is not implemented\n");
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        }
        else
        {
            /* Create or open the class registry key 'HKLM\CurrentControlSet\Class\{GUID}' */
            hRootKey = SETUP_CreateClassKey(hInf);
            if (hRootKey == INVALID_HANDLE_VALUE)
                goto cleanup;

            /* Retrieve the actual section name */
            ret = SetupDiGetActualSectionToInstallW(
                hInf,
                ClassInstall32,
                SectionName,
                MAX_PATH - strlenW(DotServices),
                NULL,
                NULL);
            if (!ret)
                goto cleanup;

            callback_context = SetupInitDefaultQueueCallback(hwndParent);
            if (!callback_context)
                goto cleanup;

            ret = SetupInstallFromInfSectionW(
                hwndParent,
                hInf,
                SectionName,
                SPINST_REGISTRY | SPINST_FILES | SPINST_BITREG | SPINST_INIFILES | SPINST_INI2REG,
                hRootKey,
                NULL, /* FIXME: SourceRootPath */
                !(Flags & DI_NOVCP) && (Flags & DI_FORCECOPY) ? SP_COPY_FORCE_IN_USE : 0, /* CopyFlags */
                SetupDefaultQueueCallbackW,
                callback_context,
                hDeviceInfo,
                NULL);
            if (!ret)
                goto cleanup;

            /* OPTIONAL: Install .Services section */
            lstrcatW(SectionName, DotServices);
            SetupInstallServicesFromInfSectionExW(
                hInf,
                SectionName,
                0,
                hDeviceInfo,
                NULL,
                NULL,
                NULL);
            ret = TRUE;
        }

cleanup:
        if (hDeviceInfo != INVALID_HANDLE_VALUE)
            SetupDiDestroyDeviceInfoList(hDeviceInfo);
        if (hInf != INVALID_HANDLE_VALUE)
            SetupCloseInfFile(hInf);
        if (hRootKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hRootKey);
        SetupTermDefaultQueueCallback(callback_context);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		Helper functions for SetupDiSetClassInstallParamsW
 */
static BOOL
SETUP_PropertyChangeHandler(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize)
{
    PSP_PROPCHANGE_PARAMS PropChangeParams = (PSP_PROPCHANGE_PARAMS)ClassInstallParams;
    BOOL ret = FALSE;

    if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (ClassInstallParamsSize != sizeof(SP_PROPCHANGE_PARAMS))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (PropChangeParams && PropChangeParams->StateChange != DICS_ENABLE
        && PropChangeParams->StateChange != DICS_DISABLE && PropChangeParams->StateChange != DICS_PROPCHANGE
        && PropChangeParams->StateChange != DICS_START && PropChangeParams->StateChange != DICS_STOP)
        SetLastError(ERROR_INVALID_FLAGS);
    else if (PropChangeParams && PropChangeParams->Scope != DICS_FLAG_GLOBAL
        && PropChangeParams->Scope != DICS_FLAG_CONFIGSPECIFIC)
        SetLastError(ERROR_INVALID_FLAGS);
    else if (PropChangeParams
        && (PropChangeParams->StateChange == DICS_START || PropChangeParams->StateChange == DICS_STOP)
        && PropChangeParams->Scope != DICS_FLAG_CONFIGSPECIFIC)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        PSP_PROPCHANGE_PARAMS *CurrentPropChangeParams;
        struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
        CurrentPropChangeParams = &deviceInfo->ClassInstallParams.PropChangeParams;

        if (*CurrentPropChangeParams)
        {
            MyFree(*CurrentPropChangeParams);
            *CurrentPropChangeParams = NULL;
        }
        if (PropChangeParams)
        {
            *CurrentPropChangeParams = MyMalloc(ClassInstallParamsSize);
            if (!*CurrentPropChangeParams)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto done;
            }
            memcpy(*CurrentPropChangeParams, PropChangeParams, ClassInstallParamsSize);
        }
        ret = TRUE;
    }

done:
    return ret;
}

static BOOL
SETUP_PropertyAddPropertyAdvancedHandler(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize)
{
    PSP_ADDPROPERTYPAGE_DATA AddPropertyPageData = (PSP_ADDPROPERTYPAGE_DATA)ClassInstallParams;
    BOOL ret = FALSE;

    if (ClassInstallParamsSize != sizeof(SP_PROPCHANGE_PARAMS))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (AddPropertyPageData && AddPropertyPageData->Flags != 0)
        SetLastError(ERROR_INVALID_FLAGS);
    else if (AddPropertyPageData && AddPropertyPageData->NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        PSP_ADDPROPERTYPAGE_DATA *CurrentAddPropertyPageData;
        if (!DeviceInfoData)
        {
            struct DeviceInfoSet *list = (struct DeviceInfoSet *)DeviceInfoSet;
            CurrentAddPropertyPageData = &list->ClassInstallParams.AddPropertyPageData;
        }
        else
        {
            struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
            CurrentAddPropertyPageData = &deviceInfo->ClassInstallParams.AddPropertyPageData;
        }
        if (*CurrentAddPropertyPageData)
        {
            MyFree(*CurrentAddPropertyPageData);
            *CurrentAddPropertyPageData = NULL;
        }
        if (AddPropertyPageData)
        {
            *CurrentAddPropertyPageData = MyMalloc(ClassInstallParamsSize);
            if (!*CurrentAddPropertyPageData)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto done;
            }
            memcpy(*CurrentAddPropertyPageData, AddPropertyPageData, ClassInstallParamsSize);
        }
        ret = TRUE;
    }

done:
    return ret;
}

/***********************************************************************
 *		SetupDiSetClassInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetClassInstallParamsW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams OPTIONAL,
    IN DWORD ClassInstallParamsSize)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p %p %lu\n", DeviceInfoSet, DeviceInfoData,
        ClassInstallParams, ClassInstallParamsSize);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (ClassInstallParams && ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (ClassInstallParams && ClassInstallParamsSize < sizeof(SP_CLASSINSTALL_HEADER))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!ClassInstallParams && ClassInstallParamsSize != 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        SP_DEVINSTALL_PARAMS_W InstallParams;
        BOOL Result;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!Result)
            goto done;

        if (ClassInstallParams)
        {
            DWORD i;
            /* Check parameters in ClassInstallParams */
            for (i = 0; i < sizeof(InstallParamsData) / sizeof(InstallParamsData[0]); i++)
            {
                if (InstallParamsData[i].Function == ClassInstallParams->InstallFunction)
                {
                    ret = InstallParamsData[i].UpdateHandler(
                        DeviceInfoSet,
                        DeviceInfoData,
                        ClassInstallParams,
                        ClassInstallParamsSize);
                    if (ret)
                    {
                        InstallParams.Flags |= DI_CLASSINSTALLPARAMS;
                        ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
                    }
                    goto done;
                }
            }
            ERR("InstallFunction %u has no associated update handler\n", ClassInstallParams->InstallFunction);
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            goto done;
        }
        else
        {
            InstallParams.Flags &= ~DI_CLASSINSTALLPARAMS;
            ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        }
    }

done:
    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassDevPropertySheetsA(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassDevPropertySheetsA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN LPPROPSHEETHEADERA PropertySheetHeader,
    IN DWORD PropertySheetHeaderPageListSize,
    OUT PDWORD RequiredSize OPTIONAL,
    IN DWORD PropertySheetType)
{
    PROPSHEETHEADERW psh;
    BOOL ret = FALSE;

    TRACE("%p %p %p 0%lx %p 0x%lx\n", DeviceInfoSet, DeviceInfoData,
        PropertySheetHeader, PropertySheetHeaderPageListSize,
        RequiredSize, PropertySheetType);

    psh.dwFlags = PropertySheetHeader->dwFlags;
    psh.phpage = PropertySheetHeader->phpage;
    psh.nPages = PropertySheetHeader->nPages;

    ret = SetupDiGetClassDevPropertySheetsW(DeviceInfoSet, DeviceInfoData, PropertySheetHeader ? &psh : NULL,
                                            PropertySheetHeaderPageListSize, RequiredSize,
                                            PropertySheetType);
    if (ret)
    {
        PropertySheetHeader->nPages = psh.nPages;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

struct ClassDevPropertySheetsData
{
    LPPROPSHEETHEADERW PropertySheetHeader;
    DWORD PropertySheetHeaderPageListSize;
    DWORD NumberOfPages;
    BOOL DontCancel;
};

static BOOL WINAPI
SETUP_GetClassDevPropertySheetsCallback(
    IN HPROPSHEETPAGE hPropSheetPage,
    IN OUT LPARAM lParam)
{
    struct ClassDevPropertySheetsData *PropPageData;

    PropPageData = (struct ClassDevPropertySheetsData *)lParam;

    PropPageData->NumberOfPages++;

    if (PropPageData->PropertySheetHeader->nPages < PropPageData->PropertySheetHeaderPageListSize)
    {
        PropPageData->PropertySheetHeader->phpage[PropPageData->PropertySheetHeader->nPages] = hPropSheetPage;
        PropPageData->PropertySheetHeader->nPages++;
        return TRUE;
    }

    return PropPageData->DontCancel;
}

/***********************************************************************
 *		SetupDiGetClassDevPropertySheetsW(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetClassDevPropertySheetsW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT LPPROPSHEETHEADERW PropertySheetHeader,
    IN DWORD PropertySheetHeaderPageListSize,
    OUT PDWORD RequiredSize OPTIONAL,
    IN DWORD PropertySheetType)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p %p 0%lx %p 0x%lx\n", DeviceInfoSet, DeviceInfoData,
        PropertySheetHeader, PropertySheetHeaderPageListSize,
        RequiredSize, PropertySheetType);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!PropertySheetHeader)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (PropertySheetHeader->dwFlags & PSH_PROPSHEETPAGE)
        SetLastError(ERROR_INVALID_FLAGS);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!DeviceInfoData && IsEqualIID(&list->ClassGuid, &GUID_NULL))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (PropertySheetType != DIGCDP_FLAG_ADVANCED
          && PropertySheetType != DIGCDP_FLAG_BASIC
          && PropertySheetType != DIGCDP_FLAG_REMOTE_ADVANCED
          && PropertySheetType != DIGCDP_FLAG_REMOTE_BASIC)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        HKEY hKey = INVALID_HANDLE_VALUE;
        SP_PROPSHEETPAGE_REQUEST Request;
        LPWSTR PropPageProvider = NULL;
        HMODULE hModule = NULL;
        PROPERTY_PAGE_PROVIDER pPropPageProvider = NULL;
        struct ClassDevPropertySheetsData PropPageData;
        DWORD dwLength, dwRegType;
        DWORD InitialNumberOfPages;
        DWORD rc;

        if (DeviceInfoData)
            hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_QUERY_VALUE);
        else
        {
            hKey = SetupDiOpenClassRegKeyExW(&list->ClassGuid, KEY_QUERY_VALUE,
                DIOCR_INSTALLER, list->MachineName + 2, NULL);
        }
        if (hKey == INVALID_HANDLE_VALUE)
            goto cleanup;

        rc = RegQueryValueExW(hKey, REGSTR_VAL_ENUMPROPPAGES_32, NULL, &dwRegType, NULL, &dwLength);
        if (rc == ERROR_FILE_NOT_FOUND)
        {
            /* No registry key. As it is optional, don't say it's a bad error */
            if (RequiredSize)
                *RequiredSize = 0;
            ret = TRUE;
            goto cleanup;
        }
        else if (rc != ERROR_SUCCESS && dwRegType != REG_SZ)
        {
            SetLastError(rc);
            goto cleanup;
        }

        PropPageProvider = HeapAlloc(GetProcessHeap(), 0, dwLength + sizeof(WCHAR));
        if (!PropPageProvider)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        rc = RegQueryValueExW(hKey, REGSTR_VAL_ENUMPROPPAGES_32, NULL, NULL, (LPBYTE)PropPageProvider, &dwLength);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        PropPageProvider[dwLength / sizeof(WCHAR)] = 0;

        rc = GetFunctionPointer(PropPageProvider, &hModule, (PVOID*)&pPropPageProvider);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PROPPAGE_PROVIDER);
            goto cleanup;
        }

        if (DeviceInfoData)
        {
            struct DeviceInfo *devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

            if (devInfo->hmodDevicePropPageProvider == NULL)
            {
                devInfo->hmodDevicePropPageProvider = hModule;
                devInfo->pDevicePropPageProvider = pPropPageProvider;
            }
        }
        else
        {
            struct DeviceInfoSet *devInfoSet = (struct DeviceInfoSet *)DeviceInfoSet;

            if (devInfoSet->hmodClassPropPageProvider == NULL)
            {
                devInfoSet->hmodClassPropPageProvider = hModule;
                devInfoSet->pClassPropPageProvider = pPropPageProvider;
            }
        }

        InitialNumberOfPages = PropertySheetHeader->nPages;

        Request.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
        Request.PageRequested = SPPSR_ENUM_ADV_DEVICE_PROPERTIES;
        Request.DeviceInfoSet = DeviceInfoSet;
        Request.DeviceInfoData = DeviceInfoData;

        PropPageData.PropertySheetHeader = PropertySheetHeader;
        PropPageData.PropertySheetHeaderPageListSize = PropertySheetHeaderPageListSize;
        PropPageData.NumberOfPages = 0;
        PropPageData.DontCancel = (RequiredSize != NULL) ? TRUE : FALSE;

        pPropPageProvider(&Request, SETUP_GetClassDevPropertySheetsCallback, (LPARAM)&PropPageData);

        if (RequiredSize)
            *RequiredSize = PropPageData.NumberOfPages;

        if (InitialNumberOfPages + PropPageData.NumberOfPages <= PropertySheetHeaderPageListSize)
        {
            ret = TRUE;
        }
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }

cleanup:
        if (hKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hKey);
        HeapFree(GetProcessHeap(), 0, PropPageProvider);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}
