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
static REGSAM g_RegSamEnum[3] = {0, KEY_WOW64_32KEY, KEY_WOW64_64KEY};
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

CAvailableApplicationInfo *
CAppDB::FindAvailableByPackageName(const CStringW &name)
{
    POSITION CurrentListPosition = m_Available.GetHeadPosition();
    while (CurrentListPosition)
    {
        CAppInfo *Info = m_Available.GetNext(CurrentListPosition);
        if (Info->szIdentifier == name)
        {
            return static_cast<CAvailableApplicationInfo *>(Info);
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
    AppsPath += RAPPS_DATABASE_SUBDIR;
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
            CConfigParser *Parser = new CConfigParser(CPathW(AppsPath) += FindFileData.cFileName);
            int Cat;
            if (!Parser->GetInt(DB_CATEGORY, Cat))
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
    AppsPath += RAPPS_DATABASE_SUBDIR;
    if (!ExtractFilesFromCab(APPLICATION_DATABASE_NAME, m_BasePath, AppsPath))
        return;

    CPathW CabFile = m_BasePath;
    CabFile += APPLICATION_DATABASE_NAME;
    DeleteFileW(CabFile);

    EnumerateFiles();
}

static inline HKEY
GetRootKeyInfo(UINT Index, REGSAM &RegSam)
{
    C_ASSERT(_countof(g_RootKeyEnum) == _countof(g_RegSamEnum));
    if (Index < _countof(g_RootKeyEnum))
    {
        RegSam = g_RegSamEnum[Index];
        return g_RootKeyEnum[Index];
    }
    return NULL;
}

HKEY
CAppDB::EnumInstalledRootKey(UINT Index, REGSAM &RegSam)
{
    // Loop for through all combinations.
    // Note that HKEY_CURRENT_USER\Software does not have a redirect
    // https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys#redirected-shared-and-reflected-keys-under-wow64
    if (Index < (IsSystem64Bit() ? 3 : 2))
        return GetRootKeyInfo(Index, RegSam);
    else
        return NULL;
}

CInstalledApplicationInfo *
CAppDB::CreateInstalledAppByRegistryKey(LPCWSTR KeyName, HKEY hKeyParent, UINT KeyIndex)
{
    CRegKey hSubKey;
    if (hSubKey.Open(hKeyParent, KeyName, KEY_READ) != ERROR_SUCCESS)
        return NULL;
    DWORD value, size;

    size = sizeof(DWORD);
    if (!RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&value, &size) && value == 1)
    {
        // Ignore system components
        return NULL;
    }

    size = 0;
    BOOL bIsUpdate = !RegQueryValueExW(hSubKey, L"ParentKeyName", NULL, NULL, NULL, &size);

    AppsCategories cat = bIsUpdate ? ENUM_UPDATES : ENUM_INSTALLED_APPLICATIONS;
    CInstalledApplicationInfo *pInfo;
    pInfo = new CInstalledApplicationInfo(hSubKey.Detach(), KeyName, cat, KeyIndex);
    if (pInfo && pInfo->Valid())
    {
        return pInfo;
    }
    delete pInfo;
    return NULL;
}

CInstalledApplicationInfo *
CAppDB::EnumerateRegistry(CAtlList<CAppInfo *> *List, LPCWSTR SearchOnly)
{
    ATLASSERT(List || SearchOnly);
    REGSAM wowsam;
    HKEY hRootKey;
    for (UINT rki = 0; (hRootKey = EnumInstalledRootKey(rki, wowsam)); ++rki)
    {
        CRegKey hKey;
        if (hKey.Open(hRootKey, UNINSTALL_SUBKEY, KEY_READ | wowsam) != ERROR_SUCCESS)
        {
            continue;
        }
        for (DWORD Index = 0;; ++Index)
        {
            WCHAR szKeyName[MAX_PATH];
            DWORD dwSize = _countof(szKeyName);
            if (hKey.EnumKey(Index, szKeyName, &dwSize) != ERROR_SUCCESS)
            {
                break;
            }
            if (List || !StrCmpIW(SearchOnly, szKeyName))
            {
                CInstalledApplicationInfo *Info;
                Info = CreateInstalledAppByRegistryKey(szKeyName, hKey, rki);
                if (Info)
                {
                    if (List)
                        List->AddTail(Info);
                    else
                        return Info;
                }
            }
        }
    }
    return NULL;
}

VOID
CAppDB::UpdateInstalled()
{
    // Remove all old entries
    ClearList(m_Installed);

    EnumerateRegistry(&m_Installed, NULL);
}

CInstalledApplicationInfo *
CAppDB::CreateInstalledAppByRegistryKey(LPCWSTR Name)
{
    return EnumerateRegistry(NULL, Name);
}

CInstalledApplicationInfo *
CAppDB::CreateInstalledAppInstance(LPCWSTR KeyName, BOOL User, REGSAM WowSam)
{
    HKEY hRootKey = User ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    UINT KeyIndex = User ? (0) : ((WowSam & KEY_WOW64_64KEY) ? 2 : 1);
    CRegKey hKey;
    if (hKey.Open(hRootKey, UNINSTALL_SUBKEY, KEY_READ | WowSam) == ERROR_SUCCESS)
    {
        return CreateInstalledAppByRegistryKey(KeyName, hKey, KeyIndex);
    }
    return NULL;
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
    AppsPath += RAPPS_DATABASE_SUBDIR;
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

DWORD
CAppDB::RemoveInstalledAppFromRegistry(const CAppInfo *Info)
{
    // Validate that this is actually an installed app / update
    ATLASSERT(Info->iCategory == ENUM_INSTALLED_APPLICATIONS || Info->iCategory == ENUM_UPDATES);
    if (Info->iCategory != ENUM_INSTALLED_APPLICATIONS && Info->iCategory != ENUM_UPDATES)
        return ERROR_INVALID_PARAMETER;

    const CInstalledApplicationInfo *InstalledInfo = static_cast<const CInstalledApplicationInfo *>(Info);

    CStringW Name = InstalledInfo->szIdentifier;
    REGSAM wowsam;
    HKEY hRoot = GetRootKeyInfo(InstalledInfo->m_KeyInfo, wowsam);
    ATLASSERT(hRoot);
    if (!hRoot)
        return ERROR_OPEN_FAILED;

    CRegKey Uninstall;
    LSTATUS err = Uninstall.Open(hRoot, UNINSTALL_SUBKEY, KEY_READ | KEY_WRITE | wowsam);
    if (err == ERROR_SUCCESS)
    {
        err = Uninstall.RecurseDeleteKey(Name);
    }
    return err;
}
