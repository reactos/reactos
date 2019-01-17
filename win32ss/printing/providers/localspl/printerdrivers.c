/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for printer driver information
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"


// Local Constants
static DWORD dwDriverInfo1Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_1W, pName),
    MAXDWORD
};

static DWORD dwDriverInfo2Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_2W, pName),
    FIELD_OFFSET(DRIVER_INFO_2W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_2W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_2W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_2W, pConfigFile),
    MAXDWORD
};

static DWORD dwDriverInfo3Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_3W, pName),
    FIELD_OFFSET(DRIVER_INFO_3W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_3W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_3W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pConfigFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pHelpFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pDependentFiles),
    FIELD_OFFSET(DRIVER_INFO_3W, pMonitorName),
    FIELD_OFFSET(DRIVER_INFO_3W, pDefaultDataType),
    MAXDWORD
};

static void
ToMultiSz(LPWSTR pString)
{
    while (*pString)
    {
        if (*pString == '|')
            *pString = '\0';
        pString++;
    }
}


static void
_LocalGetPrinterDriverLevel1(PLOCAL_PRINTER_HANDLE pHandle, PDRIVER_INFO_1W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[1];

    /* This value is only here to send something, I have not verified if it is actually correct */
    pwszStrings[0] = pHandle->pPrinter->pwszPrinterDriver;

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
        }

        *pcbNeeded += sizeof(DRIVER_INFO_1W);
        return;
    }


    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo1Offsets, *ppDriverInfoEnd);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel2(PLOCAL_PRINTER_HANDLE pHandle, PDRIVER_INFO_2W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[5];

    /* Clearly these things should not be hardcoded, so when it is needed, someone can add meaningfull values here */
    pwszStrings[0] = pHandle->pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = L"c:\\reactos\\system32\\localspl.dll";  // pDriverPath
    pwszStrings[3] = L"c:\\reactos\\system32\\localspl.dll";  // pDataFile
    pwszStrings[4] = L"c:\\reactos\\system32\\localspl.dll";  // pConfigFile

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
        }

        *pcbNeeded += sizeof(DRIVER_INFO_2W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo2Offsets, *ppDriverInfoEnd);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel3(PLOCAL_PRINTER_HANDLE pHandle, PDRIVER_INFO_3W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[9];

    /* Clearly these things should not be hardcoded, so when it is needed, someone can add meaningfull values here */
    pwszStrings[0] = pHandle->pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = L"c:\\reactos\\system32\\localspl.dll";  // pDriverPath
    pwszStrings[3] = L"c:\\reactos\\system32\\localspl.dll";  // pDataFile
    pwszStrings[4] = L"c:\\reactos\\system32\\printui.dll";  // pConfigFile
    pwszStrings[5] = L"";  // pHelpFile
    pwszStrings[6] = L"localspl.dll|printui.dll|";  // pDependentFiles, | is separator and terminator!
    pwszStrings[7] = NULL;  // pMonitorName
    pwszStrings[8] = NULL;  // pDefaultDataType


    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_3W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo3Offsets, *ppDriverInfoEnd);
    ToMultiSz((*ppDriverInfo)->pDependentFiles);
    (*ppDriverInfo)++;
}


BOOL WINAPI LocalGetPrinterDriver(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pEnd = &pDriverInfo[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    TRACE("LocalGetPrinterDriver(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Only support 3 levels for now
    if (Level > 3)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;

    if (Level == 1)
        _LocalGetPrinterDriverLevel1(pPrinterHandle, NULL, NULL, pcbNeeded);
    else if (Level == 2)
        _LocalGetPrinterDriverLevel2(pPrinterHandle, NULL, NULL, pcbNeeded);
    else if (Level == 3)
        _LocalGetPrinterDriverLevel3(pPrinterHandle, NULL, NULL, pcbNeeded);

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the information.
    pEnd = &pDriverInfo[*pcbNeeded];

    if (Level == 1)
        _LocalGetPrinterDriverLevel1(pPrinterHandle, (PDRIVER_INFO_1W*)&pDriverInfo, &pEnd, NULL);
    else if (Level == 2)
        _LocalGetPrinterDriverLevel2(pPrinterHandle, (PDRIVER_INFO_2W*)&pDriverInfo, &pEnd, NULL);
    else if (Level == 3)
        _LocalGetPrinterDriverLevel3(pPrinterHandle, (PDRIVER_INFO_3W*)&pDriverInfo, &pEnd, NULL);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
