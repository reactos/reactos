/*
 * PROJECT:     ReactOS WHERE command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search executable files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
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

typedef BOOL (CALLBACK *WHERE_CALLBACK)(LPCWSTR pattern, LPCWSTR path, PWIN32_FIND_DATAW data);

static BOOL
WhereSearchGeneric(LPCWSTR pattern, LPWSTR path, size_t path_len, BOOL bDir,
                   WHERE_CALLBACK callback)
{
    LPWSTR pch;
    size_t cch;
    BOOL ret;
    WIN32_FIND_DATAW data;
    HANDLE hFind = FindFirstFileExW(path, FindExInfoStandard, &data, FindExSearchNameMatch,
                                    NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
        return TRUE; // not found

    pch = wcsrchr(path, L'\\') + 1;
    cch = path_len - (pch - path);
    do
    {
        if (bDir != !!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (bDir && IS_DOTS(data.cFileName))
            continue; // ignore "." and ".."
        if (data.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL)
            continue; // ignore virtual

        StringCchCopyW(pch, cch, data.cFileName); // build full path

        ret = callback(pattern, path, &data);
        if (!ret) // out of memory
            break;
    } while (FindNextFileW(hFind, &data));
    FindClose(hFind);
    return ret;
}

static BOOL CALLBACK WherePrintPath(LPCWSTR pattern, LPCWSTR path, PWIN32_FIND_DATAW data)
{
    WCHAR szPath[MAX_PATH + 2], szDate[32], szTime[32];
    LARGE_INTEGER FileSize;
    FILETIME ftLocal;
    SYSTEMTIME st;

    if (strlist_find_i(&s_results, path) >= 0)
        return TRUE; // already exists
    if (!strlist_add(&s_results, path))
        return FALSE; // out of memory
    if (s_dwFlags & FLAG_Q) // quiet mode?
        return TRUE;

    if (s_dwFlags & FLAG_T) // print detailed info
    {
        // convert date/time
        FileTimeToLocalFileTime(&data->ftLastWriteTime, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
        // get date/time strings
        GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, _countof(szDate));
        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, _countof(szTime));
        // set size
        FileSize.LowPart = data->nFileSizeLow;
        FileSize.HighPart = data->nFileSizeHigh;
        // print
        if (s_dwFlags & FLAG_F) // double quote
            StringCchPrintfW(szPath, _countof(szPath), L"\"%s\"", path);
        else
            StringCchCopyW(szPath, _countof(szPath), path);
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
    INT iExt;
    size_t cch;
    WCHAR szPath[MAX_PATH];
    StringCchCopyW(szPath, _countof(szPath), dir);
    StringCchCatW(szPath, _countof(szPath), L"\\");
    StringCchCatW(szPath, _countof(szPath), pattern);
    cch = wcslen(szPath);

    for (iExt = 0; iExt < s_pathext.count; ++iExt)
    {
        szPath[cch] = 0; // cut off extension
        // append extension
        StringCchCatW(szPath, _countof(szPath), strlist_get_at(&s_pathext, iExt));

        if (!WhereSearchGeneric(pattern, szPath, _countof(szPath), FALSE, WherePrintPath))
            return FALSE;
    }
    return TRUE;
}

static BOOL WhereSearchRecursive(LPCWSTR pattern, LPCWSTR dir);

static BOOL CALLBACK
WhereSearchRecursiveCallback(LPCWSTR pattern, LPCWSTR path, PWIN32_FIND_DATAW data)
{
    return WhereSearchRecursive(pattern, path);
}

// FIXME: Too slow. Optimize for speed.
static BOOL WhereSearchRecursive(LPCWSTR pattern, LPCWSTR dir)
{
    WCHAR szPath[MAX_PATH];
    if (!WhereSearchFiles(pattern, dir))
        return FALSE; // out of memory

    // build path with wildcard
    StringCchCopyW(szPath, _countof(szPath), dir);
    StringCchCatW(szPath, _countof(szPath), L"\\*");
    return WhereSearchGeneric(pattern, szPath, _countof(szPath), TRUE,
                              WhereSearchRecursiveCallback);
}

static BOOL WhereSearch(LPCWSTR pattern, strlist_t *dirlist)
{
    UINT iDir;
    for (iDir = 0; iDir < dirlist->count; ++iDir)
    {
        if (!WhereSearchFiles(pattern, strlist_get_at(dirlist, iDir)))
            return FALSE;
    }
    return TRUE;
}

static BOOL WhereGetVariable(LPCWSTR name, LPWSTR *value)
{
    DWORD cch = GetEnvironmentVariableW(name, NULL, 0);
    if (cch == 0) // variable not found
    {
        *value = NULL;
        if (!(s_dwFlags & FLAG_Q)) // not quiet mode?
            ConResPrintf(StdErr, IDS_BAD_ENVVAR, name);
        return TRUE; // it is error, but continue the task
    }

    *value = malloc(cch * sizeof(WCHAR));
    if (!*value || !GetEnvironmentVariableW(name, *value, cch))
    {
        free(*value);
        *value = NULL;
        return FALSE; // error
    }
    return TRUE;
}

static BOOL WhereDoOption(DWORD flag, LPCWSTR option)
{
    if (s_dwFlags & flag)
    {
        ConResPrintf(StdErr, IDS_TOO_MANY, option, 1);
        ConResPuts(StdErr, IDS_TYPE_HELP);
        return FALSE;
    }
    s_dwFlags |= flag;
    return TRUE;
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
                        if (!WhereDoOption(FLAG_HELP, L"/?"))
                            return FALSE;
                        continue;
                    case L'F':
                        if (!WhereDoOption(FLAG_F, L"/F"))
                            return FALSE;
                        continue;
                    case L'Q':
                        if (!WhereDoOption(FLAG_Q, L"/Q"))
                            return FALSE;
                        continue;
                    case L'T':
                        if (!WhereDoOption(FLAG_T, L"/T"))
                            return FALSE;
                        continue;
                    case L'R':
                    {
                        if (!WhereDoOption(FLAG_R, L"/R"))
                            return FALSE;
                        if (iArg + 1 < argc)
                        {
                            ++iArg;
                            s_pszRecursiveDir = argv[iArg];
                            continue;
                        }
                        ConResPrintf(StdErr, IDS_WANT_VALUE, L"/R");
                        ConResPuts(StdErr, IDS_TYPE_HELP);
                        return FALSE;
                    }
                }
            }
            ConResPrintf(StdErr, IDS_BAD_ARG, argv[iArg]);
            ConResPuts(StdErr, IDS_TYPE_HELP);
            return FALSE;
        }
        else // pattern?
        {
            if (!strlist_add(&s_patterns, argv[iArg])) // append pattern
            {
                ConResPuts(StdErr, IDS_OUTOFMEMORY);
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

    pszPathExt = (cchPathExt ? malloc(cchPathExt * sizeof(WCHAR)) : str_clone(DEFAULT_PATHEXT));
    if (!pszPathExt)
        return FALSE; // out of memory

    if (cchPathExt)
        GetEnvironmentVariableW(L"PATHEXT", pszPathExt, cchPathExt);

    if (!strlist_add(ext_list, L"")) // add empty extension for normal search
    {
        strlist_destroy(ext_list);
        free(pszPathExt);
        return FALSE;
    }

    for (ext = wcstok(pszPathExt, L";"); ext; ext = wcstok(NULL, L";")) // for all extensions
    {
        if (!strlist_add(ext_list, ext)) // add extension to ext_list
        {
            strlist_destroy(ext_list);
            ret = FALSE;
            break;
        }
    }

    free(pszPathExt);
    return ret;
}

static BOOL WhereFindByDirs(LPCWSTR pattern, LPWSTR dirs)
{
    BOOL ret;
    size_t cch;
    WCHAR szPath[MAX_PATH];
    LPWSTR dir, pch;
    strlist_t dirlist = strlist_default;

    GetCurrentDirectoryW(_countof(szPath), szPath);
    if (!strlist_add(&dirlist, szPath))
        return FALSE; // out of memory

    for (dir = wcstok(dirs, L";"); dir; dir = wcstok(NULL, L";"))
    {
        if (*dir == L'"') // began from '"'
        {
            pch = wcschr(++dir, L'"'); // find '"'
            if (*pch)
                *pch = 0; // cut off
        }

        if (*dir != '\\' && dir[1] != L':')
            continue; // relative path

        cch = wcslen(dir);
        if (cch > 0 && dir[cch - 1] == L'\\')
            dir[cch - 1] = 0; // remove trailing backslash

        if (!strlist_add(&dirlist, dir))
        {
            strlist_destroy(&dirlist);
            return FALSE; // out of memory
        }
    }

    ret = WhereSearch(pattern, &dirlist);
    strlist_destroy(&dirlist);
    return ret;
}

static BOOL WhereFindByVar(LPCWSTR pattern, LPCWSTR name)
{
    LPWSTR value;
    BOOL ret = WhereGetVariable(name, &value);
    if (ret && value)
        ret = WhereFindByDirs(pattern, value);
    free(value);
    return ret;
}

static BOOL WhereIsRecursiveDirOK(LPCWSTR name)
{
    if (wcschr(name, L';') != NULL)
    {
        WhereError(IDS_BAD_NAME);
        return FALSE;
    }
    else
    {
        DWORD attrs = GetFileAttributesW(name);
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
        return TRUE;
    }
}

static BOOL WhereDoPattern(LPWSTR pattern)
{
    BOOL ret;
    LPWSTR pch = wcsrchr(pattern, L':');
    if (pch)
    {
        *pch++ = 0;
        if (pattern[0] == L'$') // $env:pattern
        {
            if (s_dwFlags & FLAG_R) // recursive?
            {
                WhereError(IDS_ENVPAT_WITH_R);
                return FALSE;
            }
            ret = WhereFindByVar(pch, pattern + 1);
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
            ret = WhereFindByDirs(pch, pattern);
        }
    }
    else if (s_pszRecursiveDir) // recursive
    {
        WCHAR szPath[MAX_PATH];

        if (!WhereIsRecursiveDirOK(s_pszRecursiveDir))
            return FALSE;

        GetFullPathNameW(s_pszRecursiveDir, _countof(szPath), szPath, NULL);

        ret = WhereSearchRecursive(pattern, szPath);
    }
    else // otherwise
    {
        ret = WhereFindByVar(pattern, L"PATH");
    }

    if (!ret)
        WhereError(IDS_OUTOFMEMORY);
    return ret;
}

INT wmain(INT argc, WCHAR **argv)
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
