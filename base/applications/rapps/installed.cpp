/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/installed.cpp
 * PURPOSE:     Functions for working with installed applications
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev         (dmitry@reactos.org)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "installed.h"

#include "gui.h"
#include "misc.h"

BOOL INSTALLED_INFO::GetApplicationString(LPCWSTR lpKeyName, ATL::CStringW& String)
{
    BOOL result = ::GetApplicationString(hSubKey, lpKeyName, String.GetBuffer(MAX_PATH));
    String.ReleaseBuffer();
    return result;
}

BOOL GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString)
{
    DWORD dwSize = MAX_PATH * sizeof(WCHAR);

    if (RegQueryValueExW(hKey,
                         lpKeyName,
                         NULL,
                         NULL,
                         (LPBYTE) szString,
                         &dwSize) == ERROR_SUCCESS)
    {
        return TRUE;
    }

    StringCchCopyW(szString, MAX_PATH, L"---");
    return FALSE;
}

BOOL UninstallApplication(INT Index, BOOL bModify)
{
    LPCWSTR szModify = L"ModifyPath";
    LPCWSTR szUninstall = L"UninstallString";
    WCHAR szPath[MAX_PATH];
    WCHAR szAppName[MAX_STR_LEN];
    DWORD dwType, dwSize;
    INT ItemIndex;
    LVITEMW Item;
    HKEY hKey;
    PINSTALLED_INFO ItemInfo;

    if (!IsInstalledEnum(SelectedEnumType))
        return FALSE;

    if (Index == -1)
    {
        ItemIndex = (INT) SendMessageW(hListView, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
        if (ItemIndex == -1)
            return FALSE;
    }
    else
    {
        ItemIndex = Index;
    }

    ListView_GetItemText(hListView, ItemIndex, 0, szAppName, _countof(szAppName));
    WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_REMOVE, szAppName);

    ZeroMemory(&Item, sizeof(Item));

    Item.mask = LVIF_PARAM;
    Item.iItem = ItemIndex;
    if (!ListView_GetItem(hListView, &Item))
        return FALSE;

    ItemInfo = (PINSTALLED_INFO) Item.lParam;
    hKey = ItemInfo->hSubKey;

    dwType = REG_SZ;
    dwSize = MAX_PATH * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
                         bModify ? szModify : szUninstall,
                         NULL,
                         &dwType,
                         (LPBYTE) szPath,
                         &dwSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return StartProcess(szPath, TRUE);
}

BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc, PVOID param)
{
    DWORD dwSize = MAX_PATH, dwType, dwValue;
    BOOL bIsSystemComponent, bIsUpdate;
    ATL::CStringW szParentKeyName;
    ATL::CStringW szDisplayName;
    INSTALLED_INFO Info;
    HKEY hKey;
    LONG ItemIndex = 0;

    Info.hRootKey = IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

    if (RegOpenKeyW(Info.hRootKey,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                    &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    while (RegEnumKeyExW(hKey, ItemIndex, Info.szKeyName.GetBuffer(MAX_PATH), &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        Info.szKeyName.ReleaseBuffer();
        if (RegOpenKeyW(hKey, Info.szKeyName.GetString(), &Info.hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);

            if (RegQueryValueExW(Info.hSubKey,
                                 L"SystemComponent",
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwValue,
                                 &dwSize) == ERROR_SUCCESS)
            {
                bIsSystemComponent = (dwValue == 0x1);
            }
            else
            {
                bIsSystemComponent = FALSE;
            }

            dwType = REG_SZ;
            dwSize = MAX_PATH * sizeof(WCHAR);
            bIsUpdate = (RegQueryValueExW(Info.hSubKey,
                                          L"ParentKeyName",
                                          NULL,
                                          &dwType,
                                          (LPBYTE) szParentKeyName.GetBuffer(MAX_PATH),
                                          &dwSize) == ERROR_SUCCESS);
            szParentKeyName.ReleaseBuffer();

            dwType = REG_SZ;
            dwSize = MAX_PATH * sizeof(WCHAR);
            if (RegQueryValueExW(Info.hSubKey,
                                 L"DisplayName",
                                 NULL,
                                 &dwType,
                                 (LPBYTE) szDisplayName.GetBuffer(MAX_PATH),
                                 &dwSize) == ERROR_SUCCESS)
            {
                szDisplayName.ReleaseBuffer();
                if (EnumType < ENUM_ALL_INSTALLED || EnumType > ENUM_UPDATES)
                    EnumType = ENUM_ALL_INSTALLED;

                if (!bIsSystemComponent)
                {
                    if ((EnumType == ENUM_ALL_INSTALLED) || /* All components */
                        ((EnumType == ENUM_INSTALLED_APPLICATIONS) && (!bIsUpdate)) || /* Applications only */
                        ((EnumType == ENUM_UPDATES) && (bIsUpdate))) /* Updates only */
                    {
                        if (!lpEnumProc(ItemIndex, szDisplayName, &Info, param))
                            break;
                    }
                    else
                    {
                        RegCloseKey(Info.hSubKey);
                    }
                }
                else
                {
                    RegCloseKey(Info.hSubKey);
                }
            }
            else
            {
                szDisplayName.ReleaseBuffer();
                RegCloseKey(Info.hSubKey);
            }
        }

        dwSize = MAX_PATH;
        ItemIndex++;
    }

    Info.szKeyName.ReleaseBuffer();
    RegCloseKey(hKey);

    return TRUE;
}
