/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS pending moves operations Information tool
 * FILE:            cmdutils/pendmoves/pendmoves.c
 * PURPOSE:         Query information from registry about pending moves
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

static
TCHAR *
BeautifyPath(TCHAR * Path, DWORD * Len)
{
    DWORD LocalLen = *Len;

    /* If there's a ! marking that existing file can be overwritten,
     * drop it
     */
    if (LocalLen > 1)
    {
        if (Path[0] == _T('!'))
        {
            ++Path;
            --LocalLen;
        }
    }

    /* Remove namespace if prefixed */
    if (LocalLen > 4)
    {
        if (Path[0] == _T('\\') && Path[1] == _T('?') &&
            Path[2] == _T('?') && Path[3] == _T('\\'))
        {
            Path += 4;
            LocalLen -= 4;
        }
    }

    /* Return modified string + len */
    *Len = LocalLen;
    return Path;
}

static
DWORD
DisplayPendingOps(TCHAR * Value, DWORD Len)
{
    DWORD Chars, i, j, Count, SrcLen, TgtLen;
    TCHAR * SrcFile, * Target, * Current;

    /* Compute the amount of chars
     * NULL char isn't relaible EOF (MULTI_SZ)
     */
    Chars = Len / sizeof(TCHAR);

    i = 0;
    Count = 0;
    Current = Value;
    /* Browse the whole string */
    while (i < Chars)
    {
        /* Jump to the next NULL (end of source) */
        for (j = i; j < Chars && Value[j] != 0; ++j);
        /* Get len & clean path */
        SrcLen = _tcslen(Current);
        SrcFile = BeautifyPath(Current, &SrcLen);
        /* Source file is null - likely the end of the MULTI_SZ, quit */
        if (SrcLen == 0)
        {
            break;
        }

        /* Remember position, jump to the begin of the target */
        i = j;
        ++i;
        /* Update position in MULTI_SZ */
        Current = Value + i;

        /* Jump to the next NULL (end of target) */
        for (j = i; j < Chars && Value[j] != 0; ++j);
        /* Get len & clean path */
        TgtLen = _tcslen(Current);
        Target = BeautifyPath(Current, &TgtLen);
        /* Remember position, jump to the begin of the next source */
        i = j;
        ++i;
        Current = Value + i;

        /* Display source */
        _ftprintf(stdout, _T("Source: %s\n"), SrcFile);
        /* If is accessible? Warn if not */
        if (GetFileAttributes(SrcFile) == INVALID_FILE_ATTRIBUTES)
        {
            _ftprintf(stdout, _T("\t *** Source file lookup error: %d\n"), GetLastError());
        }
        /* And display target - if empty, it's for deletion, mark as it */
        _ftprintf(stdout, _T("Target: %s\n\n"), (_tcslen(Target) != 0 ? Target: _T("DELETE")));

        /* Remember position and number of entries */
        Current = Value + i;
        ++Count;
    }

    return Count;
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    HKEY hKey;
    LONG Ret;
    DWORD MaxLen, Len, Count, Type;
    PVOID Buffer;
    FILETIME LastModified;
    TCHAR RegistryPath[] = _T("System\\CurrentControlSet\\Control\\Session Manager");

    /* Open the SMSS registry key */
    Ret = RegOpenKey(HKEY_LOCAL_MACHINE, RegistryPath, &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        _ftprintf(stderr, _T("Failed opening the registry key '%s' (%lx)\n"), RegistryPath, Ret);
        return 1;
    }

    /* Get last modified date + buffer length we need to allocate */
    Ret = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &MaxLen, NULL, &LastModified);
    if (Ret != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        _ftprintf(stderr, _T("Failed querying information for '%s' (%lx)\n"), RegistryPath, Ret);
        return 1;
    }

    /* No value, so no operations */
    if (MaxLen == 0)
    {
        RegCloseKey(hKey);
        _ftprintf(stdout, _T("No pending file rename operations registered.\n\n"));
        return 0;
    }

    /* Allocate memory */
    Buffer = HeapAlloc(GetProcessHeap(), 0, MaxLen);
    if (Buffer == NULL)
    {
        RegCloseKey(hKey);
        _ftprintf(stderr, _T("Failed allocating %d bytes\n"), MaxLen);
        return 1;
    }

    /* Start with PendingFileRenameOperations */
    Count = 0;
    Len = MaxLen;
    Ret = RegQueryValueEx(hKey, _T("PendingFileRenameOperations"), NULL, &Type, Buffer, &Len);
    if (Ret == ERROR_SUCCESS && Type == REG_MULTI_SZ)
    {
        Count += DisplayPendingOps(Buffer, Len);
    }

    /* Continue with PendingFileRenameOperations2 - used if PendingFileRenameOperations is too big */
    Len = MaxLen;
    Ret = RegQueryValueEx(hKey, _T("PendingFileRenameOperations2"), NULL, &Type, Buffer, &Len);
    if (Ret == ERROR_SUCCESS && Type == REG_MULTI_SZ)
    {
        Count += DisplayPendingOps(Buffer, Len);
    }

    /* Release everything */
    HeapFree(GetProcessHeap(), 0, Buffer);
    RegCloseKey(hKey);

    /* If we found entries, display modification date */
    if (Count != 0)
    {
        FILETIME LocalTime;
        SYSTEMTIME SysTime;

        /* Convert our UTC time to local time, and then to system time to allow easy display */
        if (FileTimeToLocalFileTime(&LastModified, &LocalTime) && FileTimeToSystemTime(&LocalTime, &SysTime))
        {
            _ftprintf(stdout, _T("Time of last update to pending moves key: %02d/%02d/%04d %02d:%02d\n\n"),
                      SysTime.wDay, SysTime.wMonth, SysTime.wYear, SysTime.wHour, SysTime.wMinute);
        }
    }
    /* No operations found */
    else
    {
        _ftprintf(stdout, _T("No pending file rename operations registered.\n\n"));
    }

    return 0;
}
