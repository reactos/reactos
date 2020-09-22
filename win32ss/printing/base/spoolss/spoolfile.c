/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Spool File
 * COPYRIGHT:   Copyright 1998-2022 ReactOS
 */

#include "precomp.h"

BOOL WINAPI
SplGetSpoolFileInfo(
    HANDLE hPrinter,
    HANDLE hProcessHandle,
    DWORD Level,
    PFILE_INFO_1 pFileInfo,
    DWORD dwSize,
    DWORD* dwNeeded )
{
    BOOL Ret;
    HANDLE hHandle, hSourceProcessHandle;
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hSourceProcessHandle = GetCurrentProcess();

    // No Local? Ok, what ever...

    Ret = pHandle->pPrintProvider->PrintProvider.fpGetSpoolFileInfo( pHandle->hPrinter,
                                                                     NULL,
                                                                    &hHandle,
                                                                     hProcessHandle,
                                                                     hSourceProcessHandle );
    if ( Ret )
    {
        pFileInfo->hSpoolFileHandle = hHandle;
        pFileInfo->bInheritHandle   = TRUE;
        pFileInfo->dwOptions        = DUPLICATE_CLOSE_SOURCE;
    }

    return Ret;
}

BOOL WINAPI
SplCommitSpoolData(
    HANDLE hPrinter,
    HANDLE hProcessHandle,
    DWORD cbCommit,
    DWORD Level,
    PFILE_INFO_1 pFileInfo,
    DWORD dwSize,
    DWORD* dwNeeded )
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pFileInfo->hSpoolFileHandle = INVALID_HANDLE_VALUE;
    pFileInfo->bInheritHandle   = TRUE;
    pFileInfo->dwOptions        = DUPLICATE_CLOSE_SOURCE;

    return pHandle->pPrintProvider->PrintProvider.fpCommitSpoolData( hPrinter, cbCommit );
}

BOOL WINAPI
SplCloseSpoolFileHandle( HANDLE hPrinter )
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpCloseSpoolFileHandle( pHandle->hPrinter );
}
