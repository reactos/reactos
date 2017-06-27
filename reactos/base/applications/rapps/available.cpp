/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/available.cpp
 * PURPOSE:         Functions for working with available applications
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#include "rapps.h"

inline void _AddText(UINT a, LPCWSTR b, DWORD c, DWORD d) {
  if (b[0] != '\0') 
  {
      WCHAR szText[MAX_STR_LEN];
      LoadStringW(hInst, a, szText, _countof(szText));
      InsertRichEditText(szText, c); 
      InsertRichEditText(b, d); 
  }
}

inline void _AddTextNewl(UINT a, DWORD b) {
    WCHAR szText[MAX_STR_LEN];
    LoadStringW(hInst, a, szText, _countof(szText));
    InsertRichEditText(L"\n", 0);
    InsertRichEditText(szText, b);
    InsertRichEditText(L"\n", 0);
}

template<typename T, size_t N, size_t N2>
inline BOOL _GetString(LPCWSTR a, T(&b)[N], T (&cFileName)[N2]) {
  return ParserGetString(a, b, N, cFileName);
}

template<typename T, size_t N, size_t N2>
inline void _GetStringNullFailure(LPCWSTR a, T(&b)[N], T (&cFileName)[N2]) {
  if (!_GetString(a, b, cFileName)) {
    b[0] = '\0';
  }
}

//App is "installed" if the RegName or Name is in the registry
inline BOOL _AppInstallCheckWithKey(PAPPLICATION_INFO Info, REGSAM key)
{
    return (*Info->szRegName
        && (IsInstalledApplication(Info->szRegName, TRUE, key)
        || IsInstalledApplication(Info->szRegName, FALSE, key)))
        || (*Info->szName && (IsInstalledApplication(Info->szName, TRUE, key)
            || IsInstalledApplication(Info->szName, FALSE, key)));
}


//Check both registry keys in 64bit system
//TODO: check system type beforehand to avoid double checks?
inline BOOL _AppInstallCheck(PAPPLICATION_INFO Info) {
  return  _AppInstallCheckWithKey(Info, KEY_WOW64_32KEY) 
    || _AppInstallCheckWithKey(Info, KEY_WOW64_64KEY);
}

//App is "installed" if the RegName or Name is in the registry
inline BOOL _GetInstalledVersionWithKey(PAPPLICATION_INFO Info, LPWSTR szVersion, UINT iVersionSize, REGSAM key)
{
    return (*Info->szRegName
        && (InstalledVersion(szVersion, iVersionSize, Info->szRegName, TRUE, key)
        || InstalledVersion(szVersion, iVersionSize, Info->szRegName, FALSE, key)))
        || (*Info->szName && (InstalledVersion(szVersion, iVersionSize, Info->szName, TRUE, key)
            || InstalledVersion(szVersion, iVersionSize, Info->szName, FALSE, key)));
}

//App is "installed" if the RegName or Name is in the registry
inline BOOL _GetInstalledVersion(PAPPLICATION_INFO Info, LPWSTR szVersion, UINT iVersionSize)
{
    return  _GetInstalledVersionWithKey(Info, szVersion, iVersionSize, KEY_WOW64_32KEY)
        || _GetInstalledVersionWithKey(Info, szVersion, iVersionSize, KEY_WOW64_64KEY);
}

LIST_ENTRY CachedEntriesHead = { &CachedEntriesHead, &CachedEntriesHead };
PLIST_ENTRY pCachedEntry = &CachedEntriesHead;

BOOL
ShowAvailableAppInfo(INT Index)
{
    PAPPLICATION_INFO Info = (PAPPLICATION_INFO) ListViewGetlParam(Index);
    BOOL bIsInstalled = _AppInstallCheck(Info);
    WCHAR szVersion[MAX_PATH];

    if (!Info) return FALSE;

    NewRichEditText(Info->szName, CFE_BOLD);
    if (bIsInstalled)
    {
      _AddTextNewl(IDS_STATUS_INSTALLED, CFE_ITALIC);
      if (_GetInstalledVersion(Info, szVersion, _countof(szVersion)))
      {
          _AddText(IDS_AINFO_VERSION, szVersion, CFE_BOLD, 0);
      }
    } else 
    {
      _AddTextNewl(IDS_STATUS_NOTINSTALLED, CFE_ITALIC);
    }

    _AddText(IDS_AINFO_AVAILABLEVERSION, Info->szVersion, CFE_BOLD, 0);
    _AddText(IDS_AINFO_LICENSE, Info->szLicense, CFE_BOLD, 0);
    _AddText(IDS_AINFO_SIZE, Info->szSize, CFE_BOLD, 0);
    _AddText(IDS_AINFO_URLSITE, Info->szUrlSite, CFE_BOLD, CFE_LINK);
    _AddText(IDS_AINFO_DESCRIPTION, Info->szDesc, CFE_BOLD, 0);
    _AddText(IDS_AINFO_URLDOWNLOAD, Info->szUrlDownload, CFE_BOLD, CFE_LINK);

    return TRUE;
}

static BOOL
DeleteCurrentAppsDB(VOID)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;
    WCHAR szCabPath[MAX_PATH];
    WCHAR szSearchPath[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    WCHAR szTmp[MAX_PATH];
    HRESULT hr;
    BOOL result = TRUE;

    if (!GetStorageDirectory(szPath, _countof(szPath)))
        return FALSE;

    hr = StringCbPrintfW(szCabPath, sizeof(szCabPath),
                         L"%ls\\rappmgr.cab",
                         szPath);
    if (FAILED(hr))
        return FALSE;

    result = result && DeleteFileW(szCabPath);

    hr = StringCbCatW(szPath, sizeof(szPath), L"\\rapps\\");

    if (FAILED(hr))
        return FALSE;

    hr = StringCbPrintfW(szSearchPath, sizeof(szSearchPath),
                         L"%ls*.txt",
                         szPath);
    if (FAILED(hr))
        return FALSE;

    hFind = FindFirstFileW(szSearchPath, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return result;

    do
    {
        hr = StringCbPrintfW(szTmp, sizeof(szTmp),
                             L"%ls%ls",
                             szPath, FindFileData.cFileName);
        if (FAILED(hr))
            continue;
        result = result && DeleteFileW(szTmp);

    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return result;
}


BOOL
UpdateAppsDB(VOID)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szAppsPath[MAX_PATH];
    WCHAR szCabPath[MAX_PATH];

    if (!DeleteCurrentAppsDB())
        return FALSE;

    DownloadApplicationsDB(APPLICATION_DATABASE_URL);

    if (!GetStorageDirectory(szPath, _countof(szPath)))
        return FALSE;

    if (FAILED(StringCbPrintfW(szCabPath, sizeof(szCabPath),
                               L"%ls\\rappmgr.cab",
                               szPath)))
    {
        return FALSE;
    }

    if (FAILED(StringCbPrintfW(szAppsPath, sizeof(szAppsPath),
                               L"%ls\\rapps\\",
                               szPath)))
    {
        return FALSE;
    }

    ExtractFilesFromCab(szCabPath, szAppsPath);

    return TRUE;
}


BOOL
EnumAvailableApplications(INT EnumType, AVAILENUMPROC lpEnumProc)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;
    WCHAR szPath[MAX_PATH];
    WCHAR szAppsPath[MAX_PATH];
    WCHAR szCabPath[MAX_PATH];
    PAPPLICATION_INFO Info;
    HRESULT hr;

    if (!GetStorageDirectory(szPath, _countof(szPath)))
        return FALSE;

    hr = StringCbPrintfW(szCabPath, sizeof(szCabPath),
                         L"%ls\\rappmgr.cab",
                         szPath);
    if (FAILED(hr))
        return FALSE;

    hr = StringCbCatW(szPath, sizeof(szPath), L"\\rapps\\");

    if (FAILED(hr))
        return FALSE;

    hr = StringCbCopyW(szAppsPath, sizeof(szAppsPath), szPath);

    if (FAILED(hr))
        return FALSE;

    if (!CreateDirectory(szPath, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return FALSE;
    }

    hr = StringCbCatW(szPath, sizeof(szPath), L"*.txt");

    if (FAILED(hr))
        return FALSE;

    hFind = FindFirstFileW(szPath, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (GetFileAttributesW(szCabPath) == INVALID_FILE_ATTRIBUTES)
            DownloadApplicationsDB(APPLICATION_DATABASE_URL);

        ExtractFilesFromCab(szCabPath, szAppsPath);
        hFind = FindFirstFileW(szPath, &FindFileData);

        if (hFind == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    do
    {
        /* loop for all the cached entries */
        for (pCachedEntry = CachedEntriesHead.Flink; pCachedEntry != &CachedEntriesHead; pCachedEntry = pCachedEntry->Flink)
        {
            Info = CONTAINING_RECORD(pCachedEntry, APPLICATION_INFO, List);

            /* do we already have this entry in cache? */
            if(_wcsicmp(FindFileData.cFileName, Info->cFileName) == 0)
            {
                /* is it current enough, or the file has been modified since our last time here? */
                if (CompareFileTime(&FindFileData.ftLastWriteTime, &Info->ftCacheStamp) == 1)
                {
                    /* recreate our cache, this is the slow path */
                    RemoveEntryList(&Info->List);
                    HeapFree(GetProcessHeap(), 0, Info);
                }
                else
                {
                    /* speedy path, compare directly, we already have the data */
                    goto skip_if_cached;
                }

                break;
            }
        }

        /* create a new entry */
        Info = (PAPPLICATION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APPLICATION_INFO));

        if(!Info)
            break;

        Info->Category = ParserGetInt(L"Category", FindFileData.cFileName);

        /* copy the cache-related fields for the next time */
        RtlCopyMemory(&Info->cFileName,    &FindFileData.cFileName, MAX_PATH);
        RtlCopyMemory(&Info->ftCacheStamp, &FindFileData.ftLastWriteTime, sizeof(FILETIME));

        /* add our cached entry to the cached list */
        InsertTailList(&CachedEntriesHead, &Info->List);

skip_if_cached:

        if (Info->Category == FALSE)
            continue;

        if (EnumType != Info->Category && EnumType != ENUM_ALL_AVAILABLE)
            continue;

        /* if our cache hit was only partial, we need to parse
           and lazily fill the rest of fields only when needed */

        if (Info->szUrlDownload[0] == 0)
        {
          if (!_GetString(L"Name", Info->szName, FindFileData.cFileName)
            || !_GetString(L"URLDownload", Info->szUrlDownload, FindFileData.cFileName))
          {
            continue;
          }

          _GetStringNullFailure(L"RegName",     Info->szRegName, FindFileData.cFileName);
          _GetStringNullFailure(L"Version",     Info->szVersion, FindFileData.cFileName);
          _GetStringNullFailure(L"License",     Info->szLicense, FindFileData.cFileName);
          _GetStringNullFailure(L"Description", Info->szDesc, FindFileData.cFileName);
          _GetStringNullFailure(L"Size",        Info->szSize, FindFileData.cFileName);
          _GetStringNullFailure(L"URLSite",     Info->szUrlSite, FindFileData.cFileName);
          _GetStringNullFailure(L"CDPath",      Info->szCDPath, FindFileData.cFileName);
          _GetStringNullFailure(L"SHA1",        Info->szSHA1, FindFileData.cFileName);
        }

        if (!lpEnumProc(Info))
            break;

    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return TRUE;
}

VOID FreeCachedAvailableEntries(VOID)
{
     PAPPLICATION_INFO Info;
 
    /* loop and deallocate all the cached app infos in the list */
    for (pCachedEntry = CachedEntriesHead.Flink; pCachedEntry != &CachedEntriesHead;)
    {
         Info = CONTAINING_RECORD(pCachedEntry, APPLICATION_INFO, List);
 
        /* grab a reference to the next linked entry before getting rid of the current one */
        pCachedEntry = pCachedEntry->Flink;
 
        /* flush them down the toilet :D */
        RemoveEntryList(&Info->List);
        HeapFree(GetProcessHeap(), 0, Info);
    }
}