/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Spool Files and printing
 * COPYRIGHT:   Copyright 1998-2020 ReactOS
 */

#include "precomp.h"

BOOL WINAPI
LocalGetSpoolFileInfo(
    HANDLE hPrinter,
    LPWSTR *pSpoolDir,
    LPHANDLE phFile,
    HANDLE hSpoolerProcess,
    HANDLE hAppProcess )
{
    FIXME("LocalGetSpoolFileInfo(%p, %S, %p, %p, %p)\n", hPrinter, pSpoolDir, phFile, hSpoolerProcess, hAppProcess);
    return FALSE;
}

BOOL WINAPI
LocalCommitSpoolData( HANDLE hPrinter, DWORD cbCommit)
{
    FIXME("LocalCommitSpoolData(%p, %lu)\n", hPrinter, cbCommit);
    return FALSE;
}

BOOL WINAPI
LocalCloseSpoolFileHandle( HANDLE hPrinter)
{
    FIXME("LocalCloseSpoolFileHandle(%p)\n", hPrinter);
    return FALSE;
}

