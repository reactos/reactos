/*
 * PROJECT:     ReactOS WHERE command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search executable files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#define WIN32_LEAN_AND_MEAN
#ifdef __REACTOS__
    #include <windef.h>
    #include <winbase.h>
    #include <winuser.h>
#else
    #include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include "resource.h"
#ifdef __REACTOS__
    #include <conutils.h>
#else
    #include "../donutils.h"
#endif
#include "strlist.h"

#define FLAG_HELP (1 << 0)
#define FLAG_R (1 << 1) // recursive directory
#define FLAG_Q (1 << 2) // quiet
#define FLAG_F (1 << 3) // double quote
#define FLAG_T (1 << 4) // detailed info

static DWORD s_dwFlags = 0;
static LPWSTR s_SearchDir = NULL;
static strlist_t s_targets = strlist_default;
static strlist_t s_founds = strlist_default;
static strlist_t s_pathext = strlist_default;

// is it either "." or ".."?
#define IS_DOTS(pch) \
    (*(pch) == L'.' && ((pch)[1] == 0 || ((pch)[1] == L'.' && (pch)[2] == 0)))

#define DEFAULT_PATHEXT L".com;.exe;.bat;.cmd"

typedef enum WRET // return code
{
    WRET_SUCCESS = 0,
    WRET_NOT_FOUND = 1,
    WRET_ERROR = 2
} WRET;

static inline VOID WhereError(UINT nID)
{
    if (!(s_dwFlags & FLAG_Q))
        ConResPuts(StdErr, nID);
}

typedef BOOL (CALLBACK *FN_SHOW_PATH)(LPCWSTR FoundPath);

static WRET WhereSearchFiles(LPCWSTR filename, LPCWSTR dir, FN_SHOW_PATH callback)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pch;
    INT cch;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    WRET ret = WRET_SUCCESS;

    StringCchPrintfW(szPath, _countof(szPath), L"%ls\\%ls", dir, filename); // build path
    pch = wcsrchr(szPath, L'\\') + 1; // file title
    cch = _countof(szPath) - (pch - szPath); // remainder

    // enumerate file items
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue; // it was directory

            StringCchCopyW(pch, cch, find.cFileName); // build full path

            if (!(*callback)(szPath))
            {
                WhereError(IDS_OUTOFMEMORY);
                ret = WRET_ERROR;
                break;
            }
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    return ret;
}

static WRET WhereSearchRecursive(LPCWSTR filename, LPCWSTR dir, FN_SHOW_PATH callback)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pch;
    INT cch;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    WRET ret = WhereSearchFiles(filename, dir, callback);
    if (ret == WRET_ERROR)
        return ret;

    StringCbPrintfW(szPath, sizeof(szPath), L"%ls\\*", dir); // build path with wildcard
    pch = wcsrchr(szPath, L'\\') + 1; // file title
    cch = _countof(szPath) - (pch - szPath); // remainder

    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue; // not directory

            if (IS_DOTS(find.cFileName))
                continue; // "." or ".."

            StringCchCopyW(pch, cch, find.cFileName); // build full path

            ret = WhereSearchRecursive(filename, szPath, callback);
            if (ret == WRET_ERROR)
                break;
        } while (FindNextFileW(hFind, &find));
        FindClose(hFind);
    }

    return ret;
}

static WRET
WhereSearch(LPCWSTR filename, strlist_t *dirlist, FN_SHOW_PATH callback)
{
    INT iDir;
    WRET ret;
    LPWSTR dir;

    for (iDir = 0; iDir < dirlist->count; ++iDir)
    {
        dir = strlist_get_at(dirlist, iDir);
        ret = WhereSearchFiles(filename, dir, callback);
        if (ret == WRET_ERROR)
            return ret;
    }

    return WRET_SUCCESS;
}

// get environment variable
static WRET WhereGetEnvVar(LPCWSTR name, LPWSTR *ppszData)
{
    DWORD cchData = GetEnvironmentVariableW(name, NULL, 0);
    *ppszData = NULL;

    if (cchData == 0) // not found
    {
        ConResPrintf(StdErr, IDS_BAD_ENVVAR, name);
        return WRET_NOT_FOUND;
    }

    // allocate and get the value of env var
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
    for (iArg = 1; iArg < argc; ++iArg)
    {
        LPWSTR arg = argv[iArg];
        if (arg[0] == L'/' || arg[0] == L'-')
        {
            if (arg[2] == 0)
            {
                switch (towupper(arg[1]))
                {
                    case L'?': s_dwFlags |= FLAG_HELP; continue;
                    case L'F': s_dwFlags |= FLAG_F; continue;
                    case L'Q': s_dwFlags |= FLAG_Q; continue;
                    case L'T': s_dwFlags |= FLAG_T; continue;
                    case L'R':
                    {
                        if (iArg + 1 < argc)
                        {
                            ++iArg;
                            s_SearchDir = argv[iArg];
                            s_dwFlags |= FLAG_R;
                            continue;
                        }
                        ConResPrintf(StdErr, IDS_BAD_SYNTAX, L"/R");
                        WhereError(IDS_TYPE_HELP);
                        return FALSE;
                    }
                }
            }
            ConResPrintf(StdErr, IDS_INVALID_ARG, argv[iArg]);
            WhereError(IDS_TYPE_HELP);
            return FALSE;
        }
        else
        {
            if (!strlist_add(&s_targets, str_clone(argv[iArg])))
            {
                WhereError(IDS_OUTOFMEMORY);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL CALLBACK WherePrintPath(LPCWSTR FoundPath)
{
    WCHAR szPath[MAX_PATH], szDate[32], szTime[32];
    LARGE_INTEGER FileSize;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    FILETIME ftLocal;
    SYSTEMTIME st;
    INT iPath;

    iPath = strlist_find_i(&s_founds, FoundPath);
    if (iPath != -1)
        return TRUE; // already exists
    if (!strlist_add(&s_founds, str_clone(FoundPath)))
        return FALSE; // failure
    if (s_dwFlags & FLAG_Q) // quiet
        return TRUE; // success

    if (s_dwFlags & FLAG_F) // double quote
        StringCbPrintfW(szPath, sizeof(szPath), L"\"%s\"", FoundPath);
    else
        StringCbCopyW(szPath, sizeof(szPath), FoundPath);

    if (s_dwFlags & FLAG_T) // detailed info
    {
        hFind = FindFirstFileW(FoundPath, &find);
        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);
        FileTimeToLocalFileTime(&find.ftLastWriteTime, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
        StringCbPrintfW(szDate, sizeof(szDate), L"%04u/%02u/%02u",
                        st.wYear, st.wMonth, st.wDay);
        StringCbPrintfW(szTime, sizeof(szTime), L"%02u:%02u:%02u",
                        st.wHour, st.wMinute, st.wSecond);
        FileSize.LowPart = find.nFileSizeLow;
        FileSize.HighPart = find.nFileSizeHigh;
        ConResPrintf(StdOut, IDS_FILE_INFO, FileSize.QuadPart, szDate, szTime, szPath);
    }
    else // path only
    {
        ConPrintf(StdOut, L"%ls\n", szPath);
    }

    return TRUE; // success
}

static BOOL WhereGetPathExt(strlist_t *plist)
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

    if (!strlist_add(plist, str_clone(L""))) // empty extension
    {
        strlist_destroy(plist);
        free(pszPathExt);
        return FALSE;
    }

    // for all exts
    for (ext = wcstok(pszPathExt, L";"); ext; ext = wcstok(NULL, L";"))
    {
        // add to plist
        if (!strlist_add(plist, str_clone(ext)))
        {
            strlist_destroy(plist);
            ret = FALSE;
            break;
        }
    }

    free(pszPathExt);
    return ret;
}

static WRET WhereFind(LPCWSTR SearchFor, LPWSTR SearchData, BOOL IsVar)
{
    WRET ret = WRET_SUCCESS;
    WCHAR szPath[MAX_PATH];
    INT iExt;
    LPWSTR pszValue, dir, dirs, pch;
    strlist_t dirlist = strlist_default;

    if (IsVar) // is SearchData an env var?
    {
        ret = WhereGetEnvVar(SearchData, &pszValue);
        if (ret == WRET_ERROR || pszValue == NULL)
            goto quit;
        dirs = pszValue;
    }
    else // paths?
    {
        dirs = SearchData;
        pszValue = NULL;
    }

    GetCurrentDirectoryW(_countof(szPath), szPath);
    if (!strlist_add(&dirlist, str_clone(szPath)))
    {
        ret = WRET_ERROR;
        goto quit;
    }

    for (dir = wcstok(dirs, L";"); dir; dir = wcstok(NULL, L";"))
    {
        // make path
        if (*dir == L'"')
        {
            ++dir;
            pch = wcschr(dir, L'"');
            if (*pch)
                *pch = 0;
        }

        if (*dir != '\\' && dir[1] != L':')
            continue; // relative path

        if (!strlist_add(&dirlist, str_clone(dir)))
        {
            ret = WRET_ERROR;
            goto quit;
        }
    }

    for (iExt = 0; iExt < s_pathext.count; ++iExt) // with extension
    {
        StringCbCopyW(szPath, sizeof(szPath), SearchFor);
        StringCbCatW(szPath, sizeof(szPath), strlist_get_at(&s_pathext, iExt));
        ret = WhereSearch(szPath, &dirlist, WherePrintPath);
        if (ret == WRET_ERROR)
            goto quit;
    }

quit:
    strlist_destroy(&dirlist);
    free(pszValue);
    return ret;
}

static WRET WhereDoTarget(LPWSTR SearchFor)
{
    LPWSTR pch = wcsrchr(SearchFor, L':');
    if (pch)
    {
        *pch++ = 0; // this function is destructive against SearchFor
        if (SearchFor[0] == L'$') // $env:pattern
        {
            if (s_dwFlags & FLAG_R)
            {
                ConResPuts(StdErr, IDS_ENVPAT_WITH_R);
                return WRET_ERROR;
            }
            return WhereFind(pch, SearchFor + 1, TRUE);
        }
        else // path:pattern
        {
            if (s_dwFlags & FLAG_R)
            {
                ConResPuts(StdErr, IDS_PATHPAT_WITH_R);
                return WRET_ERROR;
            }
            if (wcschr(pch, L'\\'))
            {
                ConResPuts(StdErr, IDS_BAD_PATHPAT);
                return WRET_ERROR;
            }
            return WhereFind(pch, SearchFor, FALSE);
        }
    }
    else if (s_SearchDir)
    {
        INT iExt;
        WRET ret;
        WCHAR szPath[MAX_PATH], filename[MAX_PATH];
        DWORD attrs;

        if (wcschr(s_SearchDir, L';') == NULL)
        {
            attrs = GetFileAttributesW(s_SearchDir);
            if (attrs == INVALID_FILE_ATTRIBUTES) // not found
            {
                WhereError(IDS_CANT_FOUND);
                return WRET_ERROR;
            }
            if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) // not directory
            {
                WhereError(IDS_BAD_DIR);
                return WRET_ERROR;
            }
        }
        else // found ';'
        {
            WhereError(IDS_BAD_NAME);
            return WRET_ERROR;
        }

        GetFullPathNameW(s_SearchDir, _countof(szPath), szPath, NULL); // get full path

        for (iExt = 0; iExt < s_pathext.count; ++iExt) // with extension
        {
            LPWSTR ext = strlist_get_at(&s_pathext, iExt);
            StringCbCopyW(filename, sizeof(filename), SearchFor);
            StringCbCatW(filename, sizeof(filename), ext);
            ret = WhereSearchRecursive(filename, szPath, WherePrintPath);
            if (ret == WRET_ERROR)
                return ret;
        }
        return WRET_SUCCESS;
    }
    else
    {
        return WhereFind(SearchFor, L"PATH", TRUE);
    }
}

INT __cdecl wmain(INT argc, WCHAR **argv)
{
    typedef BOOL (WINAPI *FN_DISABLE_WOW)(PVOID *);
    HANDLE hKernel32 = GetModuleHandleA("kernel32");
    FN_DISABLE_WOW DisableWow =
        (FN_DISABLE_WOW)GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
    DWORD iTarget;
    WRET ret;

    ConInitStdStreams(); // Initialize the Console Standard Streams

    if (!WhereParseCommandLine(argc, argv))
        goto quit;

    if ((s_dwFlags & FLAG_HELP) || !s_targets.count)
    {
        ConResPuts(StdOut, IDS_USAGE);
        goto quit;
    }

    if (DisableWow)
    {
        PVOID dummy;
        DisableWow(&dummy);
    }

    if (!WhereGetPathExt(&s_pathext)) // get PATHEXT info
    {
        WhereError(IDS_OUTOFMEMORY);
        goto quit;
    }

    // do targets
    ret = WRET_SUCCESS;
    for (iTarget = 0; iTarget < s_targets.count; ++iTarget)
    {
        if (WhereDoTarget(strlist_get_at(&s_targets, iTarget)) == WRET_ERROR)
        {
            ret = WRET_ERROR;
            goto quit;
        }
    }

    if (!s_founds.count && ret != WRET_ERROR) // not found
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
