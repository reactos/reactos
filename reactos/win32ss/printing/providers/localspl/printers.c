/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
SKIPLIST PrinterList;


/**
 * @name _PrinterListCompareRoutine
 *
 * SKIPLIST_COMPARE_ROUTINE for the Printer List.
 * Does a case-insensitive comparison, because e.g. LocalOpenPrinter doesn't match the case when looking for Printers.
 */
static int WINAPI
_PrinterListCompareRoutine(PVOID FirstStruct, PVOID SecondStruct)
{
    PLOCAL_PRINTER A = (PLOCAL_PRINTER)FirstStruct;
    PLOCAL_PRINTER B = (PLOCAL_PRINTER)SecondStruct;

    return wcsicmp(A->pwszPrinterName, B->pwszPrinterName);
}

/**
 * @name InitializePrinterList
 *
 * Initializes a list of locally available Printers.
 * The list is searchable by name and returns information about the printers, including their job queues.
 * During this process, the job queues are also initialized.
 */
void
InitializePrinterList()
{
    const WCHAR wszPrintersKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers";

    DWORD cbData;
    DWORD cchPrinterName;
    DWORD dwSubKeys;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    LONG lStatus;
    PLOCAL_PRINTER pPrinter = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PWSTR pwszPrintProcessor = NULL;
    WCHAR wszPrinterName[MAX_PRINTER_NAME + 1];

    // Initialize an empty list for our printers.
    InitializeSkiplist(&PrinterList, DllAllocSplMem, _PrinterListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);

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

            if (pPrinter->pwszDescription)
                DllFreeSplStr(pPrinter->pwszDescription);

            if (pPrinter->pwszPrinterDriver)
                DllFreeSplStr(pPrinter->pwszPrinterDriver);

            if (pPrinter->pwszPrinterName)
                DllFreeSplStr(pPrinter->pwszPrinterName);

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

        // Try to find it in the Print Processor List.
        pPrintProcessor = FindPrintProcessor(pwszPrintProcessor);
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
        InitializePrinterJobList(pPrinter);

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
        if (!FindDatatype(pPrintProcessor, pPrinter->pwszDefaultDatatype))
        {
            ERR("Invalid default datatype \"%S\" for Printer \"%S\"!\n", pPrinter->pwszDefaultDatatype, wszPrinterName);
            continue;
        }

        // Get the default DevMode.
        cbData = sizeof(DEVMODEW);
        lStatus = RegQueryValueExW(hSubKey, L"Default DevMode", NULL, NULL, (PBYTE)&pPrinter->DefaultDevMode, &cbData);
        if (lStatus != ERROR_SUCCESS || cbData != sizeof(DEVMODEW))
        {
            ERR("Couldn't query a valid DevMode for Printer \"%S\", status is %ld, cbData is %lu!\n", wszPrinterName, lStatus, cbData);
            continue;
        }

        // Get the Attributes.
        cbData = sizeof(DWORD);
        lStatus = RegQueryValueExW(hSubKey, L"Attributes", NULL, NULL, (PBYTE)&pPrinter->dwAttributes, &cbData);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("Couldn't query Attributes for Printer \"%S\", status is %ld!\n", wszPrinterName, lStatus);
            continue;
        }

        // Get the Status.
        cbData = sizeof(DWORD);
        lStatus = RegQueryValueExW(hSubKey, L"Status", NULL, NULL, (PBYTE)&pPrinter->dwStatus, &cbData);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("Couldn't query Status for Printer \"%S\", status is %ld!\n", wszPrinterName, lStatus);
            continue;
        }

        // Add this printer to the printer list.
        if (!InsertElementSkiplist(&PrinterList, pPrinter))
        {
            ERR("InsertElementSkiplist failed for Printer \"%S\"!\n", pPrinter->pwszPrinterName);
            goto Cleanup;
        }

        // Don't let the cleanup routines free this.
        pPrinter = NULL;
    }

Cleanup:
    // Inside the loop
    if (hSubKey)
        RegCloseKey(hSubKey);

    if (pPrinter)
    {
        if (pPrinter->pwszDefaultDatatype)
            DllFreeSplStr(pPrinter->pwszDefaultDatatype);

        if (pPrinter->pwszDescription)
            DllFreeSplStr(pPrinter->pwszDescription);

        if (pPrinter->pwszPrinterDriver)
            DllFreeSplStr(pPrinter->pwszPrinterDriver);

        if (pPrinter->pwszPrinterName)
            DllFreeSplStr(pPrinter->pwszPrinterName);

        DllFreeSplMem(pPrinter);
    }

    if (pwszPrintProcessor)
        DllFreeSplStr(pwszPrintProcessor);

    // Outside the loop
    if (hKey)
        RegCloseKey(hKey);
}


DWORD
_LocalEnumPrintersLevel1(DWORD Flags, LPWSTR Name, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    const WCHAR wszComma[] = L",";

    DWORD cbName;
    DWORD cbComment;
    DWORD cbDescription;
    DWORD cchComputerName = 0;
    DWORD dwErrorCode;
    DWORD i;
    PBYTE pPrinterInfo;
    PBYTE pPrinterString;
    PSKIPLIST_NODE pNode;
    PLOCAL_PRINTER pPrinter;
    PRINTER_INFO_1W PrinterInfo1;
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
                    dwErrorCode = GetLastError();
                    ERR("GetComputerNameW failed with error %lu!\n", dwErrorCode);
                    goto Cleanup;
                }

                // Add the leading slashes to the total length.
                cchComputerName += 2;

                // Now compare this with the local computer name and reject if it doesn't match.
                if (wcsicmp(&Name[2], &wszComputerName[2]) != 0)
                {
                    dwErrorCode = ERROR_INVALID_NAME;
                    goto Cleanup;
                }

                // Add a trailing backslash to wszComputerName, which will later be prepended in front of the printer names.
                wszComputerName[cchComputerName++] = L'\\';
                wszComputerName[cchComputerName] = 0;
            }
            else if (wcsicmp(Name, wszPrintProviderInfo[0]) != 0)
            {
                // The user supplied a name that cannot be processed by the local print provider.
                dwErrorCode = ERROR_INVALID_NAME;
                goto Cleanup;
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
                dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
                goto Cleanup;
            }

            // Copy over the print processor information.
            ((PPRINTER_INFO_1W)pPrinterEnum)->Flags = 0;
            PackStrings(wszPrintProviderInfo, pPrinterEnum, dwOffsets, &pPrinterEnum[*pcbNeeded]);
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
    }

    // Count the required buffer size and the number of printers.
    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        // This looks wrong, but is totally right. PRINTER_INFO_1W has three members pName, pComment and pDescription.
        // But pComment equals the "Description" registry value while pDescription is concatenated out of pName and pComment.
        // On top of this, the computer name is prepended to the printer name if the user supplied the local computer name during the query.
        cbName = (wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        cbDescription = cchComputerName * sizeof(WCHAR) + cbName + cbComment + sizeof(WCHAR);

        *pcbNeeded += sizeof(PRINTER_INFO_1W) + cchComputerName * sizeof(WCHAR) + cbName + cbComment + cbDescription;
        (*pcReturned)++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the strings right after the last PRINTER_INFO_1W structure.
    // Due to all the required string processing, we can't just use PackStrings here :(
    pPrinterInfo = pPrinterEnum;
    pPrinterString = pPrinterEnum + *pcReturned * sizeof(PRINTER_INFO_1W);

    // Copy over the printer information.
    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

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

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

BOOL WINAPI
LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD dwErrorCode;

    // Do no sanity checks here. This is verified by localspl_apitest!

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // Think positive :)
    // Treat it as success if the caller queried no information and we don't need to return any.
    dwErrorCode = ERROR_SUCCESS;

    if (Flags & PRINTER_ENUM_LOCAL)
    {
        // The function behaves quite differently for each level.
        if (Level == 1)
        {
            dwErrorCode = _LocalEnumPrintersLevel1(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
        }
        else
        {
            // TODO: Handle other levels.
            // The caller supplied an invalid level.
            dwErrorCode = ERROR_INVALID_LEVEL;
            goto Cleanup;
        }
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    DWORD cchComputerName;
    DWORD cchPrinterName;
    DWORD dwErrorCode;
    DWORD dwJobID;
    PWSTR p = lpPrinterName;
    PWSTR pwszPrinterName = NULL;
    PLOCAL_JOB pJob;
    PLOCAL_HANDLE pHandle;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle = NULL;
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    // Sanity checks
    if (!lpPrinterName || !phPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
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
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        // Null-terminate the string here to enable comparison.
        *p = 0;

        // Get the local computer name for comparison.
        cchComputerName = _countof(wszComputerName);
        if (!GetComputerNameW(wszComputerName, &cchComputerName))
        {
            dwErrorCode = GetLastError();
            ERR("GetComputerNameW failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        // Now compare this with the local computer name and reject if it doesn't match, because this print provider only supports local printers.
        if (wcsicmp(lpPrinterName, wszComputerName) != 0)
        {
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
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
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;
        goto Cleanup;
    }

    // Do we have a printer name?
    if (cchPrinterName)
    {
        // Yes, extract it.
        pwszPrinterName = DllAllocSplMem((cchPrinterName + 1) * sizeof(WCHAR));
        CopyMemory(pwszPrinterName, lpPrinterName, cchPrinterName * sizeof(WCHAR));
        pwszPrinterName[cchPrinterName] = 0;

        // Retrieve the associated printer from the list.
        pPrinter = LookupElementSkiplist(&PrinterList, &pwszPrinterName, NULL);
        if (!pPrinter)
        {
            // The printer does not exist.
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        // Create a new printer handle.
        pPrinterHandle = DllAllocSplMem(sizeof(LOCAL_PRINTER_HANDLE));
        pPrinterHandle->pPrinter = pPrinter;

        // Check if a datatype was given.
        if (pDefault && pDefault->pDatatype)
        {
            // Use the datatype if it's valid.
            if (!FindDatatype(pPrinter->pPrintProcessor, pDefault->pDatatype))
            {
                dwErrorCode = ERROR_INVALID_DATATYPE;
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
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
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
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
                goto Cleanup;
            }

            // Look for this job in the Global Job List.
            pJob = LookupElementSkiplist(&GlobalJobList, &dwJobID, NULL);
            if (!pJob || pJob->pPrinter != pPrinter)
            {
                // The user supplied a non-existing Job ID or the Job ID does not belong to the supplied printer name.
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
                goto Cleanup;
            }

            pPrinterHandle->pStartedJob = pJob;
        }

        // Create a new handle that references a printer.
        pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
        pHandle->HandleType = Printer;
        pHandle->pSpecificHandle = pPrinterHandle;
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
            //pHandle->pSpecificHandle = pMonitorHandle;
        }
        else if (wcscmp(p, L"XcvPort ") == 0)
        {
            // Skip the "XcvPort " string. 
            p += sizeof("XcvPort ") - 1;

            //////////// TODO //////////////////////
            pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
            pHandle->HandleType = Port;
            //pHandle->pSpecificHandle = pPortHandle;
        }
        else
        {
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }
    }

    *phPrinter = (HANDLE)pHandle;
    dwErrorCode = ERROR_SUCCESS;

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

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

DWORD WINAPI
LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    DWORD dwErrorCode;
    DWORD dwReturnValue = 0;
    PDOC_INFO_1W pDocumentInfo1;
    PLOCAL_HANDLE pHandle;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks
    if (!pDocInfo)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!hPrinter)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Check if this is the right document information level.
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    pDocumentInfo1 = (PDOC_INFO_1W)pDocInfo;

    // Create a new job.
    pJob = DllAllocSplMem(sizeof(LOCAL_JOB));
    pJob->pPrinter = pPrinterHandle->pPrinter;
    pJob->dwPriority = DEF_PRIORITY;

    // Check if a datatype was given.
    if (pDocumentInfo1->pDatatype)
    {
        // Use the datatype if it's valid.
        if (!FindDatatype(pJob->pPrintProcessor, pDocumentInfo1->pDatatype))
        {
            dwErrorCode = ERROR_INVALID_DATATYPE;
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

    // Get an ID for the new job.
    if (!GetNextJobID(&pJob->dwJobID))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Add the job to the Global Job List.
    if (!InsertElementSkiplist(&GlobalJobList, pJob))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("InsertElementSkiplist failed for job %lu for the GlobalJobList!\n", pJob->dwJobID);
        goto Cleanup;
    }

    // Add the job at the end of the Printer's Job List.
    // As all new jobs are created with default priority, we can be sure that it would always be inserted at the end.
    if (!InsertTailElementSkiplist(&pJob->pPrinter->JobList, pJob))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("InsertTailElementSkiplist failed for job %lu for the Printer's Job List!\n", pJob->dwJobID);
        goto Cleanup;
    }

    pPrinterHandle->pStartedJob = pJob;
    dwErrorCode = ERROR_SUCCESS;
    dwReturnValue = pJob->dwJobID;

Cleanup:
    SetLastError(dwErrorCode);
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
