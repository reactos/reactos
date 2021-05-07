/*
 * PROJECT:     ReactOS WHERE command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search executable files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <conutils.h>
#include "strlist.h" // strlist_...
#include "resource.h"

#define FLAG_HELP (1 << 0) // "/?"
#define FLAG_R (1 << 1) // recursive directory
#define FLAG_Q (1 << 2) // quiet mode
#define FLAG_F (1 << 3) // double quote
#define FLAG_T (1 << 4) // detailed info

static DWORD s_dwFlags = 0;
static LPWSTR s_pszRecursiveDir = NULL;
static strlist_t s_patterns = strlist_default;
static strlist_t s_results = strlist_default;
static strlist_t s_pathext = strlist_default;

// is it either "." or ".."?
#define IS_DOTS(pch) \
    (*(pch) == L'.' && ((pch)[1] == 0 || ((pch)[1] == L'.' && (pch)[2] == 0)))

#define DEFAULT_PATHEXT L".com;.exe;.bat;.cmd"

typedef enum WRET // return code of WHERE command
{
    WRET_SUCCESS = 0,
    WRET_NOT_FOUND = 1,
    WRET_ERROR = 2
} WRET;

static VOID WhereError(UINT nID)
{
    if (!(s_dwFlags & FLAG_Q)) // not quiet mode?
        ConResPuts(StdErr, nID);
}

typedef BOOL (CALLBACK *WHERE_SEARCH_FN)
    (LPCWSTR pattern, LPCWSTR path, WIN32_FIND_DATAW *finddata);

static BOOL
WhereSearchGeneric(LPCWSTR pattern, LPWSTR pszPath, BOOL bDir, WHERE_SEARCH_FN callback)
{
    LPWSTR pch;
    INT cch;
    BOOL ret = TRUE;
    HANDLE hFind;
    WIN32_FIND_DATAW find;

    hFind = FindFirstFileExW(pszPath, FindExInfoStandard, &find, FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
        return TRUE; // not found

    pch = wcsrchr(pszPath, L'\\') + 1;
    cch = MAX_PATH - (pch - pszPath);

    do
    {
        if (bDir != !!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (bDir && IS_DOTS(find.cFileName))
            continue;

        StringCchCopyW(pch, cch, find.cFileName); // build full path

        if (!callback(pattern, pszPath, &find))
        {
            WhereError(IDS_OUTOFMEMORY);
            ret = FALSE;
            break;
        }
    } while (FindNextFile(hFind, &find));
    FindClose(hFind);

    //--pch;
    //*pch = 0;
    return ret;
}

static BOOL CALLBACK WherePrintPath(LPCWSTR pattern, LPCWSTR path, WIN32_FIND_DATAW *finddata)
{
    WCHAR szPath[MAX_PATH], szDate[32], szTime[32];
    LARGE_INTEGER FileSize;
    FILETIME ftLocal;
    SYSTEMTIME st;
    INT iPath;

    iPath = strlist_find_i(&s_results, path);
    if (iPath >= 0)
        return TRUE; // already exists
    if (!strlist_add(&s_results, str_clone(path)))
        return FALSE;
    if (s_dwFlags & FLAG_Q) // quiet mode?
        return TRUE;

    if (s_dwFlags & FLAG_T) // print detailed info
    {
        // convert date/time
        FileTimeToLocalFileTime(&finddata->ftLastWriteTime, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
        // get date/time strings
        GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, _countof(szDate));
        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, _countof(szTime));
        // set size
        FileSize.LowPart = finddata->nFileSizeLow;
        FileSize.HighPart = finddata->nFileSizeHigh;
        // print
        if (s_dwFlags & FLAG_F) // double quote
            StringCbPrintfW(szPath, sizeof(szPath), L"\"%s\"", path);
        else
            StringCbCopyW(szPath, sizeof(szPath), path);
        ConResPrintf(StdOut, IDS_FILE_INFO, FileSize.QuadPart, szDate, szTime, szPath);
    }
    else // print path only
    {
        if (s_dwFlags & FLAG_F) // double quote
            ConPrintf(StdOut, L"\"%ls\"\n", path);
        else
            ConPrintf(StdOut, L"%ls\n", path);
    }

    return TRUE; // success
}

static BOOL WhereSearchFiles(LPCWSTR pattern, LPCWSTR dir)
{
    WCHAR szPath[MAX_PATH];
    INT iExt, cch;

    StringCchCopyW(szPath, _countof(szPath), dir);
    StringCchCatW(szPath, _countof(szPath), L"\\");
    StringCchCatW(szPath, _countof(szPath), pattern);
    cch = lstrlenW(szPath);

    for (iExt = 0; iExt < s_pathext.count; ++iExt)
    {
        // build path
        szPath[cch] = 0;
        StringCchCatW(szPath, _countof(szPath), strlist_get_at(&s_pathext, iExt)); // extension

        if (!WhereSearchGeneric(pattern, szPath, FALSE, WherePrintPath))
            return FALSE;
    }
    return TRUE;
}

static BOOL WhereSearchRecursive(LPCWSTR pattern, LPCWSTR dir);

static BOOL CALLBACK
WhereSearchRecursiveCallback(LPCWSTR pattern, LPCWSTR path, WIN32_FIND_DATAW *finddata)
{
    return WhereSearchRecursive(pattern, path);
}

// FIXME: Too slow. Optimize for speed.
static BOOL WhereSearchRecursive(LPCWSTR pattern, LPCWSTR dir)
{
    WCHAR szPath[MAX_PATH];
    if (!WhereSearchFiles(pattern, dir))
        return FALSE;

    // build path with wildcard
    StringCchCopyW(szPath, _countof(szPath), dir);
    StringCchCatW(szPath, _countof(szPath), L"\\*");

    return WhereSearchGeneric(pattern, szPath, TRUE, WhereSearchRecursiveCallback);
}

static BOOL WhereSearch(LPCWSTR pattern, strlist_t *dirlist)
{
    INT iDir;
    for (iDir = 0; iDir < dirlist->count; ++iDir)
    {
        if (!WhereSearchFiles(pattern, strlist_get_at(dirlist, iDir)))
            return FALSE;
    }
    return TRUE;
}

// get environment variable
static WRET WhereGetVariable(LPCWSTR name, LPWSTR *value)
{
    DWORD cch = GetEnvironmentVariableW(name, NULL, 0);
    *value = NULL;
    if (cch == 0) // variable not found
    {
        if (!(s_dwFlags & FLAG_Q)) // not quiet mode?
            ConResPrintf(StdErr, IDS_BAD_ENVVAR, name);
        return WRET_NOT_FOUND;
    }

    *value = malloc(cch * sizeof(WCHAR));
    if (!*value || !GetEnvironmentVariableW(name, *value, cch))
    {
        WhereError(IDS_OUTOFMEMORY);
        free(*value);
        *value = NULL;
        return WRET_ERROR;
    }
    return WRET_SUCCESS;
}

static BOOL WhereParseCommandLine(INT argc, WCHAR** argv)
{
    INT iArg;
    for (iArg = 1; iArg < argc; ++iArg)
    {
        LPWSTR arg = argv[iArg];
        if (arg[0] == L'/' || arg[0] == L'-')
        {
            if (arg[2] == 0)
            {
                switch (towupper(arg[1]))
                {
                    case L'?':
                        if (s_dwFlags & FLAG_HELP)
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/?", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE;
                        }
                        s_dwFlags |= FLAG_HELP;
                        continue;
                    case L'F':
                        if (s_dwFlags & FLAG_F)
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/F", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE;
                        }
                        s_dwFlags |= FLAG_F;
                        continue;
                    case L'Q':
                        if (s_dwFlags & FLAG_Q)
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/Q", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE;
                        }
                        s_dwFlags |= FLAG_Q;
                        continue;
                    case L'T':
                        if (s_dwFlags & FLAG_T)
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/T", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE;
                        }
                        s_dwFlags |= FLAG_T;
                        continue;
                    case L'R':
                    {
                        if (s_dwFlags & FLAG_R)
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/R", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE;
                        }
                        if (iArg + 1 < argc)
                        {
                            ++iArg;
                            s_pszRecursiveDir = argv[iArg];
                            s_dwFlags |= FLAG_R;
                            continue;
                        }
                        ConResPrintf(StdErr, IDS_WANT_VALUE, L"/R");
                        WhereError(IDS_TYPE_HELP);
                        return FALSE;
                    }
                }
            }
            ConResPrintf(StdErr, IDS_BAD_ARG, argv[iArg]);
            WhereError(IDS_TYPE_HELP);
            return FALSE;
        }
        else // pattern?
        {
            if (!strlist_add(&s_patterns, str_clone(argv[iArg]))) // append pattern
            {
                WhereError(IDS_OUTOFMEMORY);
                return FALSE;
            }
        }
    }

    return TRUE; // success
}

static BOOL WhereGetPathExt(strlist_t *ext_list)
{
    BOOL ret = TRUE;
    LPWSTR pszPathExt, ext;
    DWORD cchPathExt = GetEnvironmentVariableW(L"PATHEXT", NULL, 0);

    if (cchPathExt)
        pszPathExt = malloc(cchPathExt * sizeof(WCHAR));
    else
        pszPathExt = str_clone(DEFAULT_PATHEXT);

    if (!pszPathExt)
        return FALSE; // out of memory

    if (cchPathExt)
        GetEnvironmentVariableW(L"PATHEXT", pszPathExt, cchPathExt); // get PATHEXT data

    if (!strlist_add(ext_list, str_clone(L""))) // add empty extension for normal search
    {
        strlist_destroy(ext_list);
        free(pszPathExt);
        return FALSE;
    }

    // for all extensions
    for (ext = wcstok(pszPathExt, L";"); ext; ext = wcstok(NULL, L";"))
    {
        if (!strlist_add(ext_list, str_clone(ext))) // add extension to ext_list
        {
            strlist_destroy(ext_list);
            ret = FALSE;
            break;
        }
    }

    free(pszPathExt);
    return ret;
}

static BOOL WhereFind(LPCWSTR SearchFor, LPWSTR SearchData, BOOL IsVar)
{
    BOOL ret = TRUE;
    size_t cch;
    WCHAR szPath[MAX_PATH];
    LPWSTR pszValue, dir, dirs, pch;
    strlist_t dirlist = strlist_default;

    if (IsVar) // is SearchData a variable?
    {
        if (WhereGetVariable(SearchData, &pszValue) == WRET_ERROR)
            ret = FALSE;
        if (pszValue == NULL)
            goto quit;
        dirs = pszValue;
    }
    else
    {
        dirs = SearchData;
        pszValue = NULL;
    }

    // add the current directory
    GetCurrentDirectoryW(_countof(szPath), szPath);
    if (!strlist_add(&dirlist, str_clone(szPath)))
    {
        ret = FALSE;
        goto quit;
    }

    // this function is destructive against SearchData
    for (dir = wcstok(dirs, L";"); dir; dir = wcstok(NULL, L";"))
    {
        if (*dir == L'"') // began from '"'
        {
            ++dir;
            pch = wcschr(dir, L'"'); // find '"'
            if (*pch)
                *pch = 0; // cut off
        }

        if (*dir != '\\' && dir[1] != L':')
            continue; // relative path

        cch = wcslen(dir);
        if (cch > 0 && dir[cch - 1] == L'\\')
            dir[cch - 1] = 0; // remove trailing backslash

        if (!strlist_add(&dirlist, str_clone(dir)))
        {
            ret = FALSE;
            goto quit;
        }
    }

    ret = WhereSearch(SearchFor, &dirlist);
    if (!ret)
        goto quit;

quit:
    strlist_destroy(&dirlist);
    free(pszValue);
    return ret;
}

static BOOL WhereIsRecursiveDirOK(LPCWSTR name)
{
    DWORD attrs;
    if (wcschr(name, L';') != NULL)
    {
        WhereError(IDS_BAD_NAME);
        return FALSE;
    }
    else
    {
        attrs = GetFileAttributesW(name);
        if (attrs == INVALID_FILE_ATTRIBUTES) // file not found
        {
            WhereError(IDS_CANT_FOUND);
            return FALSE;
        }
        if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            WhereError(IDS_BAD_DIR);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL WhereDoPattern(LPWSTR SearchFor)
{
    LPWSTR pch = wcsrchr(SearchFor, L':');
    if (pch)
    {
        *pch++ = 0; // this function is destructive against SearchFor
        if (SearchFor[0] == L'$') // $env:pattern
        {
            if (s_dwFlags & FLAG_R) // recursive?
            {
                WhereError(IDS_ENVPAT_WITH_R);
                return FALSE;
            }
            return WhereFind(pch, SearchFor + 1, TRUE);
        }
        else // path:pattern
        {
            if (s_dwFlags & FLAG_R) // recursive?
            {
                WhereError(IDS_PATHPAT_WITH_R);
                return FALSE;
            }
            if (wcschr(pch, L'\\') != NULL) // found '\\'?
            {
                WhereError(IDS_BAD_PATHPAT);
                return FALSE;
            }
            return WhereFind(pch, SearchFor, FALSE);
        }
    }
    else if (s_pszRecursiveDir) // recursive
    {
        WCHAR szPath[MAX_PATH];

        if (!WhereIsRecursiveDirOK(s_pszRecursiveDir))
            return FALSE;

        GetFullPathNameW(s_pszRecursiveDir, _countof(szPath), szPath, NULL);

        return WhereSearchRecursive(SearchFor, szPath);
    }
    else // otherwise
    {
        return WhereFind(SearchFor, L"PATH", TRUE);
    }
}

INT __cdecl wmain(INT argc, WCHAR **argv)
{
    typedef BOOL (WINAPI *FN_DISABLE_WOW)(PVOID *);
    HANDLE hKernel32 = GetModuleHandleA("kernel32");
    FN_DISABLE_WOW DisableWOW =
        (FN_DISABLE_WOW)GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
    DWORD iPattern;
    WRET ret = WRET_ERROR;
    PVOID dummy;

    ConInitStdStreams(); // Initialize the Console Standard Streams

    if (!WhereParseCommandLine(argc, argv))
        goto quit;

    if ((s_dwFlags & FLAG_HELP) || !s_patterns.count)
    {
        ConResPuts(StdOut, IDS_USAGE);
        goto quit;
    }

    if (DisableWOW)
        DisableWOW(&dummy);

    if (!WhereGetPathExt(&s_pathext))
    {
        WhereError(IDS_OUTOFMEMORY);
        goto quit;
    }

    ret = WRET_SUCCESS;
    for (iPattern = 0; iPattern < s_patterns.count; ++iPattern)
    {
        if (!WhereDoPattern(strlist_get_at(&s_patterns, iPattern)))
        {
            ret = WRET_ERROR;
            goto quit;
        }
    }

    if (!s_results.count)
    {
        WhereError(IDS_NOT_FOUND);
        ret = WRET_NOT_FOUND;
    }

quit:
    strlist_destroy(&s_results);
    strlist_destroy(&s_patterns);
    strlist_destroy(&s_pathext);
    return ret;
}
