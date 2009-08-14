/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/installed.c
 * PURPOSE:         Functions for working with installed applications
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"


BOOL
GetApplicationString(HKEY hKey, LPWSTR lpKeyName, LPWSTR lpString)
{
    DWORD dwSize = MAX_PATH;

    if (RegQueryValueExW(hKey,
                         lpKeyName,
                         NULL,
                         NULL,
                         (LPBYTE)lpString,
                         &dwSize) == ERROR_SUCCESS)
    {
        return TRUE;
    }

    wcscpy(lpString, L"---");

    return FALSE;
}


BOOL
IsInstalledApplication(LPWSTR lpRegName)
{
    DWORD dwSize = MAX_PATH, dwType;
    WCHAR szName[MAX_PATH];
    WCHAR szDisplayName[MAX_PATH];
    HKEY hKey, hSubKey;
    INT ItemIndex = 0;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                    &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    while (RegEnumKeyExW(hKey, ItemIndex, szName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (RegOpenKeyW(hKey, szName, &hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_SZ;
            dwSize = MAX_PATH;
            if (RegQueryValueExW(hSubKey,
                                 L"DisplayName",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)szDisplayName,
                                 &dwSize) == ERROR_SUCCESS)
            {
                if (wcscmp(szDisplayName, lpRegName) == 0)
                {
                    RegCloseKey(hSubKey);
                    RegCloseKey(hKey);
                    return TRUE;
                }
            }
        }

        RegCloseKey(hSubKey);
        dwSize = MAX_PATH;
        ItemIndex++;
    }

    RegCloseKey(hKey);
    return FALSE;
}


BOOL
UninstallApplication(INT Index, BOOL bModify)
{
    WCHAR szModify[] = L"ModifyPath";
    WCHAR szUninstall[] = L"UninstallString";
    WCHAR szPath[MAX_PATH];
    DWORD dwType, dwSize;
    INT ItemIndex;
    LVITEM Item;
    HKEY hKey;

    if (!IS_INSTALLED_ENUM(SelectedEnumType))
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

    ZeroMemory(&Item, sizeof(LVITEM));

    Item.mask = LVIF_PARAM;
    Item.iItem = ItemIndex;
    if (!ListView_GetItem(hListView, &Item))
        return FALSE;

    hKey = (HKEY)Item.lParam;

    dwType = REG_SZ;
    dwSize = MAX_PATH;
    if (RegQueryValueExW(hKey,
                         bModify ? szModify : szUninstall,
                         NULL,
                         &dwType,
                         (LPBYTE)szPath,
                         &dwSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return StartProcess(szPath, TRUE);
}


BOOL
ShowInstalledAppInfo(INT Index)
{
    WCHAR szText[MAX_PATH], szInfo[MAX_PATH];
    HKEY hKey = (HKEY) ListViewGetlParam(Index);

    if (!hKey) return FALSE;

    GetApplicationString(hKey, L"DisplayName", szText);
    NewRichEditText(szText, CFE_BOLD);

    InsertRichEditText(L"\n", 0);

#define GET_INFO(a, b, c, d) \
    if (GetApplicationString(hKey, a, szInfo)) \
    { \
        LoadStringW(hInst, b, szText, sizeof(szText) / sizeof(WCHAR)); \
        InsertRichEditText(szText, c); \
        InsertRichEditText(szInfo, d); \
    } \

    GET_INFO(L"DisplayVersion", IDS_INFO_VERSION, CFE_BOLD, 0);
    GET_INFO(L"Publisher", IDS_INFO_PUBLISHER, CFE_BOLD, 0);
    GET_INFO(L"RegOwner", IDS_INFO_REGOWNER, CFE_BOLD, 0);
    GET_INFO(L"ProductID", IDS_INFO_PRODUCTID, CFE_BOLD, 0);
    GET_INFO(L"HelpLink", IDS_INFO_HELPLINK, CFE_BOLD, CFM_LINK);
    GET_INFO(L"HelpTelephone", IDS_INFO_HELPPHONE, CFE_BOLD, 0);
    GET_INFO(L"Readme", IDS_INFO_README, CFE_BOLD, 0);
    GET_INFO(L"Contact", IDS_INFO_CONTACT, CFE_BOLD, 0);
    GET_INFO(L"URLUpdateInfo", IDS_INFO_UPDATEINFO, CFE_BOLD, CFM_LINK);
    GET_INFO(L"URLInfoAbout", IDS_INFO_INFOABOUT, CFE_BOLD, CFM_LINK);
    GET_INFO(L"Comments", IDS_INFO_COMMENTS, CFE_BOLD, 0);
    GET_INFO(L"InstallDate", IDS_INFO_INSTALLDATE, CFE_BOLD, 0);
    GET_INFO(L"InstallLocation", IDS_INFO_INSTLOCATION, CFE_BOLD, 0);
    GET_INFO(L"InstallSource", IDS_INFO_INSTALLSRC, CFE_BOLD, 0);
    GET_INFO(L"UninstallString", IDS_INFO_UNINSTALLSTR, CFE_BOLD, 0);
    GET_INFO(L"InstallSource", IDS_INFO_INSTALLSRC, CFE_BOLD, 0);
    GET_INFO(L"ModifyPath", IDS_INFO_MODIFYPATH, CFE_BOLD, 0);

    return TRUE;
}


BOOL
EnumInstalledApplications(INT EnumType, APPENUMPROC lpEnumProc)
{
    DWORD dwSize = MAX_PATH, dwType, dwValue;
    BOOL bIsSystemComponent, bIsUpdate;
    WCHAR pszName[MAX_PATH];
    WCHAR pszParentKeyName[MAX_PATH];
    WCHAR pszDisplayName[MAX_PATH];
    HKEY hKey, hSubKey;
    LONG ItemIndex = 0;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                    &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    while (RegEnumKeyExW(hKey, ItemIndex, pszName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (RegOpenKeyW(hKey, pszName, &hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);

            if (RegQueryValueExW(hSubKey,
                                 L"SystemComponent",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &dwSize) == ERROR_SUCCESS)
            {
                bIsSystemComponent = (dwValue == 0x1);
            }
            else
            {
                bIsSystemComponent = FALSE;
            }

            dwType = REG_SZ;
            dwSize = MAX_PATH;
            bIsUpdate = (RegQueryValueExW(hSubKey,
                                          L"ParentKeyName",
                                          NULL,
                                          &dwType,
                                          (LPBYTE)pszParentKeyName,
                                          &dwSize) == ERROR_SUCCESS);

            dwSize = MAX_PATH;
            if (RegQueryValueExW(hSubKey,
                                 L"DisplayName",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)pszDisplayName,
                                 &dwSize) == ERROR_SUCCESS)
            {
                if (EnumType < ENUM_ALL_COMPONENTS || EnumType > ENUM_UPDATES)
                    EnumType = ENUM_ALL_COMPONENTS;

                if (!bIsSystemComponent)
                {
                    if ((EnumType == ENUM_ALL_COMPONENTS) || /* All components */
                        ((EnumType == ENUM_APPLICATIONS) && (!bIsUpdate)) || /* Applications only */
                        ((EnumType == ENUM_UPDATES) && (bIsUpdate))) /* Updates only */
                    {
                        if (!lpEnumProc(ItemIndex, pszDisplayName, pszName, (LPARAM)hSubKey))
                            break;
                    }
                }
            }
        }

        dwSize = MAX_PATH;
        ItemIndex++;
    }

    RegCloseKey(hSubKey);
    RegCloseKey(hKey);

    return TRUE;
}
