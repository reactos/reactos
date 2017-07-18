/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/installed.cpp
 * PURPOSE:         Functions for working with installed applications
 * PROGRAMMERS:     Dmitry Chapyshev            (dmitry@reactos.org)
 *                  Alexander Shaposhnikov      (chaez.san@gmail.com)
 */

#include "rapps.h"

BOOL
GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, ATL::CStringW& String)
{
    BOOL result = GetApplicationString(hKey, lpKeyName, String.GetBuffer(MAX_PATH));
    String.ReleaseBuffer();
    return result;
}

BOOL
GetApplicationString(HKEY hKey, LPCWSTR lpKeyName, LPWSTR szString)
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

BOOL
GetInstalledVersion_WowUser(_Out_opt_ ATL::CStringW* szVersionResult, 
                            _In_z_ const ATL::CStringW& RegName, 
                            _In_ BOOL IsUserKey, 
                            _In_ REGSAM keyWow)
{
    HKEY hKey;
    BOOL bHasSucceded = FALSE;
    ATL::CStringW szVersion;
    ATL::CStringW szPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + RegName;

    if (RegOpenKeyExW(IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
                      szPath.GetString(), 0, keyWow | KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        if (szVersionResult != NULL)
        {
            DWORD dwSize = MAX_PATH * sizeof(WCHAR);
            DWORD dwType = REG_SZ;
            if (RegQueryValueExW(hKey,
                                 L"DisplayVersion",
                                 NULL,
                                 &dwType,
                                 (LPBYTE) szVersion.GetBuffer(MAX_PATH),
                                 &dwSize) == ERROR_SUCCESS)
            {
                szVersion.ReleaseBuffer();
                *szVersionResult = szVersion;
                bHasSucceded = TRUE;
            }
            else
            {
                szVersion.ReleaseBuffer();
            }
        }
        else
        {
            bHasSucceded = TRUE;
            szVersion.ReleaseBuffer();
        }
        
    }

    RegCloseKey(hKey);
    return bHasSucceded;
}

BOOL GetInstalledVersion(ATL::CStringW* pszVersion, const ATL::CStringW& szRegName)
{
    return (!szRegName.IsEmpty()
            && (::GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_32KEY)
                || ::GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_32KEY)
                || ::GetInstalledVersion_WowUser(pszVersion, szRegName, TRUE, KEY_WOW64_64KEY)
                || ::GetInstalledVersion_WowUser(pszVersion, szRegName, FALSE, KEY_WOW64_64KEY)));
}

BOOL
UninstallApplication(INT Index, BOOL bModify)
{
    LPCWSTR szModify = L"ModifyPath";
    LPCWSTR szUninstall = L"UninstallString";
    WCHAR szPath[MAX_PATH];
    WCHAR szAppName[MAX_STR_LEN];
    DWORD dwType, dwSize;
    INT ItemIndex;
    LVITEM Item;
    HKEY hKey;
    PINSTALLED_INFO ItemInfo;

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


BOOL
ShowInstalledAppInfo(INT Index)
{
    ATL::CStringW szText;
    ATL::CStringW szInfo;
    PINSTALLED_INFO Info = (PINSTALLED_INFO) ListViewGetlParam(Index);

    if (!Info || !Info->hSubKey) return FALSE;

    GetApplicationString(Info->hSubKey, L"DisplayName", szText);
    NewRichEditText(szText, CFE_BOLD);
    InsertRichEditText(L"\n", 0);

#define GET_INFO(a, b, c, d) \
    if (GetApplicationString(Info->hSubKey, a, szInfo)) \
    { \
        szText.LoadStringW(hInst, b); \
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


VOID
RemoveAppFromRegistry(INT Index)
{
    PINSTALLED_INFO Info;
    WCHAR szFullName[MAX_PATH] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    ATL::CStringW szMsgText, szMsgTitle;
    INT ItemIndex = SendMessage(hListView, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

    if (!IS_INSTALLED_ENUM(SelectedEnumType))
        return;

    Info = (PINSTALLED_INFO) ListViewGetlParam(Index);
    if (!Info || !Info->hSubKey || (ItemIndex == -1)) return;

    if (!szMsgText.LoadStringW(hInst, IDS_APP_REG_REMOVE) ||
        !szMsgTitle.LoadStringW(hInst, IDS_INFORMATION))
        return;

    if (MessageBoxW(hMainWnd, szMsgText, szMsgTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        ATL::CStringW::CopyChars(szFullName, 
                                 MAX_PATH, 
                                 Info->szKeyName.GetString(), 
                                 MAX_PATH - wcslen(szFullName));

        if (RegDeleteKeyW(Info->hRootKey, szFullName) == ERROR_SUCCESS)
        {
            ListView_DeleteItem(hListView, ItemIndex);
            return;
        }

        if (!szMsgText.LoadStringW(hInst, IDS_UNABLE_TO_REMOVE))
            return;

        MessageBoxW(hMainWnd, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
    }
}


BOOL
EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc)
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
                if (EnumType < ENUM_ALL_COMPONENTS || EnumType > ENUM_UPDATES)
                    EnumType = ENUM_ALL_COMPONENTS;

                if (!bIsSystemComponent)
                {
                    if ((EnumType == ENUM_ALL_COMPONENTS) || /* All components */
                        ((EnumType == ENUM_APPLICATIONS) && (!bIsUpdate)) || /* Applications only */
                        ((EnumType == ENUM_UPDATES) && (bIsUpdate))) /* Updates only */
                    {
                        if (!lpEnumProc(ItemIndex, szDisplayName, &Info))
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
