/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with installed applications
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev         (dmitry@reactos.org)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 *              Copyright 2020 He Yang                  (1160386205@qq.com)
 */
#include "rapps.h"

#include "installed.h"

#include "misc.h"

CInstalledApplicationInfo::CInstalledApplicationInfo(BOOL bIsUserKey, REGSAM RegWowKey, HKEY hKey)
    : IsUserKey(bIsUserKey), WowKey(RegWowKey), hSubKey(hKey)
{
    // if Initialize failed, hSubKey will be closed automatically and set to zero

    DWORD dwSize = MAX_PATH, dwType, dwValue;
    BOOL bIsSystemComponent;
    ATL::CStringW szParentKeyName;

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
    dwSize = MAX_PATH * sizeof(WCHAR);
    bIsUpdate = (RegQueryValueExW(hSubKey,
        L"ParentKeyName",
        NULL,
        &dwType,
        (LPBYTE)szParentKeyName.GetBuffer(MAX_PATH),
        &dwSize) == ERROR_SUCCESS);
    szParentKeyName.ReleaseBuffer();

    if (bIsSystemComponent)
    {
        CloseHandle(hSubKey);
        hSubKey = NULL;
    }

}

CInstalledApplicationInfo::~CInstalledApplicationInfo()
{
    if (hSubKey)
    {
        CloseHandle(hSubKey);
        hSubKey = NULL;
    }
}

BOOL CInstalledApplicationInfo::GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String)
{
    DWORD dwSize = 0;
    String.Empty();
    DWORD dwType;

    // retrieve the size of value first.
    if (RegQueryValueExW(hSubKey,
        lpKeyName,
        NULL,
        &dwType,
        NULL,
        &dwSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // TODO: I assume the type as REG_SZ. but I think REG_EXPAND_SZ should be handled correctly too.
    if (dwType != REG_SZ)
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
    String.GetBuffer()[dwSize / sizeof(WCHAR)] = L'\0'; // ensure zero terminated
    String.ReleaseBuffer();
    return TRUE;
}

BOOL CInstalledApplicationInfo::GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue)
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hSubKey,
        lpKeyName,
        NULL,
        &dwType,
        (LPBYTE)lpValue,
        &dwSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CInstalledApplicationInfo::RetrieveIcon(ATL::CStringW& IconLocation)
{
    if (szDisplayIcon.IsEmpty())
    {
        return FALSE;
    }
    IconLocation = szDisplayIcon;
    return TRUE;
}

BOOL CInstalledApplicationInfo::UninstallApplication(BOOL bModify)
{
    if (!bModify)
        WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_REMOVE, szDisplayName);

    return StartProcess(bModify ? szModifyPath : szUninstallString, TRUE);
}

typedef LSTATUS (WINAPI *RegDeleteKeyExWProc)(HKEY, LPCWSTR, REGSAM, DWORD);

LSTATUS CInstalledApplicationInfo::RemoveFromRegistry()
{
    ATL::CStringW szFullName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + szKeyName;
    HMODULE hMod = GetModuleHandleW(L"advapi32.dll");
    RegDeleteKeyExWProc pRegDeleteKeyExW;

    // TODO: if there are subkeys inside, simply RegDeleteKeyExW
    // (or RegDeleteKeyW on Server 2003 SP0 and earlier) will fail
    // we don't have RegDeleteTree for ReactOS now. (It's a WinVista API)
    // write a function to delete all subkeys recursively to solve this
    // or consider letting ReactOS having this API

    /* Load RegDeleteKeyExW from advapi32.dll if available */
    if (hMod)
    {
        pRegDeleteKeyExW = (RegDeleteKeyExWProc)GetProcAddress(hMod, "RegDeleteKeyExW");

        if (pRegDeleteKeyExW)
        {
            /* Return it */
            return pRegDeleteKeyExW(IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szFullName, WowKey, 0);
        }
    }

    /* Otherwise, return non-Ex function */
    return RegDeleteKeyW(IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szFullName);
}

BOOL CInstalledApps::Enum(INT EnumType, APPENUMPROC lpEnumProc, PVOID param)
{
    FreeCachedEntries();

    HKEY RootKeyEnum[3]  = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_LOCAL_MACHINE };
    REGSAM RegSamEnum[3] = { KEY_WOW64_32KEY,   KEY_WOW64_32KEY,    KEY_WOW64_64KEY    };

    int LoopTime;

    // test if the OS is 64 bit.
    if (IsSystem64Bit())
    {
        // loop for all 3 combination.
        // note that HKEY_CURRENT_USER\Software don't have a redirect
        // https://docs.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys#redirected-shared-and-reflected-keys-under-wow64
        LoopTime = 3;
    }
    else
    {
        // loop for 2 combination for KEY_WOW64_32KEY only
        LoopTime = 2;
    }

    // loop for all combination
    for (int i = 0; i < LoopTime; i++)
    {
        DWORD dwSize = MAX_PATH;
        HKEY hKey, hSubKey;
        LONG ItemIndex = 0;
        ATL::CStringW szKeyName;

        if (RegOpenKeyExW(RootKeyEnum[i],
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            NULL,
            KEY_READ | RegSamEnum[i],
            &hKey) != ERROR_SUCCESS)
        {
            return FALSE;
        }

        while (1)
        {
            dwSize = MAX_PATH;
            if (RegEnumKeyExW(hKey, ItemIndex, szKeyName.GetBuffer(MAX_PATH), &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            {
                break;
            }

            ItemIndex++;

            szKeyName.ReleaseBuffer();
            if (RegOpenKeyW(hKey, szKeyName.GetString(), &hSubKey) == ERROR_SUCCESS)
            {
                BOOL bSuccess = FALSE;
                CInstalledApplicationInfo *Info = new CInstalledApplicationInfo(RootKeyEnum[i] == HKEY_CURRENT_USER, RegSamEnum[i], hSubKey);
                Info->szKeyName = szKeyName;

                // check for failure. if failed to init, Info->hSubKey will be set to NULL
                if (Info->hSubKey)
                {
                    // those items without display name are ignored
                    if (Info->GetApplicationRegString(L"DisplayName", Info->szDisplayName))
                    {
                        Info->GetApplicationRegString(L"DisplayIcon", Info->szDisplayIcon);
                        Info->GetApplicationRegString(L"DisplayVersion", Info->szDisplayVersion);
                        Info->GetApplicationRegString(L"Publisher", Info->szPublisher);
                        Info->GetApplicationRegString(L"RegOwner", Info->szRegOwner);
                        Info->GetApplicationRegString(L"ProductID", Info->szProductID);
                        Info->GetApplicationRegString(L"HelpLink", Info->szHelpLink);
                        Info->GetApplicationRegString(L"HelpTelephone", Info->szHelpTelephone);
                        Info->GetApplicationRegString(L"Readme", Info->szReadme);
                        Info->GetApplicationRegString(L"Contact", Info->szContact);
                        Info->GetApplicationRegString(L"URLUpdateInfo", Info->szURLUpdateInfo);
                        Info->GetApplicationRegString(L"URLInfoAbout", Info->szURLInfoAbout);
                        Info->GetApplicationRegString(L"Comments", Info->szComments);
                        if (Info->GetApplicationRegString(L"InstallDate", Info->szInstallDate) == FALSE)
                        {
                            // It might be a DWORD (Unix timestamp). try again.
                            DWORD dwInstallTimeStamp;
                            if (Info->GetApplicationRegDword(L"InstallDate", &dwInstallTimeStamp))
                            {
                                FILETIME InstallFileTime;
                                SYSTEMTIME InstallSystemTime, InstallLocalTime;

                                UnixTimeToFileTime(dwInstallTimeStamp, &InstallFileTime);
                                FileTimeToSystemTime(&InstallFileTime, &InstallSystemTime);

                                // convert to localtime
                                SystemTimeToTzSpecificLocalTime(NULL, &InstallSystemTime, &InstallLocalTime);

                                // convert to readable date string
                                int cchTimeStrLen = GetDateFormatW(LOCALE_USER_DEFAULT,
                                    0,
                                    &InstallLocalTime,
                                    NULL, 0, 0);

                                GetDateFormatW(LOCALE_USER_DEFAULT, // use default locale for current user
                                    0,
                                    &InstallLocalTime,
                                    NULL, Info->szInstallDate.GetBuffer(cchTimeStrLen), cchTimeStrLen);
                                Info->szInstallDate.ReleaseBuffer();
                            }
                        }
                        Info->GetApplicationRegString(L"InstallLocation", Info->szInstallLocation);
                        Info->GetApplicationRegString(L"InstallSource", Info->szInstallSource);
                        Info->GetApplicationRegString(L"UninstallString", Info->szUninstallString);
                        Info->GetApplicationRegString(L"ModifyPath", Info->szModifyPath);

                        bSuccess = TRUE;
                    }
                }

                // close handle
                if (Info->hSubKey)
                {
                    CloseHandle(Info->hSubKey);
                    Info->hSubKey = NULL;
                }

                if (bSuccess)
                {
                    // add to InfoList.
                    m_InfoList.AddTail(Info);

                    // invoke callback
                    if (lpEnumProc)
                    {
                        if ((EnumType == ENUM_ALL_INSTALLED) || /* All components */
                            ((EnumType == ENUM_INSTALLED_APPLICATIONS) && (!Info->bIsUpdate)) || /* Applications only */
                            ((EnumType == ENUM_UPDATES) && (Info->bIsUpdate))) /* Updates only */
                        {
                            lpEnumProc(Info, param);
                        }
                    }
                }
                else
                {
                    // destory object
                    delete Info;
                }
            }
        }

        szKeyName.ReleaseBuffer();
        RegCloseKey(hKey);
    }

    return TRUE;
}

VOID CInstalledApps::FreeCachedEntries()
{
    POSITION InfoListPosition = m_InfoList.GetHeadPosition();

    /* loop and deallocate all the cached app infos in the list */
    while (InfoListPosition)
    {
        CInstalledApplicationInfo *Info = m_InfoList.GetNext(InfoListPosition);
        delete Info;
    }

    m_InfoList.RemoveAll();
}
