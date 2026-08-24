/*
 * PROJECT:     ReactOS System Setup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 *              or GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Various utility routines
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include <stdlib.h>

#define NDEBUG
#include <debug.h>

BOOL
DoesPathExist(
    _In_ PCWSTR pszPath,
    _Out_opt_ PDWORD pAttrs)
{
    UINT prevMode;
    DWORD attrs;

    /* Prevent a dialog box if path is on a disk that has been ejected */
    prevMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    attrs = GetFileAttributesW(pszPath);
    SetErrorMode(prevMode);

    if (pAttrs)
        *pAttrs = attrs;
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

BOOL
DoesFileExist(
    _In_ PCWSTR pszPath)
{
    DWORD attrs = 0;
    return (DoesPathExist(pszPath, &attrs) && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL
DoesDirExist(
    _In_ PCWSTR pszPath)
{
    DWORD attrs = 0;
    return (DoesPathExist(pszPath, &attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL
RecursiveRemoveDir(
    _In_ PCWSTR pPath)
{
    WIN32_FIND_DATAW FindData;
    HANDLE hFind;
    BOOL bResult;
    WCHAR szPath[MAX_PATH];

    if (!SUCCEEDED(StringCchCopyW(szPath, _countof(szPath), pPath)) ||
        !SUCCEEDED(StringCchCatW(szPath, _countof(szPath), L"\\*.*")))
    {
        DPRINT1("Path too long\n");
        return FALSE;
    }
    DPRINT("Search path: '%S'\n", szPath);

    hFind = FindFirstFileW(szPath, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    bResult = TRUE;
    while (TRUE)
    {
        if (wcscmp(FindData.cFileName, L".") &&
            wcscmp(FindData.cFileName, L".."))
        {
            if (!SUCCEEDED(StringCchCopyW(szPath, _countof(szPath), pPath)) ||
                !SUCCEEDED(StringCchCatW(szPath, _countof(szPath), L"\\"))  ||
                !SUCCEEDED(StringCchCatW(szPath, _countof(szPath), FindData.cFileName)))
            {
                DPRINT1("Path too long\n");
                bResult = FALSE;
                continue;
            }
            DPRINT("File name: '%S'\n", szPath);

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                DPRINT("Delete directory: '%S'\n", szPath);

                if (!RecursiveRemoveDir(szPath))
                {
                    bResult = FALSE;
                    break;
                }

__debugbreak();
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                {
                    SetFileAttributesW(szPath,
                                       FindData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                }
                if (!RemoveDirectoryW(szPath))
                {
                    DPRINT1("RemoveDirectoryW(%S) failed, Error %lu\n", szPath, GetLastError());
                    bResult = FALSE;
                    break;
                }
            }
            else
            {
                DPRINT("Delete file: '%S'\n", szPath);

                if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
                    SetFileAttributesW(szPath, FILE_ATTRIBUTE_NORMAL);

                if (!DeleteFileW(szPath))
                {
                    DPRINT1("DeleteFileW(%S) failed, Error %lu\n", szPath, GetLastError());
                    bResult = FALSE;
                    break;
                }
            }
        }

        if (!FindNextFileW(hFind, &FindData))
        {
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                DPRINT1("Error: %lu\n", GetLastError());
                bResult = FALSE;
            }
            break;
        }
    }

    FindClose(hFind);

    return bResult;
}

BOOL
RemoveDirectoryPath(
    _In_ PCWSTR pPathName)
{
    if (!RecursiveRemoveDir(pPathName))
        return FALSE;

    DPRINT("Delete directory: '%S'\n", pPathName);
    return RemoveDirectoryW(pPathName);
}
