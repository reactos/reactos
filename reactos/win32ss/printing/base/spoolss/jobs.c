/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
AddJob(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return LocalSplFuncs.fpAddJob(hPrinter, Level, pData, cbBuf, pcbNeeded);
}

BOOL WINAPI
GetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return LocalSplFuncs.fpGetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
}
