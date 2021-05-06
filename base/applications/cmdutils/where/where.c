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

static WRET WhereSearchInner(LPCWSTR filename, LPWSTR dirs, FN_SHOW_PATH callback)
{
    BOOL fFound = FALSE;
    WCHAR szPath[MAX_PATH], szFull[MAX_PATH];
    LPWSTR dir, title, pch;
    INT cch;
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    DWORD attrs = GetFileAttributesW(filename);

    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) // just match
    {
        fFound = TRUE;
        GetFullPathNameW(filename, _countof(szFull), szFull, NULL); // full path

        if (!(*callback)(szFull))
        {
            WhereError(IDS_OUTOFMEMORY);
            return WRET_ERROR;
        }
    }

    // this function is destructive against dirs
    for (dir = wcstok(dirs, L";"); dir; dir = wcstok(NULL, L";"))
    {
        // build path
        if (*dir == L'"')
        {
            ++dir;
            pch = wcschr(dir, L'"');
            if (*pch)
                *pch = 0;
        }
        StringCchPrintfW(szPath, _countof(szPath), L"%ls\\%ls", dir, filename);

        // enumerate file items
        hFind = FindFirstFileW(szPath, &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                fFound = TRUE;

                // build full path
                GetFullPathNameW(szPath, _countof(szFull), szFull, &title);
                cch = _countof(szFull) - (INT)(title - szFull);
                StringCchCopyW(title, cch, find.cFileName);

                if (!(*callback)(szFull))
                {
                    WhereError(IDS_OUTOFMEMORY);
                    return WRET_ERROR;
                }
            } while (FindNextFile(hFind, &find));
            FindClose(hFind);
        }
    }

    return (fFound ? WRET_SUCCESS : WRET_NOT_FOUND);
}

static WRET WhereSearch(LPCWSTR filename, LPCWSTR pszDirs, FN_SHOW_PATH callback)
{
    WRET ret;
    LPWSTR pszClone = str_clone(pszDirs); // WhereSearchInner is destructive. It needs a clone.
    if (!pszClone)
    {
        WhereError(IDS_OUTOFMEMORY);
        return WRET_ERROR;
    }
    ret = WhereSearchInner(filename, pszClone, callback);
    free(pszClone);
    return ret;
}

static WRET WhereGetEnvVar(LPCWSTR name, LPWSTR *ppszData)
{
    DWORD cchData = GetEnvironmentVariableW(name, NULL, 0);
    *ppszData = NULL;

    if (cchData == 0) // env var is not found
    {
        ConResPrintf(StdErr, IDS_BAD_ENVVAR, name);
        return WRET_NOT_FOUND;
    }

    // get data of env var
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
        return TRUE;

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
    return TRUE;
}

static BOOL WhereGetPathExt(strlist_t *plist)
{
#define DEFAULT_PATHEXT L".com;.exe;.bat;.cmd"
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
#undef DEFAULT_PATHEXT
}

static WRET WhereFind(LPCWSTR SearchFor, LPCWSTR SearchData, BOOL IsVar)
{
    WCHAR filename[MAX_PATH];
    INT iExt;
    WRET ret = WRET_SUCCESS;
    LPWSTR pszValue = NULL;
    LPCWSTR pszDirs;

    if (IsVar) // is SearchData an env var?
    {
        ret = WhereGetEnvVar(SearchData, &pszValue);
        if (ret == WRET_ERROR || pszValue == NULL)
            goto quit;
        pszDirs = pszValue;
    }
    else // paths?
    {
        pszDirs = SearchData;
    }

    // without extension
    ret = WhereSearch(SearchFor, pszDirs, WherePrintPath);
    if (ret == WRET_ERROR)
        goto quit;

    // with extension
    for (iExt = 0; iExt < s_pathext.count; ++iExt)
    {
        StringCbCopyW(filename, sizeof(filename), SearchFor);
        StringCbCatW(filename, sizeof(filename), strlist_get_at(&s_pathext, iExt));
        ret = WhereSearch(filename, pszDirs, WherePrintPath);
        if (ret == WRET_ERROR)
            goto quit;
    }

quit:
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
        return WhereFind(SearchFor, s_SearchDir, FALSE);
    }
    else
    {
        static WCHAR szPath[] = L"PATH";
        return WhereFind(SearchFor, szPath, TRUE);
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

    // Initialize the Console Standard Streams
    ConInitStdStreams();

#if 0 // no need to initialize here
    s_dwFlags = 0;
    s_SearchDir = NULL;
    strlist_init(&s_targets);
    strlist_init(&s_founds);
    strlist_init(&s_pathext);
#endif
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
