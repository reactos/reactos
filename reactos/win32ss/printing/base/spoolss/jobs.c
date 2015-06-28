/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return LocalSplFuncs.fpAddJob(hPrinter, Level, pData, cbBuf, pcbNeeded);
}

BOOL WINAPI
EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    return LocalSplFuncs.fpEnumJobs(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);
}

BOOL WINAPI
GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return LocalSplFuncs.fpGetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
}

BOOL WINAPI
ScheduleJob(HANDLE hPrinter, DWORD dwJobID)
{
    return LocalSplFuncs.fpScheduleJob(hPrinter, dwJobID);
}

BOOL WINAPI
SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    return LocalSplFuncs.fpSetJob(hPrinter, JobId, Level, pJobInfo, Command);
}
