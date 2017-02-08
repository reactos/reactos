/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/fileutils.c
 * PURPOSE:     File Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "utils.h"
#include "fileutils.h"

//
// NOTE: A function called "FileExists" with the very same prototype
// already exists in the PSDK headers (in setupapi.h)
//
BOOL
MyFileExists(IN  LPCWSTR           lpszFilePath,
             OUT PWIN32_FIND_DATAW pFindData OPTIONAL)
{
    BOOL             bIsFound = FALSE;
    WIN32_FIND_DATAW find_data;

    DWORD  dwNumOfChars;
    LPWSTR lpszCmdLine;
    HANDLE search;

    dwNumOfChars = ExpandEnvironmentStringsW(lpszFilePath, NULL, 0);
    lpszCmdLine  = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszFilePath, lpszCmdLine, dwNumOfChars);

    search = FindFirstFileW(lpszCmdLine, &find_data);
    MemFree(lpszCmdLine);

    bIsFound = (search != INVALID_HANDLE_VALUE);

    FindClose(search);

    if (bIsFound && pFindData)
        *pFindData = find_data;

    return bIsFound;
}

LRESULT
FileQueryFiles(IN LPCWSTR Path,
               IN LPCWSTR FileNamesQuery,
               IN PQUERY_FILES_TABLE QueryTable,
               IN PVOID   Context)
{
    LRESULT          res = ERROR_SUCCESS;
    WIN32_FIND_DATAW find_data;

    LPWSTR lpszQuery;
    DWORD  dwNumOfChars;
    LPWSTR lpszExpandedQuery;
    HANDLE search;

    dwNumOfChars = wcslen(Path) + 1 + wcslen(FileNamesQuery) + 1;
    lpszQuery = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    wcscpy(lpszQuery, Path);
    wcscat(lpszQuery, L"\\");
    wcscat(lpszQuery, FileNamesQuery);

    dwNumOfChars      = ExpandEnvironmentStringsW(lpszQuery, NULL, 0);
    lpszExpandedQuery = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszQuery, lpszExpandedQuery, dwNumOfChars);
    MemFree(lpszQuery);

    search = FindFirstFileW(lpszExpandedQuery, &find_data);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            PQUERY_FILES_TABLE pTable = QueryTable;
            while (pTable && pTable->QueryRoutine)
            {
                pTable->QueryRoutine(Path, FileNamesQuery, lpszExpandedQuery, &find_data, Context, pTable->EntryContext);
                ++pTable;
            }
        } while (/*res = */ FindNextFileW(search, &find_data));
    }
    else
        res = ERROR_NO_MORE_FILES;

    FindClose(search);

    MemFree(lpszExpandedQuery);

    return res;
}

BOOL BackupIniFile(IN LPCWSTR lpszIniFile)
{
    BOOL Success = FALSE;
    DWORD dwNumOfChars = 0;
    LPWSTR SourceFile, DestFile;
    LPWSTR lpName, lpPath;

    dwNumOfChars = ExpandEnvironmentStringsW(lpszIniFile, NULL, 0);
    SourceFile = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszIniFile, SourceFile, dwNumOfChars);

    lpName = wcsrchr(SourceFile, L'\\');
    lpName = (lpName ? lpName + 1 : SourceFile);
    dwNumOfChars = wcslen(L"%SystemRoot%\\pss\\") + wcslen(lpName) + 7 + 1;
    lpPath = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    wcscpy(lpPath, L"%SystemRoot%\\pss\\");
    wcscat(lpPath, lpName);
    wcscat(lpPath, L".backup");

    dwNumOfChars = ExpandEnvironmentStringsW(lpPath, NULL, 0);
    DestFile = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpPath, DestFile, dwNumOfChars);
    MemFree(lpPath);

    Success = CopyFileW(SourceFile, DestFile, TRUE /* don't overwrite */ /* FALSE */ /* overwrite */);

    MemFree(DestFile);
    MemFree(SourceFile);

    return Success;
}
