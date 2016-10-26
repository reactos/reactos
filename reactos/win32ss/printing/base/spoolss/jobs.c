/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
AddJobW(HANDLE hPrinter, DWORD Level, PBYTE pData, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpAddJob(pHandle->hPrinter, Level, pData, cbBuf, pcbNeeded);
}

BOOL WINAPI
EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEnumJobs(pHandle->hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned);
}

BOOL WINAPI
GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJob, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetJob(pHandle->hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
}

BOOL WINAPI
ScheduleJob(HANDLE hPrinter, DWORD dwJobID)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpScheduleJob(pHandle->hPrinter, dwJobID);
}

BOOL WINAPI
SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpSetJob(pHandle->hPrinter, JobId, Level, pJobInfo, Command);
}
