/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
SKIPLIST PrinterList;

// Local Constants
static DWORD dwPrinterInfo1Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_1W, pName),
    FIELD_OFFSET(PRINTER_INFO_1W, pComment),
    FIELD_OFFSET(PRINTER_INFO_1W, pDescription),
    MAXDWORD
};

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
BOOL
InitializePrinterList()
{
    const WCHAR wszPrintersKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers";

    DWORD cbData;
    DWORD cchPrinterName;
    DWORD dwErrorCode;
    DWORD dwSubKeys;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PRINTER pPrinter = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PWSTR pwszPort = NULL;
    PWSTR pwszPrintProcessor = NULL;
    WCHAR wszPrinterName[MAX_PRINTER_NAME + 1];

    // Initialize an empty list for our printers.
    InitializeSkiplist(&PrinterList, DllAllocSplMem, _PrinterListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);

    // Open our printers registry key. Each subkey is a local printer there.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszPrintersKey, 0, KEY_READ, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of subkeys.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
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
            if (pPrinter->pDefaultDevMode)
                DllFreeSplMem(pPrinter->pDefaultDevMode);

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
        dwErrorCode = (DWORD)RegEnumKeyExW(hKey, i, wszPrinterName, &cchPrinterName, NULL, NULL, NULL, NULL);
        if (dwErrorCode == ERROR_MORE_DATA)
        {
            // This printer name exceeds the maximum length and is invalid.
            continue;
        }
        else if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed for iteration %lu with status %lu!\n", i, dwErrorCode);
            continue;
        }

        // Open this Printer's registry key.
        dwErrorCode = (DWORD)RegOpenKeyExW(hKey, wszPrinterName, 0, KEY_READ, &hSubKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Printer \"%S\" with status %lu!\n", wszPrinterName, dwErrorCode);
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

        // Get the Port.
        pwszPort = AllocAndRegQueryWSZ(hSubKey, L"Port");
        if (!pwszPort)
            continue;

        // Try to find it in the Port List.
        pPort = FindPort(pwszPort);
        if (!pPort)
        {
            ERR("Invalid Port \"%S\" for Printer \"%S\"!\n", pwszPort, wszPrinterName);
            continue;
        }

        // Create a new LOCAL_PRINTER structure for it.
        pPrinter = DllAllocSplMem(sizeof(LOCAL_PRINTER));
        if (!pPrinter)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pPrinter->pwszPrinterName = AllocSplStr(wszPrinterName);
        pPrinter->pPrintProcessor = pPrintProcessor;
        pPrinter->pPort = pPort;
        InitializePrinterJobList(pPrinter);

        // Get the location.
        pPrinter->pwszLocation = AllocAndRegQueryWSZ(hSubKey, L"Location");
        if (!pPrinter->pwszLocation)
            continue;

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

        // Determine the size of the DevMode.
        dwErrorCode = (DWORD)RegQueryValueExW(hSubKey, L"Default DevMode", NULL, NULL, NULL, &cbData);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("Couldn't query the size of the DevMode for Printer \"%S\", status is %lu, cbData is %lu!\n", wszPrinterName, dwErrorCode, cbData);
            continue;
        }

        // Allocate enough memory for the DevMode.
        pPrinter->pDefaultDevMode = DllAllocSplMem(cbData);
        if (!pPrinter->pDefaultDevMode)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        // Get the default DevMode.
        dwErrorCode = (DWORD)RegQueryValueExW(hSubKey, L"Default DevMode", NULL, NULL, (PBYTE)pPrinter->pDefaultDevMode, &cbData);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("Couldn't query a DevMode for Printer \"%S\", status is %lu, cbData is %lu!\n", wszPrinterName, dwErrorCode, cbData);
            continue;
        }

        // Get the Attributes.
        cbData = sizeof(DWORD);
        dwErrorCode = (DWORD)RegQueryValueExW(hSubKey, L"Attributes", NULL, NULL, (PBYTE)&pPrinter->dwAttributes, &cbData);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("Couldn't query Attributes for Printer \"%S\", status is %lu!\n", wszPrinterName, dwErrorCode);
            continue;
        }

        // Get the Status.
        cbData = sizeof(DWORD);
        dwErrorCode = (DWORD)RegQueryValueExW(hSubKey, L"Status", NULL, NULL, (PBYTE)&pPrinter->dwStatus, &cbData);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("Couldn't query Status for Printer \"%S\", status is %lu!\n", wszPrinterName, dwErrorCode);
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

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (hSubKey)
        RegCloseKey(hSubKey);

    if (pPrinter)
    {
        if (pPrinter->pDefaultDevMode)
            DllFreeSplMem(pPrinter->pDefaultDevMode);

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

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/**
 * @name _LocalEnumPrintersCheckName
 *
 * Checks the Name parameter supplied to a call to EnumPrinters.
 *
 * @param Flags
 * Flags parameter of EnumPrinters.
 *
 * @param Name
 * Name parameter of EnumPrinters to check.
 *
 * @param pwszComputerName
 * Pointer to a string able to hold 2 + MAX_COMPUTERNAME_LENGTH + 1 + 1 characters.
 * On return, it may contain a computer name to prepend in EnumPrinters depending on the case.
 *
 * @param pcchComputerName
 * If a string to prepend is returned, this pointer receives its length in characters.
 *
 * @return
 * ERROR_SUCCESS if processing in EnumPrinters can be continued.
 * ERROR_INVALID_NAME if the Name parameter is invalid for the given flags and this Print Provider.
 * Any other error code if GetComputerNameW fails. Error codes indicating failure should then be returned by EnumPrinters.
 */
static DWORD
_LocalEnumPrintersCheckName(DWORD Flags, PCWSTR Name, PWSTR pwszComputerName, PDWORD pcchComputerName)
{
    PCWSTR pName;
    PCWSTR pComputerName;

    // If there is no Name parameter to check, we can just continue in EnumPrinters.
    if (!Name)
        return ERROR_SUCCESS;

    // Check if Name does not begin with two backslashes (required for specifying Computer Names).
    if (Name[0] != L'\\' || Name[1] != L'\\')
    {
        if (Flags & PRINTER_ENUM_NAME)
        {
            // If PRINTER_ENUM_NAME is specified, any given Name parameter may only contain the
            // Print Provider Name or the local Computer Name.

            // Compare with the Print Provider Name.
            if (wcsicmp(Name, wszPrintProviderInfo[0]) == 0)
                return ERROR_SUCCESS;

            // Dismiss anything else.
            return ERROR_INVALID_NAME;
        }
        else
        {
            // If PRINTER_ENUM_NAME is not specified, we just ignore anything that is not a Computer Name.
            return ERROR_SUCCESS;
        }
    }

    // Prepend the backslashes to the output computer name.
    pwszComputerName[0] = L'\\';
    pwszComputerName[1] = L'\\';

    // Get the local computer name for comparison.
    *pcchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(&pwszComputerName[2], pcchComputerName))
    {
        ERR("GetComputerNameW failed with error %lu!\n", GetLastError());
        return GetLastError();
    }

    // Add the leading slashes to the total length.
    *pcchComputerName += 2;

    // Compare both names.
    pComputerName = &pwszComputerName[2];
    pName = &Name[2];
    for (;;)
    {
        // Are we at the end of the local Computer Name string?
        if (!*pComputerName)
        {
            // Are we also at the end of the supplied Name parameter?
            // A terminating NUL character and a backslash are both treated as the end, but they are treated differently.
            if (!*pName)
            {
                // If both names match and Name ends with a NUL character, the computer name will be prepended in EnumPrinters.
                // Add a trailing backslash for that.
                pwszComputerName[(*pcchComputerName)++] = L'\\';
                pwszComputerName[*pcchComputerName] = 0;
                return ERROR_SUCCESS;
            }
            else if (*pName == L'\\')
            {
                if (Flags & PRINTER_ENUM_NAME)
                {
                    // If PRINTER_ENUM_NAME is specified and a Name parameter is given, it must be exactly the local
                    // Computer Name with two backslashes prepended. Anything else (like "\\COMPUTERNAME\") is dismissed.
                    return ERROR_INVALID_NAME;
                }
                else
                {
                    // If PRINTER_ENUM_NAME is not specified and a Name parameter is given, it may also end with a backslash.
                    // Only the Computer Name between the backslashes is checked then.
                    // This is largely undocumented, but verified by tests (see winspool_apitest).
                    // In this case, no computer name is prepended in EnumPrinters though.
                    *pwszComputerName = 0;
                    *pcchComputerName = 0;
                    return ERROR_SUCCESS;
                }
            }
        }

        // Compare both Computer Names case-insensitively and reject with ERROR_INVALID_NAME if they don't match.
        if (towlower(*pName) != towlower(*pComputerName))
            return ERROR_INVALID_NAME;

        pName++;
        pComputerName++;
    }
}

static DWORD
_DumpLevel1PrintProviderInformation(PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    int i;

    // Count the needed bytes for Print Provider information.
    *pcbNeeded = sizeof(PRINTER_INFO_1W);

    for (i = 0; i < 3; i++)
        *pcbNeeded += (wcslen(wszPrintProviderInfo[i]) + 1) * sizeof(WCHAR);

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
        return ERROR_INSUFFICIENT_BUFFER;

    // Copy over the Print Provider information.
    ((PPRINTER_INFO_1W)pPrinterEnum)->Flags = 0;
    PackStrings(wszPrintProviderInfo, pPrinterEnum, dwPrinterInfo1Offsets, &pPrinterEnum[*pcbNeeded]);
    *pcReturned = 1;

    return ERROR_SUCCESS;
}

static DWORD
_LocalEnumPrintersLevel0(DWORD Flags, PCWSTR Name, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned, DWORD cchComputerName, PWSTR wszComputerName)
{
    return ERROR_INVALID_LEVEL;
}

static DWORD
_LocalEnumPrintersLevel1(DWORD Flags, PCWSTR Name, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned, DWORD cchComputerName, PWSTR wszComputerName)
{
    const WCHAR wszComma[] = L",";

    size_t cbName;
    size_t cbComment;
    size_t cbDescription;
    DWORD dwErrorCode;
    DWORD i;
    PBYTE pPrinterInfo;
    PBYTE pPrinterStrings;
    PSKIPLIST_NODE pNode;
    PLOCAL_PRINTER pPrinter;
    PWSTR p;
    PWSTR pwszStrings[3];

    if (Flags & PRINTER_ENUM_NAME && !Name)
    {
        // The caller wants information about this Print Provider.
        // spoolss packs this into an array of information about all Print Providers.
        dwErrorCode = _DumpLevel1PrintProviderInformation(pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
        goto Cleanup;
    }

    // Count the required buffer size and the number of printers.
    i = 0;

    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        // TODO: If PRINTER_ENUM_SHARED is given, add this Printer if it's shared instead of just ignoring it.
        if (Flags & PRINTER_ENUM_SHARED)
            continue;

        // Attention: pComment equals the "Description" registry value while pDescription is concatenated out of several strings.
        // On top of this, the computer name is prepended to the printer name if the user supplied the local computer name during the query.
        cbName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        cbDescription = cbName + (wcslen(pPrinter->pwszPrinterDriver) + 1 + wcslen(pPrinter->pwszLocation) + 1) * sizeof(WCHAR);

        *pcbNeeded += sizeof(PRINTER_INFO_1W) + cbName + cbComment + cbDescription;
        i++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Initialize the variables for filling the output buffer using PackStrings.
    pPrinterInfo = pPrinterEnum;
    pPrinterStrings = &pPrinterEnum[*pcbNeeded];

    // Copy over the Printer information.
    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        // TODO: If PRINTER_ENUM_SHARED is given, add this Printer if it's shared instead of just ignoring it.
        if (Flags & PRINTER_ENUM_SHARED)
            continue;

        // Indicate that this is a Printer.
        ((PPRINTER_INFO_1W)pPrinterInfo)->Flags = PRINTER_ENUM_ICON8;

        // Calculate the string lengths.
        cbName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        cbDescription = cbName + (wcslen(pPrinter->pwszPrinterDriver) + 1 + wcslen(pPrinter->pwszLocation) + 1) * sizeof(WCHAR);

        // Copy the Printer Name.
        pwszStrings[0] = DllAllocSplMem(cbName);
        p = pwszStrings[0];
        StringCbCopyExW(p, cbName, wszComputerName, &p, &cbName, 0);
        StringCbCopyExW(p, cbName, pPrinter->pwszPrinterName, &p, &cbName, 0);

        // Copy the Printer comment (equals the "Description" registry value).
        pwszStrings[1] = pPrinter->pwszDescription;

        // Copy the description, which for PRINTER_INFO_1W has the form "Name,Printer Driver,Location"
        pwszStrings[2] = DllAllocSplMem(cbDescription);
        p = pwszStrings[2];
        StringCbCopyExW(p, cbDescription, wszComputerName, &p, &cbDescription, 0);
        StringCbCopyExW(p, cbDescription, pPrinter->pwszPrinterName, &p, &cbDescription, 0);
        StringCbCopyExW(p, cbDescription, wszComma, &p, &cbDescription, 0);
        StringCbCopyExW(p, cbDescription, pPrinter->pwszPrinterDriver, &p, &cbDescription, 0);
        StringCbCopyExW(p, cbDescription, wszComma, &p, &cbDescription, 0);
        StringCbCopyExW(p, cbDescription, pPrinter->pwszLocation, &p, &cbDescription, 0);

        // Finally copy the structure and advance to the next one in the output buffer.
        pPrinterStrings = PackStrings(pwszStrings, pPrinterInfo, dwPrinterInfo1Offsets, pPrinterStrings);
        pPrinterInfo += sizeof(PRINTER_INFO_1W);

        // Free the memory for temporary strings.
        DllFreeSplMem(pwszStrings[0]);
        DllFreeSplMem(pwszStrings[2]);
    }

    *pcReturned = i;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    return dwErrorCode;
}

static DWORD
_LocalEnumPrintersLevel2(DWORD Flags, PCWSTR Name, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned, DWORD cchComputerName, PWSTR wszComputerName)
{
    return ERROR_INVALID_LEVEL;
}

static DWORD
_LocalEnumPrintersLevel4(DWORD Flags, PCWSTR Name, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned, DWORD cchComputerName, PWSTR wszComputerName)
{
    return ERROR_INVALID_LEVEL;
}

static DWORD
_LocalEnumPrintersLevel5(DWORD Flags, PCWSTR Name, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned, DWORD cchComputerName, PWSTR wszComputerName)
{
    return ERROR_INVALID_LEVEL;
}

BOOL WINAPI
LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD cchComputerName = 0;
    DWORD dwErrorCode;
    WCHAR wszComputerName[2 + MAX_COMPUTERNAME_LENGTH + 1 + 1] = { 0 };

    ASSERT(pcbNeeded);
    ASSERT(pcReturned);

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    if (Flags & PRINTER_ENUM_CONNECTIONS || Flags & PRINTER_ENUM_REMOTE || Flags & PRINTER_ENUM_NETWORK)
    {
        // If the flags for the Network Print Provider are given, bail out with ERROR_INVALID_NAME.
        // This is the internal way for a Print Provider to signal that it doesn't handle this request.
        dwErrorCode = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    if (!(Flags & PRINTER_ENUM_LOCAL || Flags & PRINTER_ENUM_NAME))
    {
        // The Local Print Provider is the right destination for the request, but without any of these flags,
        // there is no information that can be returned.
        // So just signal a successful request.
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Check the supplied Name parameter (if any).
    // This may return a Computer Name string we later prepend to the output.
    dwErrorCode = _LocalEnumPrintersCheckName(Flags, Name, wszComputerName, &cchComputerName);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    if (Level == 0)
    {
        dwErrorCode = _LocalEnumPrintersLevel0(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned, cchComputerName, wszComputerName);
    }
    else if (Level == 1)
    {
        dwErrorCode = _LocalEnumPrintersLevel1(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned, cchComputerName, wszComputerName);
    }
    else if (Level == 2)
    {
        dwErrorCode = _LocalEnumPrintersLevel2(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned, cchComputerName, wszComputerName);
    }
    else if (Level == 4)
    {
        dwErrorCode = _LocalEnumPrintersLevel4(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned, cchComputerName, wszComputerName);
    }
    else if (Level == 5)
    {
        dwErrorCode = _LocalEnumPrintersLevel5(Flags, Name, pPrinterEnum, cbBuf, pcbNeeded, pcReturned, cchComputerName, wszComputerName);
    }
    else
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    BOOL bReturnValue;
    DWORD cchComputerName;
    DWORD cchFirstParameter;
    DWORD dwErrorCode;
    DWORD dwJobID;
    HANDLE hExternalHandle;
    PWSTR p = lpPrinterName;
    PWSTR pwszFirstParameter = NULL;
    PWSTR pwszSecondParameter = NULL;
    PLOCAL_JOB pJob;
    PLOCAL_HANDLE pHandle = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PORT_HANDLE pPortHandle = NULL;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle = NULL;
    PLOCAL_XCV_HANDLE pXcvHandle = NULL;
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR wszFullPath[MAX_PATH];

    // TODO: lpPrinterName == NULL is supported and means access to the local printer server.
    // Not sure yet if that is passed down to localspl.dll or processed in advance.

    // Sanity checks
    if (!lpPrinterName || !phPrinter)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *phPrinter = NULL;

    // Skip any server name in the first parameter.
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

        // Get the local computer name for comparison.
        cchComputerName = _countof(wszComputerName);
        if (!GetComputerNameW(wszComputerName, &cchComputerName))
        {
            dwErrorCode = GetLastError();
            ERR("GetComputerNameW failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        // Now compare this string excerpt with the local computer name.
        // The input parameter may not be writable, so we can't null-terminate the input string at this point.
        // This print provider only supports local printers, so both strings have to match.
        if (p - lpPrinterName != cchComputerName || _wcsnicmp(lpPrinterName, wszComputerName, cchComputerName) != 0)
        {
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        // We have checked the server name and don't need it anymore.
        lpPrinterName = p + 1;
    }

    // Look for a comma. If it exists, it indicates the end of the first parameter.
    pwszSecondParameter = wcschr(lpPrinterName, L',');
    if (pwszSecondParameter)
        cchFirstParameter = pwszSecondParameter - p;
    else
        cchFirstParameter = wcslen(lpPrinterName);

    // We must have at least one parameter.
    if (!cchFirstParameter && !pwszSecondParameter)
    {
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;
        goto Cleanup;
    }

    // Do we have a first parameter?
    if (cchFirstParameter)
    {
        // Yes, extract it.
        // No null-termination is necessary here, because DllAllocSplMem returns a zero-initialized buffer.
        pwszFirstParameter = DllAllocSplMem((cchFirstParameter + 1) * sizeof(WCHAR));
        CopyMemory(pwszFirstParameter, lpPrinterName, cchFirstParameter * sizeof(WCHAR));
    }

    // Do we have a second parameter?
    if (pwszSecondParameter)
    {
        // Yes, skip the comma at the beginning.
        ++pwszSecondParameter;

        // Skip whitespace as well.
        while (*pwszSecondParameter == L' ')
            ++pwszSecondParameter;
    }

    // Create a new handle.
    pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
    if (!pHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Now we can finally check the type of handle actually requested.
    if (pwszFirstParameter && pwszSecondParameter && wcsncmp(pwszSecondParameter, L"Port", 4) == 0)
    {
        // The caller wants a port handle and provided a string like:
        //    "LPT1:, Port"
        //    "\\COMPUTERNAME\LPT1:, Port"
        
        // Look for this port in our Print Monitor Port list.
        pPort = FindPort(pwszFirstParameter);
        if (!pPort)
        {
            // The supplied port is unknown to all our Print Monitors.
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        pPrintMonitor = pPort->pPrintMonitor;

        // Call the monitor's OpenPort function.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnOpenPort(pPrintMonitor->hMonitor, pwszFirstParameter, &hExternalHandle);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnOpenPort(pwszFirstParameter, &hExternalHandle);

        if (!bReturnValue)
        {
            // The OpenPort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // Create a new LOCAL_PORT_HANDLE.
        pPortHandle = DllAllocSplMem(sizeof(LOCAL_PORT_HANDLE));
        if (!pPortHandle)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pPortHandle->hPort = hExternalHandle;
        pPortHandle->pPort = pPort;

        // Return the Port handle through our general handle.
        pHandle->HandleType = HandleType_Port;
        pHandle->pSpecificHandle = pPortHandle;
    }
    else if (!pwszFirstParameter && pwszSecondParameter && wcsncmp(pwszSecondParameter, L"Xcv", 3) == 0)
    {
        // The caller wants an Xcv handle and provided a string like:
        //    ", XcvMonitor Local Port"
        //    "\\COMPUTERNAME\, XcvMonitor Local Port"
        //    ", XcvPort LPT1:"
        //    "\\COMPUTERNAME\, XcvPort LPT1:"

        // Skip the "Xcv" string.
        pwszSecondParameter += 3;

        // Is XcvMonitor or XcvPort requested?
        if (wcsncmp(pwszSecondParameter, L"Monitor ", 8) == 0)
        {
            // Skip the "Monitor " string.
            pwszSecondParameter += 8;

            // Look for this monitor in our Print Monitor list.
            pPrintMonitor = FindPrintMonitor(pwszSecondParameter);
            if (!pPrintMonitor)
            {
                // The caller supplied a non-existing Monitor name.
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
                goto Cleanup;
            }
        }
        else if (wcsncmp(pwszSecondParameter, L"Port ", 5) == 0)
        {
            // Skip the "Port " string.
            pwszSecondParameter += 5;

            // Look for this port in our Print Monitor Port list.
            pPort = FindPort(pwszFirstParameter);
            if (!pPort)
            {
                // The supplied port is unknown to all our Print Monitors.
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
                goto Cleanup;
            }

            pPrintMonitor = pPort->pPrintMonitor;
        }
        else
        {
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        // Call the monitor's XcvOpenPort function.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvOpenPort(pPrintMonitor->hMonitor, pwszSecondParameter, SERVER_EXECUTE, &hExternalHandle);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvOpenPort(pwszSecondParameter, SERVER_EXECUTE, &hExternalHandle);

        if (!bReturnValue)
        {
            // The XcvOpenPort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // Create a new LOCAL_XCV_HANDLE.
        pXcvHandle = DllAllocSplMem(sizeof(LOCAL_XCV_HANDLE));
        if (!pXcvHandle)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pXcvHandle->hXcv = hExternalHandle;
        pXcvHandle->pPrintMonitor = pPrintMonitor;

        // Return the Xcv handle through our general handle.
        pHandle->HandleType = HandleType_Xcv;
        pHandle->pSpecificHandle = pXcvHandle;
    }
    else
    {
        // The caller wants a Printer or Printer Job handle and provided a string like:
        //    "HP DeskJet"
        //    "\\COMPUTERNAME\HP DeskJet"
        //    "HP DeskJet, Job 5"
        //    "\\COMPUTERNAME\HP DeskJet, Job 5"

        // Retrieve the printer from the list.
        pPrinter = LookupElementSkiplist(&PrinterList, &pwszFirstParameter, NULL);
        if (!pPrinter)
        {
            // The printer does not exist.
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Cleanup;
        }

        // Create a new LOCAL_PRINTER_HANDLE.
        pPrinterHandle = DllAllocSplMem(sizeof(LOCAL_PRINTER_HANDLE));
        if (!pPrinterHandle)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pPrinterHandle->hSPLFile = INVALID_HANDLE_VALUE;
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
            pPrinterHandle->pDevMode = DuplicateDevMode(pDefault->pDevMode);
        else
            pPrinterHandle->pDevMode = DuplicateDevMode(pPrinter->pDefaultDevMode);

        // Check if the caller wants a handle to an existing Print Job.
        if (pwszSecondParameter)
        {
            // The "Job " string has to follow now.
            if (wcsncmp(pwszSecondParameter, L"Job ", 4) != 0)
            {
                dwErrorCode = ERROR_INVALID_PRINTER_NAME;
                goto Cleanup;
            }

            // Skip the "Job " string. 
            pwszSecondParameter += 4;

            // Skip even more whitespace.
            while (*pwszSecondParameter == ' ')
                ++pwszSecondParameter;

            // Finally extract the desired Job ID.
            dwJobID = wcstoul(pwszSecondParameter, NULL, 10);
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

            // Try to open its SPL file.
            GetJobFilePath(L"SPL", dwJobID, wszFullPath);
            pPrinterHandle->hSPLFile = CreateFileW(wszFullPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (pPrinterHandle->hSPLFile == INVALID_HANDLE_VALUE)
            {
                dwErrorCode = GetLastError();
                ERR("CreateFileW failed with error %lu for \"%S\"!", dwErrorCode, wszFullPath);
                goto Cleanup;
            }

            // Associate the job to our Printer Handle, but don't set bStartedDoc.
            // This prevents the caller from doing further StartDocPrinter, WritePrinter, etc. calls on it.
            pPrinterHandle->pJob = pJob;
        }

        // Return the Printer handle through our general handle.
        pHandle->HandleType = HandleType_Printer;
        pHandle->pSpecificHandle = pPrinterHandle;
    }

    // We were successful! Return the handle.
    *phPrinter = (HANDLE)pHandle;
    dwErrorCode = ERROR_SUCCESS;

    // Don't let the cleanup routines free this.
    pHandle = NULL;
    pPrinterHandle = NULL;

Cleanup:
    if (pHandle)
        DllFreeSplMem(pHandle);

    if (pPrinterHandle)
    {
        if (pPrinterHandle->pwszDatatype)
            DllFreeSplStr(pPrinterHandle->pwszDatatype);

        if (pPrinterHandle->pDevMode)
            DllFreeSplMem(pPrinterHandle->pDevMode);

        DllFreeSplMem(pPrinterHandle);
    }

    if (pwszFirstParameter)
        DllFreeSplMem(pwszFirstParameter);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead)
{
    BOOL bReturnValue;
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PORT_HANDLE pPortHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Port handles are an entirely different thing.
    if (pHandle->HandleType == HandleType_Port)
    {
        pPortHandle = (PLOCAL_PORT_HANDLE)pHandle->pSpecificHandle;

        // Call the monitor's ReadPort function.
        if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnReadPort(pPortHandle->hPort, pBuf, cbBuf, pNoBytesRead);
        else
            bReturnValue = ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnReadPort(pPortHandle->hPort, pBuf, cbBuf, pNoBytesRead);

        if (!bReturnValue)
        {
            // The ReadPort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // We were successful!
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // The remaining function deals with Printer handles only.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // ReadPrinter needs an opened SPL file to work.
    // This only works if a Printer Job Handle was requested in OpenPrinter.
    if (pPrinterHandle->hSPLFile == INVALID_HANDLE_VALUE)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Pass the parameters to ReadFile.
    if (!ReadFile(pPrinterHandle->hSPLFile, pBuf, cbBuf, pNoBytesRead, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("ReadFile failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

DWORD WINAPI
LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    BOOL bReturnValue;
    DWORD dwErrorCode;
    DWORD dwReturnValue = 0;
    PDOC_INFO_1W pDocInfo1 = (PDOC_INFO_1W)pDocInfo;
    PLOCAL_JOB pJob;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PORT_HANDLE pPortHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Port handles are an entirely different thing.
    if (pHandle->HandleType == HandleType_Port)
    {
        pPortHandle = (PLOCAL_PORT_HANDLE)pHandle->pSpecificHandle;

        // This call should come from a Print Processor and the job this port is going to print was assigned to us before opening the Print Processor.
        // Claim it exclusively for this port handle.
        pJob = pPortHandle->pPort->pNextJobToProcess;
        pPortHandle->pPort->pNextJobToProcess = NULL;
        ASSERT(pJob);

        // Call the monitor's StartDocPort function.
        if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnStartDocPort(pPortHandle->hPort, pJob->pPrinter->pwszPrinterName, pJob->dwJobID, Level, pDocInfo);
        else
            bReturnValue = ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnStartDocPort(pPortHandle->hPort, pJob->pPrinter->pwszPrinterName, pJob->dwJobID, Level, pDocInfo);

        if (!bReturnValue)
        {
            // The StartDocPort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // We were successful!
        dwErrorCode = ERROR_SUCCESS;
        dwReturnValue = pJob->dwJobID;
        goto Cleanup;
    }

    // The remaining function deals with Printer handles only.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (!pDocInfo1)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // pJob may already be occupied if this is a Print Job handle. In this case, StartDocPrinter has to fail.
    if (pPrinterHandle->pJob)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Check the validity of the datatype if we got one.
    if (pDocInfo1->pDatatype && !FindDatatype(pPrinterHandle->pJob->pPrintProcessor, pDocInfo1->pDatatype))
    {
        dwErrorCode = ERROR_INVALID_DATATYPE;
        goto Cleanup;
    }

    // Check if this is the right document information level.
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // All requirements are met - create a new job.
    dwErrorCode = CreateJob(pPrinterHandle);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Use any given datatype.
    if (pDocInfo1->pDatatype && !ReallocSplStr(&pPrinterHandle->pJob->pwszDatatype, pDocInfo1->pDatatype))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Use any given document name.
    if (pDocInfo1->pDocName && !ReallocSplStr(&pPrinterHandle->pJob->pwszDocumentName, pDocInfo1->pDocName))
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("ReallocSplStr failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    // We were successful!
    dwErrorCode = ERROR_SUCCESS;
    dwReturnValue = pPrinterHandle->pJob->dwJobID;

Cleanup:
    SetLastError(dwErrorCode);
    return dwReturnValue;
}

BOOL WINAPI
LocalStartPagePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks.
    if (!pHandle || pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // We require StartDocPrinter or AddJob to be called first.
    if (!pPrinterHandle->bStartedDoc)
    {
        dwErrorCode = ERROR_SPL_NO_STARTDOC;
        goto Cleanup;
    }

    // Increase the page count.
    ++pPrinterHandle->pJob->dwTotalPages;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalWritePrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pcWritten)
{
    BOOL bReturnValue;
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PORT_HANDLE pPortHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Port handles are an entirely different thing.
    if (pHandle->HandleType == HandleType_Port)
    {
        pPortHandle = (PLOCAL_PORT_HANDLE)pHandle->pSpecificHandle;

        // Call the monitor's WritePort function.
        if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnWritePort(pPortHandle->hPort, pBuf, cbBuf, pcWritten);
        else
            bReturnValue = ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnWritePort(pPortHandle->hPort, pBuf, cbBuf, pcWritten);

        if (!bReturnValue)
        {
            // The WritePort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // We were successful!
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // The remaining function deals with Printer handles only.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // We require StartDocPrinter or AddJob to be called first.
    if (!pPrinterHandle->bStartedDoc)
    {
        dwErrorCode = ERROR_SPL_NO_STARTDOC;
        goto Cleanup;
    }

    // TODO: This function is only called when doing non-spooled printing.
    // This needs to be investigated further. We can't just use pPrinterHandle->hSPLFile here, because that's currently reserved for Printer Job handles (see LocalReadPrinter).
#if 0
    // Pass the parameters to WriteFile.
    if (!WriteFile(SOME_SPOOL_FILE_HANDLE, pBuf, cbBuf, pcWritten, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("WriteFile failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }
#endif

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEndPagePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle || pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // This function doesn't do anything else for now.
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEndDocPrinter(HANDLE hPrinter)
{
    BOOL bReturnValue;
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PORT_HANDLE pPortHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Port handles are an entirely different thing.
    if (pHandle->HandleType == HandleType_Port)
    {
        pPortHandle = (PLOCAL_PORT_HANDLE)pHandle->pSpecificHandle;

        // Call the monitor's EndDocPort function.
        if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnEndDocPort(pPortHandle->hPort);
        else
            bReturnValue = ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnEndDocPort(pPortHandle->hPort);

        if (!bReturnValue)
        {
            // The EndDocPort function failed. Return its last error.
            dwErrorCode = GetLastError();
            goto Cleanup;
        }

        // We were successful!
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // The remaining function deals with Printer handles only.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // We require StartDocPrinter or AddJob to be called first.
    if (!pPrinterHandle->bStartedDoc)
    {
        dwErrorCode = ERROR_SPL_NO_STARTDOC;
        goto Cleanup;
    }

    // TODO: Something like ScheduleJob

    // Finish the job.
    pPrinterHandle->bStartedDoc = FALSE;
    pPrinterHandle->pJob = NULL;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalClosePrinter(HANDLE hPrinter)
{
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PORT_HANDLE pPortHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;
    PLOCAL_XCV_HANDLE pXcvHandle;

    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pHandle->HandleType == HandleType_Port)
    {
        pPortHandle = (PLOCAL_PORT_HANDLE)pHandle->pSpecificHandle;

        // Call the monitor's ClosePort function.
        if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
            ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnClosePort(pPortHandle->hPort);
        else
            ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnClosePort(pPortHandle->hPort);
    }
    else if (pHandle->HandleType == HandleType_Printer)
    {
        pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

        // Terminate any started job.
        if (pPrinterHandle->pJob)
            FreeJob(pPrinterHandle->pJob);

        // Free memory for the fields.
        DllFreeSplMem(pPrinterHandle->pDevMode);
        DllFreeSplStr(pPrinterHandle->pwszDatatype);
    }
    else if (pHandle->HandleType == HandleType_Xcv)
    {
        pXcvHandle = (PLOCAL_XCV_HANDLE)pHandle->pSpecificHandle;

        // Call the monitor's XcvClosePort function.
        if (pXcvHandle->pPrintMonitor->bIsLevel2)
            ((PMONITOR2)pXcvHandle->pPrintMonitor->pMonitor)->pfnXcvClosePort(pXcvHandle->hXcv);
        else
            ((LPMONITOREX)pXcvHandle->pPrintMonitor->pMonitor)->Monitor.pfnXcvClosePort(pXcvHandle->hXcv);
    }

    // Free memory for the handle and the specific handle.
    DllFreeSplMem(pHandle->pSpecificHandle);
    DllFreeSplMem(pHandle);

    return TRUE;
}
