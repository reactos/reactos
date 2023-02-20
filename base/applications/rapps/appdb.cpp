/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with available applications
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2020 He Yang (1160386205@qq.com)
 *              Copyright 2021-2023 Mark Jansen <mark.jansen@reactos.org>
 */

#include "rapps.h"
#include "appdb.h"
#include "configparser.h"
#include "settings.h"


static HKEY g_RootKeyEnum[3] = {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_LOCAL_MACHINE};
static REGSAM g_RegSamEnum[3] = {KEY_WOW64_32KEY, KEY_WOW64_32KEY, KEY_WOW64_64KEY};
#define UNINSTALL_SUBKEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

static VOID
ClearList(CAtlList<CAppInfo *> &list)
{
    POSITION InfoListPosition = list.GetHeadPosition();
    while (InfoListPosition)
    {
        CAppInfo *Info = list.GetNext(InfoListPosition);
        delete Info;
    }
    list.RemoveAll();
}

CAppDB::CAppDB(const CStringW &path) : m_BasePath(path)
{
    m_BasePath.Canonicalize();
}

CAppInfo *
CAppDB::FindByPackageName(const CStringW &name)
{
    POSITION CurrentListPosition = m_Available.GetHeadPosition();
    while (CurrentListPosition)
    {
        CAppInfo *Info = m_Available.GetNext(CurrentListPosition);
        if (Info->szIdentifier == name)
        {
            return Info;
        }
    }
    return NULL;
}

void
CAppDB::GetApps(CAtlList<CAppInfo *> &List, AppsCategories Type) const
{
    const BOOL UseInstalled = IsInstalledEnum(Type);
    const CAtlList<CAppInfo *> &list = UseInstalled ? m_Installed : m_Available;
    const BOOL IncludeAll = UseInstalled ? (Type == ENUM_ALL_INSTALLED) : (Type == ENUM_ALL_AVAILABLE);

    POSITION CurrentListPosition = list.GetHeadPosition();
    while (CurrentListPosition)
    {
        CAppInfo *Info = list.GetNext(CurrentListPosition);

        if (IncludeAll || Type == Info->iCategory)
        {
            List.AddTail(Info);
        }
    }
}

BOOL
CAppDB::EnumerateFiles()
{
    ClearList(m_Available);

    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    CPathW WildcardPath = AppsPath;
    WildcardPath += L"*.txt";

    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind = FindFirstFileW(WildcardPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    do
    {
        CStringW szPkgName = FindFileData.cFileName;
        PathRemoveExtensionW(szPkgName.GetBuffer(MAX_PATH));
        szPkgName.ReleaseBuffer();

        CAppInfo *Info = FindByPackageName(szPkgName);
        ATLASSERT(Info == NULL);
        if (!Info)
        {
            CConfigParser *Parser = new CConfigParser(FindFileData.cFileName);
            int Cat;
            if (!Parser->GetInt(L"Category", Cat))
                Cat = ENUM_INVALID;

            Info = new CAvailableApplicationInfo(Parser, szPkgName, static_cast<AppsCategories>(Cat), AppsPath);
            if (Info->Valid())
            {
                m_Available.AddTail(Info);
            }
            else
            {
                delete Info;
            }
        }

    } while (FindNextFileW(hFind, &FindFileData));

    FindClose(hFind);
    return TRUE;
}

VOID
CAppDB::UpdateAvailable()
{
    if (!CreateDirectoryW(m_BasePath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return;

    if (EnumerateFiles())
        return;

    DownloadApplicationsDB(
        SettingsInfo.bUseSource ? SettingsInfo.szSourceURL : APPLICATION_DATABASE_URL, !SettingsInfo.bUseSource);

    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    if (!ExtractFilesFromCab(APPLICATION_DATABASE_NAME, m_BasePath, AppsPath))
        return;

    CPathW CabFile = m_BasePath;
    CabFile += APPLICATION_DATABASE_NAME;
    DeleteFileW(CabFile);

    EnumerateFiles();
}

VOID
CAppDB::UpdateInstalled()
{
    // Remove all old entries
    ClearList(m_Installed);

    int LoopKeys = 2;

    if (IsSystem64Bit())
    {
        // loop for all 3 combination.
        // note that HKEY_CURRENT_USER\Software don't have a redirect
        // https://docs.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys#redirected-shared-and-reflected-keys-under-wow64
        LoopKeys = 3;
    }

    for (int keyIndex = 0; keyIndex < LoopKeys; keyIndex++)
    {
        LONG ItemIndex = 0;
        WCHAR szKeyName[MAX_PATH];

        CRegKey hKey;
        if (hKey.Open(g_RootKeyEnum[keyIndex], UNINSTALL_SUBKEY, KEY_READ | g_RegSamEnum[keyIndex]) != ERROR_SUCCESS)
        {
            continue;
        }

        while (1)
        {
            DWORD dwSize = _countof(szKeyName);
            if (hKey.EnumKey(ItemIndex, szKeyName, &dwSize) != ERROR_SUCCESS)
            {
                break;
            }

            ItemIndex++;

            CRegKey hSubKey;
            if (hSubKey.Open(hKey, szKeyName, KEY_READ) == ERROR_SUCCESS)
            {
                DWORD dwValue = 0;

                dwSize = sizeof(DWORD);
                if (RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&dwValue, &dwSize) ==
                        ERROR_SUCCESS &&
                    dwValue == 1)
                {
                    // Ignore system components
                    continue;
                }

                BOOL bIsUpdate =
                    (RegQueryValueExW(hSubKey, L"ParentKeyName", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS);

                CInstalledApplicationInfo *Info = new CInstalledApplicationInfo(
                    hSubKey.Detach(), szKeyName, bIsUpdate ? ENUM_UPDATES : ENUM_INSTALLED_APPLICATIONS, keyIndex);

                if (Info->Valid())
                {
                    m_Installed.AddTail(Info);
                }
                else
                {
                    delete Info;
                }
            }
        }
    }
}

static void
DeleteWithWildcard(const CPathW &Dir, const CStringW &Filter)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;

    CPathW DirWithFilter = Dir;
    DirWithFilter += Filter;

    hFind = FindFirstFileW(DirWithFilter, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        CPathW szTmp = Dir;
        szTmp += FindFileData.cFileName;

        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            DeleteFileW(szTmp);
        }
    } while (FindNextFileW(hFind, &FindFileData) != 0);
    FindClose(hFind);
}

VOID
CAppDB::RemoveCached()
{
    // Delete icons
    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    CPathW IconPath = AppsPath;
    IconPath += L"icons";
    DeleteWithWildcard(IconPath, L"*.ico");

    // Delete leftover screenshots
    CPathW ScrnshotFolder = AppsPath;
    ScrnshotFolder += L"screenshots";
    DeleteWithWildcard(ScrnshotFolder, L"*.tmp");

    // Delete data base files (*.txt)
    DeleteWithWildcard(AppsPath, L"*.txt");

    RemoveDirectoryW(IconPath);
    RemoveDirectoryW(ScrnshotFolder);
    RemoveDirectoryW(AppsPath);
    RemoveDirectoryW(m_BasePath);
}

BOOL
CAppDB::RemoveInstalledAppFromRegistry(const CAppInfo *Info)
{
    // Validate that this is actually an installed app / update
    ATLASSERT(Info->iCategory == ENUM_INSTALLED_APPLICATIONS || Info->iCategory == ENUM_UPDATES);
    if (Info->iCategory != ENUM_INSTALLED_APPLICATIONS && Info->iCategory != ENUM_UPDATES)
        return FALSE;

    // Grab the index in the registry keys
    const CInstalledApplicationInfo *InstalledInfo = static_cast<const CInstalledApplicationInfo *>(Info);
    ATLASSERT(InstalledInfo->iKeyIndex >= 0 && InstalledInfo->iKeyIndex < (int)_countof(g_RootKeyEnum));
    if (InstalledInfo->iKeyIndex < 0 && InstalledInfo->iKeyIndex >= (int)_countof(g_RootKeyEnum))
        return FALSE;

    int keyIndex = InstalledInfo->iKeyIndex;

    // Grab the registry key name
    CStringW Name = InstalledInfo->szIdentifier;

    // Recursively delete this key
    CRegKey Uninstall;
    if (Uninstall.Open(g_RootKeyEnum[keyIndex], UNINSTALL_SUBKEY, KEY_READ | KEY_WRITE | g_RegSamEnum[keyIndex]) !=
        ERROR_SUCCESS)
        return FALSE;

    return Uninstall.RecurseDeleteKey(Name) == ERROR_SUCCESS;
}
