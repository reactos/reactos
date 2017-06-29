/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
EnumPrintProcessorDatatypesW(PWSTR pName, PWSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Sanity checks
    if (cbBuf && !pDatatypes)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    // Always call this function on the Local Spooler.
    pPrintProvider = CONTAINING_RECORD(PrintProviderList.Flink, SPOOLSS_PRINT_PROVIDER, Entry);
    return pPrintProvider->PrintProvider.fpEnumPrintProcessorDatatypes(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
}

BOOL WINAPI
EnumPrintProcessorsW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    // Always call this function on the Local Spooler.
    PSPOOLSS_PRINT_PROVIDER pPrintProvider = CONTAINING_RECORD(PrintProviderList.Flink, SPOOLSS_PRINT_PROVIDER, Entry);
    return pPrintProvider->PrintProvider.fpEnumPrintProcessors(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
}

BOOL WINAPI
GetPrintProcessorDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Sanity checks
    if (cbBuf && !pPrintProcessorInfo)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    // Always call this function on the Local Spooler.
    pPrintProvider = CONTAINING_RECORD(PrintProviderList.Flink, SPOOLSS_PRINT_PROVIDER, Entry);
    return pPrintProvider->PrintProvider.fpGetPrintProcessorDirectory(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);
}
