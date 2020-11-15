/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for managing print Forms
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

BOOL WINAPI
AddFormW(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpAddForm(pHandle->hPrinter, Level, pForm);
}

BOOL WINAPI
DeleteFormW(HANDLE hPrinter, PWSTR pFormName)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpDeleteForm(pHandle->hPrinter, pFormName);
}

BOOL WINAPI
EnumFormsW(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( cbBuf && !pForm )
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEnumForms(pHandle->hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
}

BOOL WINAPI
GetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( cbBuf && pForm )
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetForm(pHandle->hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);
}

BOOL WINAPI
SetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpSetForm(pHandle->hPrinter, pFormName, Level, pForm);
}
