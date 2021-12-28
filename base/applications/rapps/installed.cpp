/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with installed applications
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev <dmitry@reactos.org>
 *              Copyright 2017 Alexander Shaposhnikov <sanchaez@reactos.org>
 *              Copyright 2020 He Yang <1160386205@qq.com>
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "rapps.h"
#include "installed.h"
#include "misc.h"

CInstalledApplicationInfo::CInstalledApplicationInfo(BOOL bIsUserKey, REGSAM RegWowKey, HKEY hKey, const CStringW& szKeyName)
    : m_IsUserKey(bIsUserKey)
    , m_WowKey(RegWowKey)
    , m_hSubKey(hKey)
    , m_szKeyName(szKeyName)
{
    DWORD dwSize = 0;
    bIsUpdate = (RegQueryValueExW(m_hSubKey, L"ParentKeyName", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS);
}

CInstalledApplicationInfo::~CInstalledApplicationInfo()
{
    if (m_hSubKey)
    {
        CloseHandle(m_hSubKey);
        m_hSubKey = NULL;
    }
}

void CInstalledApplicationInfo::EnsureDetailsLoaded()
{
    // Key not closed, so we have not loaded details yet
    if (m_hSubKey)
    {
        GetApplicationRegString(L"Publisher", szPublisher);
        GetApplicationRegString(L"RegOwner", szRegOwner);
        GetApplicationRegString(L"ProductID", szProductID);
        GetApplicationRegString(L"HelpLink", szHelpLink);
        GetApplicationRegString(L"HelpTelephone", szHelpTelephone);
        GetApplicationRegString(L"Readme", szReadme);
        GetApplicationRegString(L"Contact", szContact);
        GetApplicationRegString(L"URLUpdateInfo", szURLUpdateInfo);
        GetApplicationRegString(L"URLInfoAbout", szURLInfoAbout);
        if (GetApplicationRegString(L"InstallDate", szInstallDate) == FALSE)
        {
            // It might be a DWORD (Unix timestamp). try again.
            DWORD dwInstallTimeStamp;
            if (GetApplicationRegDword(L"InstallDate", &dwInstallTimeStamp))
            {
                FILETIME InstallFileTime;
                SYSTEMTIME InstallSystemTime, InstallLocalTime;

                UnixTimeToFileTime(dwInstallTimeStamp, &InstallFileTime);
                FileTimeToSystemTime(&InstallFileTime, &InstallSystemTime);

                // convert to localtime
                SystemTimeToTzSpecificLocalTime(NULL, &InstallSystemTime, &InstallLocalTime);

                // convert to readable date string
                int cchTimeStrLen = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &InstallLocalTime, NULL, 0, 0);

                GetDateFormatW(
                    LOCALE_USER_DEFAULT, // use default locale for current user
                    0, &InstallLocalTime, NULL, szInstallDate.GetBuffer(cchTimeStrLen), cchTimeStrLen);
                szInstallDate.ReleaseBuffer();
            }
        }
        GetApplicationRegString(L"InstallLocation", szInstallLocation);
        GetApplicationRegString(L"InstallSource", szInstallSource);
        GetApplicationRegString(L"UninstallString", szUninstallString);
        GetApplicationRegString(L"ModifyPath",szModifyPath);

        CloseHandle(m_hSubKey);
        m_hSubKey = NULL;
    }
}

BOOL CInstalledApplicationInfo::GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String)
{
    DWORD dwAllocated = 0, dwSize, dwType;

    // retrieve the size of value first.
    if (RegQueryValueExW(m_hSubKey, lpKeyName, NULL, &dwType, NULL, &dwAllocated) != ERROR_SUCCESS ||
        dwType != REG_SZ)
    {
        String.Empty();
        return FALSE;
    }

    // query the value
    dwSize = dwAllocated;
    LSTATUS Result =
        RegQueryValueExW(m_hSubKey, lpKeyName, NULL, NULL, (LPBYTE)String.GetBuffer(dwAllocated / sizeof(WCHAR)), &dwSize);

    dwSize = min(dwAllocated, dwSize);
    // CString takes care of zero-terminating it
    String.ReleaseBuffer(dwSize / sizeof(WCHAR));

    if (Result != ERROR_SUCCESS)
    {
        String.Empty();
        return FALSE;
    }

    return TRUE;
}

BOOL CInstalledApplicationInfo::GetApplicationRegDword(LPCWSTR lpKeyName, DWORD *lpValue)
{
    DWORD dwSize = sizeof(DWORD), dwType;
    if (RegQueryValueExW(m_hSubKey,
        lpKeyName,
        NULL,
        &dwType,
        (LPBYTE)lpValue,
        &dwSize) != ERROR_SUCCESS || dwType != REG_DWORD)
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
    ATL::CStringW szFullName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + m_szKeyName;
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
            return pRegDeleteKeyExW(m_IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szFullName, m_WowKey, 0);
        }
    }

    /* Otherwise, return non-Ex function */
    return RegDeleteKeyW(m_IsUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, szFullName);
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
            if (RegOpenKeyW(hKey, szKeyName, &hSubKey) == ERROR_SUCCESS)
            {
                DWORD dwValue = 0;
                BOOL bIsSystemComponent = FALSE;

                dwSize = sizeof(DWORD);
                if (RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
                {
                    bIsSystemComponent = (dwValue == 0x1);
                }
                // Ignore system components
                if (bIsSystemComponent)
                {
                    RegCloseKey(hSubKey);
                    continue;
                }

                BOOL bSuccess = FALSE;
                CInstalledApplicationInfo *Info = new CInstalledApplicationInfo(RootKeyEnum[i] == HKEY_CURRENT_USER, RegSamEnum[i], hSubKey, szKeyName);

                // items without display name are ignored
                if (Info->GetApplicationRegString(L"DisplayName", Info->szDisplayName))
                {
                    Info->GetApplicationRegString(L"DisplayIcon", Info->szDisplayIcon);
                    Info->GetApplicationRegString(L"DisplayVersion", Info->szDisplayVersion);
                    Info->GetApplicationRegString(L"Comments", Info->szComments);

                    bSuccess = TRUE;
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
