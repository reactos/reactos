/*
 * PROJECT:     ReactOS WHERE command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search executable files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#ifdef __REACTOS__
    #include <windef.h>
    #include <winbase.h>
    #include <winuser.h>
    #include <winnls.h>
#else
    #include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <strsafe.h> // StringC...
#include "resource.h"
#if 0
    #include <conutils.h> // StdOut/StdErr, Con...
#else
    #include "miniconutils.h" // It can reduce the program size.
#endif
#include "strlist.h" // strlist_...

#define FLAG_HELP (1 << 0) // "/?"
#define FLAG_R (1 << 1) // recursive directory
#define FLAG_Q (1 << 2) // quiet mode
#define FLAG_F (1 << 3) // double quote
#define FLAG_T (1 << 4) // detailed info

static DWORD s_dwFlags = 0;
static LPWSTR s_RecursiveDir = NULL;
static strlist_t s_targets = strlist_default;
static strlist_t s_founds = strlist_default;
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

static BOOL WherePrintPath(LPCWSTR FoundPath)
{
    WCHAR szPath[MAX_PATH], szDate[32], szTime[32];
    LARGE_INTEGER FileSize;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    FILETIME ftLocal;
    SYSTEMTIME st;
    INT iPath;

    iPath = strlist_find_i(&s_founds, FoundPath); // find string in s_founds
    if (iPath >= 0)
        return TRUE; // already exists
    if (!strlist_add(&s_founds, str_clone(FoundPath))) // append found path
        return FALSE; // failure
    if (s_dwFlags & FLAG_Q) // quiet mode?
        return TRUE; // success

    if (s_dwFlags & FLAG_T) // print detailed info
    {
        // get info
        hFind = FindFirstFileExW(FoundPath, FindExInfoStandard, &find, FindExSearchNameMatch,
                                 NULL, 0);
        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);
        // convert date/time
        FileTimeToLocalFileTime(&find.ftLastWriteTime, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
        // get date/time strings
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szDate, _countof(szDate));
        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, _countof(szTime));
        // set size
        FileSize.LowPart = find.nFileSizeLow;
        FileSize.HighPart = find.nFileSizeHigh;
        // print
        if (s_dwFlags & FLAG_F) // double quote
            StringCbPrintfW(szPath, sizeof(szPath), L"\"%s\"", FoundPath);
        else
            StringCbCopyW(szPath, sizeof(szPath), FoundPath);
        ConResPrintf(StdOut, IDS_FILE_INFO, FileSize.QuadPart, szDate, szTime, szPath);
    }
    else // print path only
    {
        if (s_dwFlags & FLAG_F) // double quote
            ConPrintf(StdOut, L"\"%ls\"\n", FoundPath);
        else
            ConPrintf(StdOut, L"%ls\n", FoundPath);
    }

    return TRUE; // success
}

static BOOL WhereSearchFiles(LPCWSTR filename, LPCWSTR dir)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pch;
    INT cch, iExt;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    BOOL ret = TRUE;

    for (iExt = 0; iExt < s_pathext.count; ++iExt)
    {
        // build path
        StringCchCopyW(szPath, _countof(szPath), dir);
        StringCchCatW(szPath, _countof(szPath), L"\\");
        StringCchCatW(szPath, _countof(szPath), filename);
        StringCchCatW(szPath, _countof(szPath), strlist_get_at(&s_pathext, iExt));

        // enumerate file items
        hFind = FindFirstFileExW(szPath, FindExInfoStandard, &find, FindExSearchNameMatch,
                                 NULL, 0);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            pch = wcsrchr(szPath, L'\\') + 1; // file title
            cch = _countof(szPath) - (pch - szPath); // remainder
            do
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue; // it was directory

                StringCchCopyW(pch, cch, find.cFileName); // build full path

                if (!WherePrintPath(szPath))
                {
                    WhereError(IDS_OUTOFMEMORY);
                    ret = FALSE;
                    break;
                }
            } while (FindNextFile(hFind, &find));
            FindClose(hFind);
        }
        if (!ret)
            break;
    }

    return ret;
}

// FIXME: Too slow. Optimize for speed.
static BOOL WhereSearchRecursive(LPCWSTR filename, LPCWSTR dir)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pch;
    INT cch;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    BOOL ret = WhereSearchFiles(filename, dir); // search files in the directory
    if (!ret)
        return ret;

    // build path with wildcard
    StringCchCopyW(szPath, _countof(szPath), dir);
    StringCchCatW(szPath, _countof(szPath), L"\\*");

    // enumerate directory items
    hFind = FindFirstFileExW(szPath, FindExInfoStandard, &find, FindExSearchNameMatch,
                             NULL, 0);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        pch = wcsrchr(szPath, L'\\') + 1; // file title
        cch = _countof(szPath) - (pch - szPath); // remainder
        do
        {
            if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue; // not directory

            if (IS_DOTS(find.cFileName))
                continue; // ignore "." and ".."

            StringCchCopyW(pch, cch, find.cFileName); // build full path

            ret = WhereSearchRecursive(filename, szPath); // recurse
            if (!ret)
                break;
        } while (FindNextFileW(hFind, &find));
        FindClose(hFind);
    }

    return ret;
}

static BOOL WhereSearch(LPCWSTR filename, strlist_t *dirlist)
{
    INT iDir;
    for (iDir = 0; iDir < dirlist->count; ++iDir) // for each directory
    {
        LPWSTR dir = strlist_get_at(dirlist, iDir);
        BOOL ret = WhereSearchFiles(filename, dir); // search
        if (!ret)
            return ret;
    }
    return TRUE;
}

// get environment variable
static WRET WhereGetVariable(LPCWSTR name, LPWSTR *ppszData)
{
    DWORD cchData = GetEnvironmentVariableW(name, NULL, 0); // is there the variable?
    *ppszData = NULL;
    if (cchData == 0) // variable not found
    {
        if (!(s_dwFlags & FLAG_Q))
            ConResPrintf(StdErr, IDS_BAD_ENVVAR, name);
        return WRET_NOT_FOUND;
    }

    // allocate and get the variable's value
    *ppszData = malloc(cchData * sizeof(WCHAR));
    if (!*ppszData || !GetEnvironmentVariableW(name, *ppszData, cchData))
    {
        WhereError(IDS_OUTOFMEMORY);
        free(*ppszData);
        *ppszData = NULL;
        return WRET_ERROR;
    }
    return WRET_SUCCESS;
}

static BOOL WhereParseCommandLine(INT argc, WCHAR** argv)
{
    INT iArg;
    for (iArg = 1; iArg < argc; ++iArg) // for each parameter
    {
        LPWSTR arg = argv[iArg];
        if (arg[0] == L'/' || arg[0] == L'-') // flag?
        {
            if (arg[2] == 0) // shortly terminated?
            {
                switch (towupper(arg[1]))
                {
                    case L'?':
                        if (s_dwFlags & FLAG_HELP) // already specified
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/?", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE; // failure
                        }
                        s_dwFlags |= FLAG_HELP;
                        continue;
                    case L'F':
                        if (s_dwFlags & FLAG_F) // already specified
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/F", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE; // failure
                        }
                        s_dwFlags |= FLAG_F;
                        continue;
                    case L'Q':
                        if (s_dwFlags & FLAG_Q) // already specified
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/Q", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE; // failure
                        }
                        s_dwFlags |= FLAG_Q;
                        continue;
                    case L'T':
                        if (s_dwFlags & FLAG_T) // already specified
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/T", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE; // failure
                        }
                        s_dwFlags |= FLAG_T;
                        continue;
                    case L'R':
                    {
                        if (s_dwFlags & FLAG_R) // already specified
                        {
                            ConResPrintf(StdErr, IDS_TOO_MANY, L"/R", 1);
                            WhereError(IDS_TYPE_HELP);
                            return FALSE; // failure
                        }
                        if (iArg + 1 < argc)
                        {
                            ++iArg;
                            s_RecursiveDir = argv[iArg];
                            s_dwFlags |= FLAG_R;
                            continue;
                        }
                        ConResPrintf(StdErr, IDS_WANT_VALUE, L"/R");
                        WhereError(IDS_TYPE_HELP);
                        return FALSE; // failure
                    }
                }
            }
            ConResPrintf(StdErr, IDS_BAD_ARG, argv[iArg]); // invalid argument
            WhereError(IDS_TYPE_HELP);
            return FALSE; // failure
        }
        else // target?
        {
            if (!strlist_add(&s_targets, str_clone(argv[iArg]))) // append target
            {
                WhereError(IDS_OUTOFMEMORY);
                return FALSE; // failure
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

    // allocate for PATHEXT info
    if (cchPathExt)
        pszPathExt = malloc(cchPathExt * sizeof(WCHAR));
    else
        pszPathExt = str_clone(DEFAULT_PATHEXT);

    if (!pszPathExt)
        return FALSE; // out of memory

    if (cchPathExt)
        GetEnvironmentVariableW(L"PATHEXT", pszPathExt, cchPathExt); // get PATHEXT data

    CharLowerW(pszPathExt); // make it lowercase

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
    else // directories
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

        if (!strlist_add(&dirlist, str_clone(dir))) // add directory of PATH
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
    if (wcschr(name, L';') == NULL) // not found ';'?
    {
        attrs = GetFileAttributesW(name);
        if (attrs == INVALID_FILE_ATTRIBUTES) // file not found
        {
            WhereError(IDS_CANT_FOUND);
            return FALSE;
        }
        if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) // not directory
        {
            WhereError(IDS_BAD_DIR);
            return FALSE;
        }
    }
    else // found ';'
    {
        WhereError(IDS_BAD_NAME);
        return FALSE;
    }
    return TRUE;
}

static BOOL WhereDoTarget(LPWSTR SearchFor)
{
    LPWSTR pch = wcsrchr(SearchFor, L':');
    if (pch) // found ':'
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
    else if (s_RecursiveDir) // recursive
    {
        WCHAR szPath[MAX_PATH];

        if (!WhereIsRecursiveDirOK(s_RecursiveDir))
            return FALSE;

        GetFullPathNameW(s_RecursiveDir, _countof(szPath), szPath, NULL); // get full path

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
    DWORD iTarget;
    WRET ret = WRET_ERROR;
    PVOID dummy;

    ConInitStdStreams(); // Initialize the Console Standard Streams

    if (!WhereParseCommandLine(argc, argv))
        goto quit;

    if ((s_dwFlags & FLAG_HELP) || !s_targets.count)
    {
        ConResPuts(StdOut, IDS_USAGE);
        goto quit;
    }

    if (DisableWOW)
        DisableWOW(&dummy);

    if (!WhereGetPathExt(&s_pathext)) // get PATHEXT info
    {
        WhereError(IDS_OUTOFMEMORY);
        goto quit;
    }

    // do targets
    ret = WRET_SUCCESS;
    for (iTarget = 0; iTarget < s_targets.count; ++iTarget)
    {
        if (!WhereDoTarget(strlist_get_at(&s_targets, iTarget)))
        {
            ret = WRET_ERROR;
            goto quit;
        }
    }

    if (!s_founds.count) // not found
    {
        WhereError(IDS_NOT_FOUND);
        ret = WRET_NOT_FOUND;
    }

quit: // cleanup
    strlist_destroy(&s_founds);
    strlist_destroy(&s_targets);
    strlist_destroy(&s_pathext);
    return ret;
}

#ifndef __REACTOS__
int main(int argc, char **argv)
{
    INT my_argc;
    LPWSTR *my_argv = CommandLineToArgvW(GetCommandLineW(), &my_argc);
    INT ret = wmain(my_argc, my_argv);
    LocalFree(my_argv);
    return ret;
}
#endif
