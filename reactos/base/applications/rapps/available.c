/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/available.c
 * PURPOSE:         Functions for working with availabled applications
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#include "rapps.h"

#define ADD_TEXT(a, b, c, d) \
    if (b[0] != '\0') \
    { \
        LoadStringW(hInst, a, szText, _countof(szText)); \
        InsertRichEditText(szText, c); \
        InsertRichEditText(b, d); \
    } \

#define GET_STRING1(a, b)  \
    if (!ParserGetString(a, b, MAX_PATH, FindFileData.cFileName)) \
        continue;

#define GET_STRING2(a, b)  \
    if (!ParserGetString(a, b, MAX_PATH, FindFileData.cFileName)) \
        b[0] = '\0';

LIST_ENTRY CachedEntriesHead = {0};
PLIST_ENTRY pCachedEntry = NULL;

BOOL
ShowAvailableAppInfo(INT Index)
{
    PAPPLICATION_INFO Info = (PAPPLICATION_INFO) ListViewGetlParam(Index);
    WCHAR szText[MAX_STR_LEN];

    if (!Info) return FALSE;

    NewRichEditText(Info->szName, CFE_BOLD);

    InsertRichEditText(L"\n", 0);

    ADD_TEXT(IDS_AINFO_VERSION,     Info->szVersion, CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_LICENSE,     Info->szLicense, CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_SIZE,        Info->szSize,    CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_URLSITE,     Info->szUrlSite, CFE_BOLD, CFE_LINK);
    ADD_TEXT(IDS_AINFO_DESCRIPTION, Info->szDesc,    CFE_BOLD, 0);

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

    /* initialize the cached list if hasn't been yet */
    if (pCachedEntry == NULL)
    {
        InitializeListHead(&CachedEntriesHead);
        pCachedEntry = &CachedEntriesHead;
    }

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
        Info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APPLICATION_INFO));

        if(!Info)
            break;

        Info->Category = ParserGetInt(L"Category", FindFileData.cFileName);

        /* copy the cache-related fields for the next time */
        RtlCopyMemory(&Info->cFileName,    &FindFileData.cFileName, MAX_PATH);
        RtlCopyMemory(&Info->ftCacheStamp, &FindFileData.ftLastWriteTime, sizeof(FILETIME));

        /* add our cached entry to the cached list */
        InsertTailList(&CachedEntriesHead, &Info->List);

skip_if_cached:

        if (Info->Category == -1)
            continue;

        if (EnumType != Info->Category && EnumType != ENUM_ALL_AVAILABLE)
            continue;

        /* if our cache hit was only partial, we need to parse
           and lazily fill the rest of fields only when needed */

        if (Info->szUrlDownload[0] == 0)
        {
            GET_STRING1(L"Name",        Info->szName);
            GET_STRING1(L"URLDownload", Info->szUrlDownload);

            GET_STRING2(L"RegName",     Info->szRegName);
            GET_STRING2(L"Version",     Info->szVersion);
            GET_STRING2(L"License",     Info->szLicense);
            GET_STRING2(L"Description", Info->szDesc);
            GET_STRING2(L"Size",        Info->szSize);
            GET_STRING2(L"URLSite",     Info->szUrlSite);
            GET_STRING2(L"CDPath",      Info->szCDPath);
        }

        if (!lpEnumProc(Info))
            break;

    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return TRUE;
}