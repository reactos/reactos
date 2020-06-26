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

#include "misc.h"

BOOL CInstalledApplicationInfo::GetApplicationString(LPCWSTR lpKeyName, ATL::CStringW& String)
{
    DWORD dwSize = 0;
    String.Empty();

    // retrieve the size of value first.
    // TODO: I assume the type as REG_SZ. but I think REG_EXPAND_SZ should be handled correctly too.
    if (RegQueryValueExW(hSubKey,
        lpKeyName,
        NULL,
        NULL,
        NULL,
        &dwSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // allocate buffer.
    // attention: dwSize is size in bytes, and RegQueryValueExW does not guarantee the terminating null character.
    String.GetBuffer(dwSize + sizeof(WCHAR));

    // query the value
    if (RegQueryValueExW(hSubKey,
        lpKeyName,
        NULL,
        NULL,
        (LPBYTE)String.GetBuffer(),
        &dwSize) != ERROR_SUCCESS)
    {
        String.ReleaseBuffer();
        String.Empty();
        return FALSE;
    }
    String.GetBuffer()[dwSize / sizeof(WCHAR)] = L'0'; // ensure zero terminated
    String.ReleaseBuffer();
    return TRUE;
}

BOOL CInstalledApplicationInfo::UninstallApplication(BOOL bModify)
{
    LPCWSTR szModify = L"ModifyPath";
    LPCWSTR szUninstall = L"UninstallString";
    DWORD dwType, dwSize;
    WCHAR szPath[MAX_PATH];

    dwType = REG_SZ;
    dwSize = MAX_PATH * sizeof(WCHAR);
    if (RegQueryValueExW(hSubKey,
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
    CInstalledApplicationInfo Info;
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
