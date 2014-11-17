/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/available.c
 * PURPOSE:         Functions for working with availabled applications
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

BOOL
ShowAvailableAppInfo(INT Index)
{
    PAPPLICATION_INFO Info = (PAPPLICATION_INFO) ListViewGetlParam(Index);
    WCHAR szText[MAX_STR_LEN];

    if (!Info) return FALSE;

    NewRichEditText(Info->szName, CFE_BOLD);

    InsertRichEditText(L"\n", 0);

#define ADD_TEXT(a, b, c, d) \
    if (b[0] != '\0') \
    { \
        LoadStringW(hInst, a, szText, sizeof(szText) / sizeof(WCHAR)); \
        InsertRichEditText(szText, c); \
        InsertRichEditText(b, d); \
    } \

    ADD_TEXT(IDS_AINFO_VERSION, Info->szVersion, CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_LICENSE, Info->szLicense, CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_SIZE, Info->szSize, CFE_BOLD, 0);
    ADD_TEXT(IDS_AINFO_URLSITE, Info->szUrlSite, CFE_BOLD, CFE_LINK);
    ADD_TEXT(IDS_AINFO_DESCRIPTION, Info->szDesc, CFE_BOLD, 0);

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

    if (!GetStorageDirectory(szPath, sizeof(szPath) / sizeof(szPath[0])))
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

    DownloadApplicationsDB(APPLICATION_DATEBASE_URL);

    if (!GetStorageDirectory(szPath, sizeof(szPath) / sizeof(szPath[0])))
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
    WCHAR szSectionLocale[MAX_PATH] = L"Section.";
    WCHAR szCabPath[MAX_PATH];
    WCHAR szLocale[4 + 1];
    APPLICATION_INFO Info;
    HRESULT hr;

    if (!GetStorageDirectory(szPath, sizeof(szPath) / sizeof(szPath[0])))
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
            DownloadApplicationsDB(APPLICATION_DATEBASE_URL);

        ExtractFilesFromCab(szCabPath, szAppsPath);
        hFind = FindFirstFileW(szPath, &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    if (!GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_ILANGUAGE,
                        szLocale, sizeof(szLocale) / sizeof(WCHAR)))
    {
        FindClose(hFind);
        return FALSE;
    }

    hr = StringCbCatW(szSectionLocale, sizeof(szSectionLocale), szLocale);
    if (FAILED(hr))
    {
        FindClose(hFind);
        return FALSE;
    }

#define GET_STRING1(a, b)  \
    if (!ParserGetString(szSectionLocale, a, b, MAX_PATH, FindFileData.cFileName)) \
        if (!ParserGetString(L"Section", a, b, MAX_PATH, FindFileData.cFileName)) \
            continue;

#define GET_STRING2(a, b)  \
    if (!ParserGetString(szSectionLocale, a, b, MAX_PATH, FindFileData.cFileName)) \
        if (!ParserGetString(L"Section", a, b, MAX_PATH, FindFileData.cFileName)) \
            b[0] = '\0';

    do
    {
        Info.Category = ParserGetInt(szSectionLocale, L"Category", FindFileData.cFileName);
        if (Info.Category == -1)
        {
            Info.Category = ParserGetInt(L"Section", L"Category", FindFileData.cFileName);
            if (Info.Category == -1)
                continue;
        }

        if (EnumType != Info.Category && EnumType != ENUM_ALL_AVAILABLE) continue;

        GET_STRING1(L"Name", Info.szName);
        GET_STRING1(L"URLDownload", Info.szUrlDownload);

        GET_STRING2(L"RegName", Info.szRegName);
        GET_STRING2(L"Version", Info.szVersion);
        GET_STRING2(L"License", Info.szLicense);
        GET_STRING2(L"Description", Info.szDesc);
        GET_STRING2(L"Size", Info.szSize);
        GET_STRING2(L"URLSite", Info.szUrlSite);
        GET_STRING2(L"CDPath", Info.szCDPath);

        if (!lpEnumProc(&Info)) break;
    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return TRUE;
}
