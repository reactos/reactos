/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
RTL_GENERIC_TABLE PrinterTable;


/**
 * @name _PrinterTableCompareRoutine
 *
 * RTL_GENERIC_COMPARE_ROUTINE for the Printer Table.
 * Does a case-insensitive comparison, because e.g. LocalOpenPrinter doesn't match the case when looking for Printers.
 */
static RTL_GENERIC_COMPARE_RESULTS NTAPI
_PrinterTableCompareRoutine(PRTL_GENERIC_TABLE Table, PVOID FirstStruct, PVOID SecondStruct)
{
    PLOCAL_PRINTER A = (PLOCAL_PRINTER)FirstStruct;
    PLOCAL_PRINTER B = (PLOCAL_PRINTER)SecondStruct;

    int iResult = wcsicmp(A->pwszPrinterName, B->pwszPrinterName);

    if (iResult < 0)
        return GenericLessThan;
    else if (iResult > 0)
        return GenericGreaterThan;
    else
        return GenericEqual;
}

/**
 * @name InitializePrinterTable
 *
 * Initializes a RTL_GENERIC_TABLE of locally available Printers.
 * The table is searchable by name and returns information about the printers, including their job queues.
 * During this process, the job queues are also initialized.
 */
void
InitializePrinterTable()
{
    const WCHAR wszPrintersKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers";

    DWORD cbDevMode;
    DWORD cchPrinterName;
    DWORD dwSubKeys;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    LONG lStatus;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PLOCAL_PRINTER pPrinter = NULL;
    PWSTR pwszPrintProcessor = NULL;
    WCHAR wszPrinterName[MAX_PRINTER_NAME + 1];

    // Initialize an empty table for our printers.
    // We will search it by printer name.
    RtlInitializeGenericTable(&PrinterTable, _PrinterTableCompareRoutine, GenericTableAllocateRoutine, GenericTableFreeRoutine, NULL);

    // Open our printers registry key. Each subkey is a local printer there.
    lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszPrintersKey, 0, KEY_READ, &hKey);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Get the number of subkeys.
    lStatus = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Loop through all available local printers.
    for (i = 0; i < dwSubKeys; i++)
    {
        // Cleanup tasks from the previous run
        if (hSubKey)
        {
            RegCloseKey(hSubKey);
            hSubKey = NULL;
        }

        if (pPrinter)
        {
            if (pPrinter->pwszDefaultDatatype)
                DllFreeSplStr(pPrinter->pwszDefaultDatatype);

            DllFreeSplMem(pPrinter);
            pPrinter = NULL;
        }

        if (pwszPrintProcessor)
        {
            DllFreeSplStr(pwszPrintProcessor);
            pwszPrintProcessor = NULL;
        }

        // Get the name of this printer.
        cchPrinterName = _countof(wszPrinterName);
        lStatus = RegEnumKeyExW(hKey, i, wszPrinterName, &cchPrinterName, NULL, NULL, NULL, NULL);
        if (lStatus == ERROR_MORE_DATA)
        {
            // This printer name exceeds the maximum length and is invalid.
            continue;
        }
        else if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed for iteration %lu with status %ld!\n", i, lStatus);
            continue;
        }

        // Open this Printer's registry key.
        lStatus = RegOpenKeyExW(hKey, wszPrinterName, 0, KEY_READ, &hSubKey);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Printer \"%S\" with status %ld!\n", wszPrinterName, lStatus);
            continue;
        }

        // Get the Print Processor.
        pwszPrintProcessor = AllocAndRegQueryWSZ(hSubKey, L"Print Processor");
        if (!pwszPrintProcessor)
            continue;

        // Try to find it in the Print Processor Table.
        pPrintProcessor = RtlLookupElementGenericTable(&PrintProcessorTable, pwszPrintProcessor);
        if (!pPrintProcessor)
        {
            ERR("Invalid Print Processor \"%S\" for Printer \"%S\"!\n", pwszPrintProcessor, wszPrinterName);
            continue;
        }

        // Create a new LOCAL_PRINTER structure for it.
        pPrinter = DllAllocSplMem(sizeof(LOCAL_PRINTER));
        if (!pPrinter)
        {
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pPrinter->pwszPrinterName = AllocSplStr(wszPrinterName);
        pPrinter->pPrintProcessor = pPrintProcessor;
        InitializeListHead(&pPrinter->JobQueue);

        // Get the printer driver.
        pPrinter->pwszPrinterDriver = AllocAndRegQueryWSZ(hSubKey, L"Printer Driver");
        if (!pPrinter->pwszPrinterDriver)
            continue;

        // Get the description.
        pPrinter->pwszDescription = AllocAndRegQueryWSZ(hSubKey, L"Description");
        if (!pPrinter->pwszDescription)
            continue;

        // Get the default datatype.
        pPrinter->pwszDefaultDatatype = AllocAndRegQueryWSZ(hSubKey, L"Datatype");
        if (!pPrinter->pwszDefaultDatatype)
            continue;

        // Verify that it's valid.
        if (!RtlLookupElementGenericTable(&pPrintProcessor->DatatypeTable, pPrinter->pwszDefaultDatatype))
        {
            ERR("Invalid default datatype \"%S\" for Printer \"%S\"!\n", pPrinter->pwszDefaultDatatype, wszPrinterName);
            continue;
        }

        // Get the default DevMode.
        cbDevMode = sizeof(DEVMODEW);
        lStatus = RegQueryValueExW(hSubKey, L"Default DevMode", NULL, NULL, (PBYTE)&pPrinter->DefaultDevMode, &cbDevMode);
        if (lStatus != ERROR_SUCCESS || cbDevMode != sizeof(DEVMODEW))
        {
            ERR("Couldn't query DevMode for Printer \"%S\", status is %ld, cbDevMode is %lu!\n", wszPrinterName, lStatus, cbDevMode);
            continue;
        }

        // Add this printer to the printer table.
        if (!RtlInsertElementGenericTable(&PrinterTable, pPrinter, sizeof(LOCAL_PRINTER), NULL))
        {
            ERR("RtlInsertElementGenericTable failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        // Don't let the cleanup routines free this.
        pPrinter = NULL;
    }

Cleanup:
    if (pwszPrintProcessor)
        DllFreeSplStr(pwszPrintProcessor);

    if (pPrinter)
    {
        if (pPrinter->pwszDefaultDatatype)
            DllFreeSplStr(pPrinter->pwszDefaultDatatype);

        DllFreeSplMem(pPrinter);
    }

    if (hSubKey)
        RegCloseKey(hSubKey);

    if (hKey)
        RegCloseKey(hKey);
}


BOOL
_LocalEnumPrintersLevel1(DWORD Flags, LPWSTR Name, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    const WCHAR wszComma[] = L",";

    DWORD cbName;
    DWORD cbComment;
    DWORD cbDescription;
    DWORD cchComputerName = 0;
    DWORD i;
    PBYTE pPrinterInfo;
    PBYTE pPrinterString;
    PLOCAL_PRINTER pPrinter;
    PRINTER_INFO_1W PrinterInfo1;
    PVOID pRestartKey = NULL;
    WCHAR wszComputerName[2 + MAX_COMPUTERNAME_LENGTH + 1 + 1];

    DWORD dwOffsets[] = {
        FIELD_OFFSET(PRINTER_INFO_1W, pName),
        FIELD_OFFSET(PRINTER_INFO_1W, pDescription),
        FIELD_OFFSET(PRINTER_INFO_1W, pComment),
        MAXDWORD
    };

    if (Flags & PRINTER_ENUM_NAME)
    {
        if (Name)
        {
            // The user supplied a Computer Name (with leading double backslashes) or Print Provider Name.
            // Only process what's directed at us and dismiss every other request with ERROR_INVALID_NAME.
            if (Name[0] == L'\\' && Name[1] == L'\\')
            {
                // Prepend slashes to the computer name.
                wszComputerName[0] = L'\\';
                wszComputerName[1] = L'\\';

                // Get the local computer name for comparison.
                cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
                if (!GetComputerNameW(&wszComputerName[2], &cchComputerName))
                {
                    ERR("GetComputerNameW failed with error %lu!\n", GetLastError());
                    return FALSE;
                }

                // Add the leading slashes to the total length.
                cchComputerName += 2;

                // Now compare this with the local computer name and reject if it doesn't match.
                if (wcsicmp(&Name[2], &wszComputerName[2]) != 0)
                {
                    SetLastError(ERROR_INVALID_NAME);
                    return FALSE;
                }

                // Add a trailing backslash to wszComputerName, which will later be prepended in front of the printer names.
                wszComputerName[cchComputerName++] = L'\\';
                wszComputerName[cchComputerName] = 0;
            }
            else if (wcsicmp(Name, wszPrintProviderInfo[0]) != 0)
            {
                // The user supplied a name that cannot be processed by the local print provider.
                SetLastError(ERROR_INVALID_NAME);
                return FALSE;
            }
        }
        else
        {
            // The caller wants information about this Print Provider.
            // spoolss packs this into an array of information about all Print Providers.
            *pcbNeeded = sizeof(PRINTER_INFO_1W);

            for (i = 0; i < 3; i++)
                *pcbNeeded += (wcslen(wszPrintProviderInfo[i]) + 1) * sizeof(WCHAR);

            *pcReturned = 1;

            // Check if the supplied buffer is large enough.
            if (cbBuf < *pcbNeeded)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            // Copy over the print processor information.
            ((PPRINTER_INFO_1W)pPrinterEnum)->Flags = 0;
            PackStrings(wszPrintProviderInfo, pPrinterEnum, dwOffsets, &pPrinterEnum[*pcbNeeded]);
            return TRUE;
        }
    }

    // Count the required buffer size and the number of printers.
    for (pPrinter = RtlEnumerateGenericTableWithoutSplaying(&PrinterTable, &pRestartKey); pPrinter; pPrinter = RtlEnumerateGenericTableWithoutSplaying(&PrinterTable, &pRestartKey))
    {
        // This looks wrong, but is totally right. PRINTER_INFO_1W has three members pName, pComment and pDescription.
        // But pComment equals the "Description" registry value while pDescription is concatenated out of pName and pComment.
        // On top of this, the computer name is prepended to the printer name if the user supplied the local computer name during the query.
        cbName = (wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        cbDescription = cchComputerName * sizeof(WCHAR) + cbName + cbComment + sizeof(WCHAR);

        *pcbNeeded += sizeof(PRINTER_INFO_1W) + cchComputerName * sizeof(WCHAR) + cbName + cbComment + cbDescription;
        *pcReturned++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Put the strings right after the last PRINTER_INFO_1W structure.
    // Due to all the required string processing, we can't just use PackStrings here :(
    pPrinterInfo = pPrinterEnum;
    pPrinterString = pPrinterEnum + *pcReturned * sizeof(PRINTER_INFO_1W);

    // Copy over the printer information.
    for (pPrinter = RtlEnumerateGenericTableWithoutSplaying(&PrinterTable, &pRestartKey); pPrinter; pPrinter = RtlEnumerateGenericTableWithoutSplaying(&PrinterTable, &pRestartKey))
    {
        // FIXME: As for now, the Flags member returns no information.
        PrinterInfo1.Flags = 0;

        // Copy the printer name.
        PrinterInfo1.pName = (PWSTR)pPrinterString;
        CopyMemory(pPrinterString, wszComputerName, cchComputerName * sizeof(WCHAR));
        pPrinterString += cchComputerName * sizeof(WCHAR);
        cbName = (wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
        CopyMemory(pPrinterString, pPrinter->pwszPrinterName, cbName);
        pPrinterString += cbName;

        // Copy the printer comment (equals the "Description" registry value).
        PrinterInfo1.pComment = (PWSTR)pPrinterString;
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        CopyMemory(pPrinterString, pPrinter->pwszDescription, cbComment);
        pPrinterString += cbComment;

        // Copy the description, which for PRINTER_INFO_1W has the form "Name,Comment,"
        PrinterInfo1.pDescription = (PWSTR)pPrinterString;
        CopyMemory(pPrinterString, wszComputerName, cchComputerName * sizeof(WCHAR));
        pPrinterString += cchComputerName * sizeof(WCHAR);
        CopyMemory(pPrinterString, pPrinter->pwszPrinterName, cbName - sizeof(WCHAR));
        pPrinterString += cbName - sizeof(WCHAR);
        CopyMemory(pPrinterString, wszComma, sizeof(WCHAR));
        pPrinterString += sizeof(WCHAR);
        CopyMemory(pPrinterString, pPrinter->pwszDescription, cbComment - sizeof(WCHAR));
        pPrinterString += cbComment - sizeof(WCHAR);
        CopyMemory(pPrinterString, wszComma, sizeof(wszComma));
        pPrinterString += sizeof(wszComma);
                
        // Finally copy the structure and advance to the next one in the output buffer.
        CopyMemory(pPrinterInfo, &PrinterInfo1, sizeof(PRINTER_INFO_1W));
        pPrinterInfo += sizeof(PRINTER_INFO_1W);
    }

    return TRUE;
}

BOOL WINAPI
LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    // Do no sanity checks here. This is verified by localspl_apitest!

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // Think positive :)
    SetLastError(ERROR_SUCCESS);

    if (Flags & PRINTER_ENUM_LOCAL)
    {
        // The function behaves quite differently for each level.
        if (Level == 1)
            return _LocalEnumPrintersLevel1(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);

        // TODO: Handle other levels.

        // The caller supplied an invalid level.
        return FALSE;
    }

    // Treat it as success if the caller queried no information and we don't need to return any.
    return TRUE;
}

BOOL WINAPI
LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    BOOL bReturnValue = ROUTER_UNKNOWN;
    DWORD cchComputerName;
    DWORD cchPrinterName;
    DWORD dwJobID;
    PWSTR p = lpPrinterName;
    PWSTR pwszPrinterName = NULL;
    PLOCAL_JOB pJob;
    PLOCAL_HANDLE pHandle;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle = NULL;
    PLIST_ENTRY pEntry;
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    // Sanity checks
    if (!lpPrinterName || !phPrinter)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    // Does lpPrinterName begin with two backslashes to indicate a server name?
    if (lpPrinterName[0] == L'\\' && lpPrinterName[1] == L'\\')
    {
        // Skip these two backslashes.
        lpPrinterName += 2;

        // Look for the closing backslash.
        p = wcschr(lpPrinterName, L'\\');
        if (!p)
        {
            // We didn't get a proper server name.
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            goto Cleanup;
        }

        // Null-terminate the string here to enable comparison.
        *p = 0;

        // Get the local computer name for comparison.
        cchComputerName = _countof(wszComputerName);
        if (!GetComputerNameW(wszComputerName, &cchComputerName))
        {
            ERR("GetComputerNameW failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        // Now compare this with the local computer name and reject if it doesn't match, because this print provider only supports local printers.
        if (wcsicmp(lpPrinterName, wszComputerName) != 0)
        {
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            goto Cleanup;
        }

        // We have checked the server name and don't need it anymore.
        lpPrinterName = p + 1;
    }

    // Look for a comma. If it exists, it indicates the end of the printer name.
    p = wcschr(lpPrinterName, L',');
    if (p)
        cchPrinterName = p - lpPrinterName;
    else
        cchPrinterName = wcslen(lpPrinterName);

    // No printer name and no comma? This is invalid!
    if (!cchPrinterName && !p)
    {
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        goto Cleanup;
    }

    // Do we have a printer name?
    if (cchPrinterName)
    {
        // Yes, extract it.
        pwszPrinterName = DllAllocSplMem((cchPrinterName + 1) * sizeof(WCHAR));
        CopyMemory(pwszPrinterName, lpPrinterName, cchPrinterName * sizeof(WCHAR));
        pwszPrinterName[cchPrinterName] = 0;

        // Retrieve the associated printer from the table.
        pPrinter = RtlLookupElementGenericTable(&PrinterTable, pwszPrinterName);
        if (!pPrinter)
        {
            // The printer does not exist.
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            goto Cleanup;
        }

        // Create a new printer handle.
        pPrinterHandle = DllAllocSplMem(sizeof(LOCAL_PRINTER_HANDLE));
        pPrinterHandle->Printer = pPrinter;

        // Check if a datatype was given.
        if (pDefault && pDefault->pDatatype)
        {
            // Use the datatype if it's valid.
            if (!RtlLookupElementGenericTable(&pPrinter->pPrintProcessor->DatatypeTable, pDefault->pDatatype))
            {
                SetLastError(ERROR_INVALID_DATATYPE);
                goto Cleanup;
            }

            pPrinterHandle->pwszDatatype = AllocSplStr(pDefault->pDatatype);
        }
        else
        {
            // Use the default datatype.
            pPrinterHandle->pwszDatatype = AllocSplStr(pPrinter->pwszDefaultDatatype);
        }

        // Check if a DevMode was given, otherwise use the default.
        if (pDefault && pDefault->pDevMode)
            CopyMemory(&pPrinterHandle->DevMode, pDefault->pDevMode, sizeof(DEVMODEW));
        else
            CopyMemory(&pPrinterHandle->DevMode, &pPrinter->DefaultDevMode, sizeof(DEVMODEW));

        // Did we have a comma? Then the user may want a handle to an existing job instead of creating a new job.
        if (p)
        {
            ++p;
            
            // Skip whitespace.
            do
            {
                ++p;
            }
            while (*p == ' ');

            // The "Job " string has to follow now.
            if (wcscmp(p, L"Job ") != 0)
            {
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                goto Cleanup;
            }

            // Skip the "Job " string. 
            p += sizeof("Job ") - 1;

            // Skip even more whitespace.
            while (*p == ' ')
                ++p;

            // Finally extract the desired Job ID.
            dwJobID = wcstoul(p, NULL, 10);
            if (!IS_VALID_JOB_ID(dwJobID))
            {
                // The user supplied an invalid Job ID.
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                goto Cleanup;
            }

            // Look for this job in the job queue of the printer.
            pEntry = pPrinter->JobQueue.Flink;

            for (;;)
            {
                if (pEntry == &pPrinter->JobQueue)
                {
                    // We have reached the end of the list without finding the desired Job ID.
                    SetLastError(ERROR_INVALID_PRINTER_NAME);
                    goto Cleanup;
                }

                // Get our job structure.
                pJob = CONTAINING_RECORD(pEntry, LOCAL_JOB, Entry);

                if (pJob->dwJobID == dwJobID)
                {
                    // We have found the desired job. Give the caller a printer handle referencing it.
                    pPrinterHandle->StartedJob = pJob;
                    break;
                }

                pEntry = pEntry->Flink;
            }
        }

        // Create a new handle that references a printer.
        pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
        pHandle->HandleType = Printer;
        pHandle->SpecificHandle = pPrinterHandle;
    }
    else
    {
        // No printer name, but we have a comma!
        // This may be a request to a XcvMonitor or XcvPort handle.
        ++p;

        // Skip whitespace.
        do
        {
            ++p;
        }
        while (*p == ' ');

        // Check if this is a request to a XcvMonitor.
        if (wcscmp(p, L"XcvMonitor ") == 0)
        {
            // Skip the "XcvMonitor " string. 
            p += sizeof("XcvMonitor ") - 1;

            ///////////// TODO /////////////////////
            pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
            pHandle->HandleType = Monitor;
            //pHandle->SpecificHandle = pMonitorHandle;
        }
        else if (wcscmp(p, L"XcvPort ") == 0)
        {
            // Skip the "XcvPort " string. 
            p += sizeof("XcvPort ") - 1;

            //////////// TODO //////////////////////
            pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
            pHandle->HandleType = Port;
            //pHandle->SpecificHandle = pPortHandle;
        }
        else
        {
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            goto Cleanup;
        }
    }

    *phPrinter = (HANDLE)pHandle;
    bReturnValue = ROUTER_SUCCESS;

    // Don't let the cleanup routines free this.
    pPrinterHandle = NULL;
    pwszPrinterName = NULL;

Cleanup:
    if (pPrinterHandle)
    {
        if (pPrinterHandle->pwszDatatype)
            DllFreeSplStr(pPrinterHandle->pwszDatatype);

        DllFreeSplMem(pPrinterHandle);
    }

    if (pwszPrinterName)
        DllFreeSplMem(pwszPrinterName);

    return bReturnValue;
}

DWORD WINAPI
LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    DWORD dwReturnValue = 0;
    PDOC_INFO_1W pDocumentInfo1;
    PLOCAL_HANDLE pHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    PLOCAL_JOB pJob;

    // Sanity checks
    if (!pDocInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!hPrinter)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->SpecificHandle;

    // Check if this is the right document information level.
    if (Level != 1)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return 0;
    }

    pDocumentInfo1 = (PDOC_INFO_1W)pDocInfo;

    // Create a new job.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    pJob->Printer = pPrinterHandle->Printer;

    // Check if a datatype was given.
    if (pDocumentInfo1->pDatatype)
    {
        // Use the datatype if it's valid.
        if (!RtlLookupElementGenericTable(&pJob->Printer->pPrintProcessor->DatatypeTable, pDocumentInfo1->pDatatype))
        {
            SetLastError(ERROR_INVALID_DATATYPE);
            goto Cleanup;
        }

        pJob->pwszDatatype = AllocSplStr(pDocumentInfo1->pDatatype);
    }
    else
    {
        // Use the printer handle datatype.
        pJob->pwszDatatype = AllocSplStr(pPrinterHandle->pwszDatatype);
    }

    // Copy over printer defaults.
    CopyMemory(&pJob->DevMode, &pPrinterHandle->DevMode, sizeof(DEVMODEW));

    // Copy over supplied information.
    if (pDocumentInfo1->pDocName)
        pJob->pwszDocumentName = AllocSplStr(pDocumentInfo1->pDocName);

    if (pDocumentInfo1->pOutputFile)
        pJob->pwszOutputFile = AllocSplStr(pDocumentInfo1->pOutputFile);

    // Enqueue the job.
    ///////////// TODO /////////////////////

Cleanup:
    if (pJob)
        DllFreeSplMem(pJob);

    return dwReturnValue;
}

BOOL WINAPI
LocalStartPagePrinter(HANDLE hPrinter)
{
    ///////////// TODO /////////////////////
    return FALSE;
}

BOOL WINAPI
LocalWritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten)
{
    ///////////// TODO /////////////////////
    return FALSE;
}

BOOL WINAPI
LocalEndPagePrinter(HANDLE hPrinter)
{
    ///////////// TODO /////////////////////
    return FALSE;
}

BOOL WINAPI
LocalEndDocPrinter(HANDLE hPrinter)
{
    ///////////// TODO /////////////////////
    return FALSE;
}

BOOL WINAPI
LocalClosePrinter(HANDLE hPrinter)
{
    PLOCAL_HANDLE pHandle;

    if (!hPrinter)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pHandle = (PLOCAL_HANDLE)hPrinter;

    ///////////// TODO /////////////////////
    /// Check the handle type, do thoroughful checks on all data fields and clean them.
    ////////////////////////////////////////

    DllFreeSplMem(pHandle);

    return TRUE;
}
