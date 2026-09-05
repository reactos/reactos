/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

// Global Variables
SKIPLIST PrinterList;

// Forward Declarations
static void _LocalGetPrinterLevel0(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_STRESS* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel1(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_1W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel2(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_2W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel3(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_3* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel4(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_4W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel5(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_5W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel6(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_6* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel7(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_7W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel8(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_8W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);
static void _LocalGetPrinterLevel9(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_9W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName);

// Local Constants
typedef void (*PLocalGetPrinterLevelFunc)(PLOCAL_PRINTER, PVOID, PBYTE*, PDWORD, DWORD, PCWSTR);

static const PLocalGetPrinterLevelFunc pfnGetPrinterLevels[] = {
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel0,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel1,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel2,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel3,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel4,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel5,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel6,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel7,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel8,
    (PLocalGetPrinterLevelFunc)&_LocalGetPrinterLevel9
};

static DWORD dwPrinterInfo0Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_STRESS, pPrinterName),
    MAXDWORD
};

static DWORD dwPrinterInfo1Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_1W, pName),
    FIELD_OFFSET(PRINTER_INFO_1W, pComment),
    FIELD_OFFSET(PRINTER_INFO_1W, pDescription),
    MAXDWORD
};

static DWORD dwPrinterInfo2Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_2W, pPrinterName),
    FIELD_OFFSET(PRINTER_INFO_2W, pShareName),
    FIELD_OFFSET(PRINTER_INFO_2W, pPortName),
    FIELD_OFFSET(PRINTER_INFO_2W, pDriverName),
    FIELD_OFFSET(PRINTER_INFO_2W, pComment),
    FIELD_OFFSET(PRINTER_INFO_2W, pLocation),
    FIELD_OFFSET(PRINTER_INFO_2W, pSepFile),
    FIELD_OFFSET(PRINTER_INFO_2W, pPrintProcessor),
    FIELD_OFFSET(PRINTER_INFO_2W, pDatatype),
    FIELD_OFFSET(PRINTER_INFO_2W, pParameters),
    MAXDWORD
};

static DWORD dwPrinterInfo4Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_4W, pPrinterName),
    MAXDWORD
};

static DWORD dwPrinterInfo5Offsets[] = {
    FIELD_OFFSET(PRINTER_INFO_5W, pPrinterName),
    FIELD_OFFSET(PRINTER_INFO_5W, pPortName),
    MAXDWORD
};

/** These values serve no purpose anymore, but are still used in PRINTER_INFO_5 and
    HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\PrinterPorts */
static const DWORD dwDeviceNotSelectedTimeout = 15000;
static const DWORD dwTransmissionRetryTimeout = 45000;


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

    return _wcsicmp(A->pwszPrinterName, B->pwszPrinterName);
}

/**
 * @name InitializePrinterList
 *
 * Initializes a list of locally available Printers.
 * The list is searchable by name and returns information about the printers, including their job queues.
 * During this process, the job queues are also initialized.
 */
BOOL
InitializePrinterList(VOID)
{
    DWORD cbData;
    DWORD cchPrinterName;
    DWORD dwErrorCode;
    DWORD dwSubKeys;
    DWORD i;
    HKEY hSubKey = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PRINTER pPrinter = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PWSTR pwszPort = NULL;
    PWSTR pwszPrintProcessor = NULL;
    WCHAR wszPrinterName[MAX_PRINTER_NAME + 1];

    TRACE("InitializePrinterList()\n");

    // Initialize an empty list for our printers.
    InitializeSkiplist(&PrinterList, DllAllocSplMem, _PrinterListCompareRoutine, (PSKIPLIST_FREE_ROUTINE)DllFreeSplMem);

    // Get the number of subkeys of the printers registry key. Each subkey is a local printer there.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hPrintersKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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
        dwErrorCode = (DWORD)RegEnumKeyExW(hPrintersKey, i, wszPrinterName, &cchPrinterName, NULL, NULL, NULL, NULL);
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
        dwErrorCode = (DWORD)RegOpenKeyExW(hPrintersKey, wszPrinterName, 0, KEY_READ, &hSubKey);
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
            ERR("DllAllocSplMem failed!\n");
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
            ERR("DllAllocSplMem failed!\n");
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

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

VOID
BroadcastChange(PLOCAL_HANDLE pHandle)
{
    PLOCAL_PRINTER pPrinter;
    PSKIPLIST_NODE pNode;
    DWORD cchMachineName = 0;
    WCHAR wszMachineName[MAX_PATH] = {0}; // if not local, use Machine Name then Printer Name... pPrinter->pJob->pwszMachineName?

    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        StringCchCopyW( &wszMachineName[cchMachineName], sizeof(wszMachineName), pPrinter->pwszPrinterName );

        PostMessageW( HWND_BROADCAST, WM_DEVMODECHANGE, 0, (LPARAM)&wszMachineName );
    }
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
            if (_wcsicmp(Name, wszPrintProviderInfo[0]) == 0)
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

static void
_LocalGetPrinterLevel0(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_STRESS* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    size_t cbName;
    PWSTR p, Allocation;
    PCWSTR pwszStrings[1];
    SYSTEM_INFO SystemInfo;

    // Calculate the string lengths.
    cbName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_STRESS) + cbName;
        return;
    }

    // Set the general fields.
    ZeroMemory(*ppPrinterInfo, sizeof(PRINTER_INFO_STRESS));
    (*ppPrinterInfo)->cJobs = pPrinter->JobList.NodeCount;
    (*ppPrinterInfo)->dwGetVersion = GetVersion();
    (*ppPrinterInfo)->Status = pPrinter->dwStatus;

#if !DBG
    (*ppPrinterInfo)->fFreeBuild = 1;
#endif

    GetSystemInfo(&SystemInfo);
    (*ppPrinterInfo)->dwNumberOfProcessors = SystemInfo.dwNumberOfProcessors;
    (*ppPrinterInfo)->dwProcessorType = SystemInfo.dwProcessorType;
    (*ppPrinterInfo)->wProcessorArchitecture = SystemInfo.wProcessorArchitecture;
    (*ppPrinterInfo)->wProcessorLevel = SystemInfo.wProcessorLevel;

    // Copy the Printer Name.
    p = Allocation = DllAllocSplMem(cbName);
    pwszStrings[0] = Allocation;
    StringCbCopyExW(p, cbName, wszComputerName, &p, &cbName, 0);
    StringCbCopyExW(p, cbName, pPrinter->pwszPrinterName, &p, &cbName, 0);

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppPrinterInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppPrinterInfo), dwPrinterInfo0Offsets, *ppPrinterInfoEnd);
    (*ppPrinterInfo)++;

    // Free the memory for temporary strings.
    DllFreeSplMem(Allocation);
}

static void
_LocalGetPrinterLevel1(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_1W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    const WCHAR wszComma[] = L",";

    size_t cbName;
    size_t cbComment;
    size_t cbDescription;
    PWSTR p, Allocation1, Allocation2;
    PCWSTR pwszStrings[3];

    // Calculate the string lengths.
    // Attention: pComment equals the "Description" registry value while pDescription is concatenated out of several strings.
    // On top of this, the computer name is prepended to the printer name if the user supplied the local computer name during the query.
    cbName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);
    cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
    cbDescription = cbName + (wcslen(pPrinter->pwszPrinterDriver) + 1 + wcslen(pPrinter->pwszLocation) + 1) * sizeof(WCHAR);

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_1W) + cbName + cbComment + cbDescription;
        return;
    }

    // Indicate that this is a Printer.
    (*ppPrinterInfo)->Flags = PRINTER_ENUM_ICON8;

    // Copy the Printer Name.
    p = Allocation1 = DllAllocSplMem(cbName);
    pwszStrings[0] = Allocation1;
    StringCbCopyExW(p, cbName, wszComputerName, &p, &cbName, 0);
    StringCbCopyExW(p, cbName, pPrinter->pwszPrinterName, &p, &cbName, 0);

    // Copy the Printer comment (equals the "Description" registry value).
    pwszStrings[1] = pPrinter->pwszDescription;

    // Copy the description, which for PRINTER_INFO_1W has the form "Name,Printer Driver,Location"
    p = Allocation2 = DllAllocSplMem(cbDescription);
    pwszStrings[2] = Allocation2;
    StringCbCopyExW(p, cbDescription, wszComputerName, &p, &cbDescription, 0);
    StringCbCopyExW(p, cbDescription, pPrinter->pwszPrinterName, &p, &cbDescription, 0);
    StringCbCopyExW(p, cbDescription, wszComma, &p, &cbDescription, 0);
    StringCbCopyExW(p, cbDescription, pPrinter->pwszPrinterDriver, &p, &cbDescription, 0);
    StringCbCopyExW(p, cbDescription, wszComma, &p, &cbDescription, 0);
    StringCbCopyExW(p, cbDescription, pPrinter->pwszLocation, &p, &cbDescription, 0);

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppPrinterInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppPrinterInfo), dwPrinterInfo1Offsets, *ppPrinterInfoEnd);
    (*ppPrinterInfo)++;

    // Free the memory for temporary strings.
    DllFreeSplMem(Allocation1);
    DllFreeSplMem(Allocation2);
}

static void
_LocalGetPrinterLevel2(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_2W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    WCHAR wszEmpty[] = L"";

    size_t cbDevMode;
    size_t cbPrinterName;
    size_t cbShareName;
    size_t cbPortName;
    size_t cbDriverName;
    size_t cbComment;
    size_t cbLocation;
    size_t cbSepFile;
    size_t cbPrintProcessor;
    size_t cbDatatype;
    size_t cbParameters;
    PWSTR p, Allocation;
    PCWSTR pwszStrings[10];
    FIXME("LocalGetPrinterLevel2\n");
    // Calculate the string lengths.
    cbDevMode = pPrinter->pDefaultDevMode->dmSize + pPrinter->pDefaultDevMode->dmDriverExtra;
    cbPrinterName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);

    if (!ppPrinterInfo)
    {
        // Attention: pComment equals the "Description" registry value.
        cbShareName = sizeof(wszEmpty);
        cbPortName = (wcslen(pPrinter->pPort->pwszName) + 1) * sizeof(WCHAR);
        cbDriverName = (wcslen(pPrinter->pwszPrinterDriver) + 1) * sizeof(WCHAR);
        cbComment = (wcslen(pPrinter->pwszDescription) + 1) * sizeof(WCHAR);
        cbLocation = (wcslen(pPrinter->pwszLocation) + 1) * sizeof(WCHAR);
        cbSepFile = sizeof(wszEmpty);
        cbPrintProcessor = (wcslen(pPrinter->pPrintProcessor->pwszName) + 1) * sizeof(WCHAR);
        cbDatatype = (wcslen(pPrinter->pwszDefaultDatatype) + 1) * sizeof(WCHAR);
        cbParameters = sizeof(wszEmpty);

        *pcbNeeded += sizeof(PRINTER_INFO_2W) + cbDevMode + cbPrinterName + cbShareName + cbPortName + cbDriverName + cbComment + cbLocation + cbSepFile + cbPrintProcessor + cbDatatype + cbParameters;
        FIXME("LocalGetPrinterLevel2 Needed %d\n",*pcbNeeded);
        return;
    }

    // Set the general fields.
    ZeroMemory(*ppPrinterInfo, sizeof(PRINTER_INFO_2W));
    (*ppPrinterInfo)->Attributes = pPrinter->dwAttributes;
    (*ppPrinterInfo)->cJobs = pPrinter->JobList.NodeCount;
    (*ppPrinterInfo)->Status = pPrinter->dwStatus;

    // Set the pDevMode field (and copy the DevMode).
    *ppPrinterInfoEnd -= cbDevMode;
    CopyMemory(*ppPrinterInfoEnd, pPrinter->pDefaultDevMode, cbDevMode);
    (*ppPrinterInfo)->pDevMode = (PDEVMODEW)(*ppPrinterInfoEnd);

    // Set the pPrinterName field.
    p = Allocation = DllAllocSplMem(cbPrinterName);
    pwszStrings[0] = Allocation;
    StringCbCopyExW(p, cbPrinterName, wszComputerName, &p, &cbPrinterName, 0);
    StringCbCopyExW(p, cbPrinterName, pPrinter->pwszPrinterName, &p, &cbPrinterName, 0);

    // Set the pShareName field.
    pwszStrings[1] = wszEmpty;

    // Set the pPortName field.
    pwszStrings[2] = pPrinter->pPort->pwszName;

    // Set the pDriverName field.
    pwszStrings[3] = pPrinter->pwszPrinterDriver;

    // Set the pComment field ((equals the "Description" registry value).
    pwszStrings[4] = pPrinter->pwszDescription;

    // Set the pLocation field.
    pwszStrings[5] = pPrinter->pwszLocation;

    // Set the pSepFile field.
    pwszStrings[6] = wszEmpty;

    // Set the pPrintProcessor field.
    pwszStrings[7] = pPrinter->pPrintProcessor->pwszName;

    // Set the pDatatype field.
    pwszStrings[8] = pPrinter->pwszDefaultDatatype;

    // Set the pParameters field.
    pwszStrings[9] = wszEmpty;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppPrinterInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppPrinterInfo), dwPrinterInfo2Offsets, *ppPrinterInfoEnd);
    (*ppPrinterInfo)++;

    // Free the memory for temporary strings.
    DllFreeSplMem(Allocation);
}

static void
_LocalGetPrinterLevel3(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_3* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    SECURITY_DESCRIPTOR SecurityDescriptor = { 0 };

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_3) + sizeof(SECURITY_DESCRIPTOR);
        return;
    }

    FIXME("Return a valid security descriptor for PRINTER_INFO_3\n");

    // Set the pSecurityDescriptor field (and copy the Security Descriptor).
    *ppPrinterInfoEnd -= sizeof(SECURITY_DESCRIPTOR);
    CopyMemory(*ppPrinterInfoEnd, &SecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    (*ppPrinterInfo)->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)(*ppPrinterInfoEnd);

    // Advance to the next structure.
    (*ppPrinterInfo)++;
}

static void
_LocalGetPrinterLevel4(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_4W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    size_t cbPrinterName;
    PWSTR p, Allocation;
    PCWSTR pwszStrings[1];

    // Calculate the string lengths.
    cbPrinterName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_4W) + cbPrinterName;
        return;
    }

    // Set the general fields.
    (*ppPrinterInfo)->pServerName = NULL;
    (*ppPrinterInfo)->Attributes = pPrinter->dwAttributes;

    // Set the pPrinterName field.
    p = Allocation = DllAllocSplMem(cbPrinterName);
    pwszStrings[0] = Allocation;
    StringCbCopyExW(p, cbPrinterName, wszComputerName, &p, &cbPrinterName, 0);
    StringCbCopyExW(p, cbPrinterName, pPrinter->pwszPrinterName, &p, &cbPrinterName, 0);

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppPrinterInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppPrinterInfo), dwPrinterInfo4Offsets, *ppPrinterInfoEnd);
    (*ppPrinterInfo)++;

    // Free the memory for temporary strings.
    DllFreeSplMem(Allocation);
}

static void
_LocalGetPrinterLevel5(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_5W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    size_t cbPrinterName;
    size_t cbPortName;
    PWSTR p, Allocation;
    PCWSTR pwszStrings[2];

    // Calculate the string lengths.
    cbPrinterName = (cchComputerName + wcslen(pPrinter->pwszPrinterName) + 1) * sizeof(WCHAR);

    if (!ppPrinterInfo)
    {
        cbPortName = (wcslen(pPrinter->pPort->pwszName) + 1) * sizeof(WCHAR);

        *pcbNeeded += sizeof(PRINTER_INFO_5W) + cbPrinterName + cbPortName;
        return;
    }

    // Set the general fields.
    (*ppPrinterInfo)->Attributes = pPrinter->dwAttributes;
    (*ppPrinterInfo)->DeviceNotSelectedTimeout = dwDeviceNotSelectedTimeout;
    (*ppPrinterInfo)->TransmissionRetryTimeout = dwTransmissionRetryTimeout;

    // Set the pPrinterName field.
    p = Allocation = DllAllocSplMem(cbPrinterName);
    pwszStrings[0] = Allocation;
    StringCbCopyExW(p, cbPrinterName, wszComputerName, &p, &cbPrinterName, 0);
    StringCbCopyExW(p, cbPrinterName, pPrinter->pwszPrinterName, &p, &cbPrinterName, 0);

    // Set the pPortName field.
    pwszStrings[1] = pPrinter->pPort->pwszName;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppPrinterInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppPrinterInfo), dwPrinterInfo5Offsets, *ppPrinterInfoEnd);
    (*ppPrinterInfo)++;

    // Free the memory for temporary strings.
    DllFreeSplMem(Allocation);
}

static void
_LocalGetPrinterLevel6(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_6* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_6);
        return;
    }

    // Set the general fields.
    (*ppPrinterInfo)->dwStatus = pPrinter->dwStatus;

    // Advance to the next structure.
    (*ppPrinterInfo)++;
}

static void
_LocalGetPrinterLevel7(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_7W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_7W);
        return;
    }

    FIXME("No Directory Support, returning DSPRINT_UNPUBLISH for PRINTER_INFO_7 all the time!\n");

    // Set the general fields.
    (*ppPrinterInfo)->dwAction = DSPRINT_UNPUBLISH;
    (*ppPrinterInfo)->pszObjectGUID = NULL;

    // Advance to the next structure.
    (*ppPrinterInfo)++;
}

static void
_LocalGetPrinterLevel8(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_8W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    DWORD cbDevMode;

    // Calculate the string lengths.
    cbDevMode = pPrinter->pDefaultDevMode->dmSize + pPrinter->pDefaultDevMode->dmDriverExtra;

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_8W) + cbDevMode;
        return;
    }

    // Set the pDevMode field (and copy the DevMode).
    *ppPrinterInfoEnd -= cbDevMode;
    CopyMemory(*ppPrinterInfoEnd, pPrinter->pDefaultDevMode, cbDevMode);
    (*ppPrinterInfo)->pDevMode = (PDEVMODEW)(*ppPrinterInfoEnd);

    // Advance to the next structure.
    (*ppPrinterInfo)++;
}

static void
_LocalGetPrinterLevel9(PLOCAL_PRINTER pPrinter, PPRINTER_INFO_9W* ppPrinterInfo, PBYTE* ppPrinterInfoEnd, PDWORD pcbNeeded, DWORD cchComputerName, PCWSTR wszComputerName)
{
    DWORD cbDevMode;

    // Calculate the string lengths.
    cbDevMode = pPrinter->pDefaultDevMode->dmSize + pPrinter->pDefaultDevMode->dmDriverExtra;

    if (!ppPrinterInfo)
    {
        *pcbNeeded += sizeof(PRINTER_INFO_9W) + cbDevMode;
        return;
    }

    FIXME("Per-user settings are not yet implemented, returning the global DevMode for PRINTER_INFO_9!\n");

    // Set the pDevMode field (and copy the DevMode).
    *ppPrinterInfoEnd -= cbDevMode;
    CopyMemory(*ppPrinterInfoEnd, pPrinter->pDefaultDevMode, cbDevMode);
    (*ppPrinterInfo)->pDevMode = (PDEVMODEW)(*ppPrinterInfoEnd);

    // Advance to the next structure.
    (*ppPrinterInfo)++;
}

BOOL WINAPI
LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD cchComputerName = 0;
    DWORD dwErrorCode;
    PBYTE pPrinterInfoEnd;
    PSKIPLIST_NODE pNode;
    WCHAR wszComputerName[2 + MAX_COMPUTERNAME_LENGTH + 1 + 1] = { 0 };
    PLOCAL_PRINTER pPrinter;

    FIXME("LocalEnumPrinters(%lu, %S, %lu, %p, %lu, %p, %p)\n", Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);

    // Do no sanity checks or assertions for pcbNeeded and pcReturned here.
    // This is verified and required by localspl_apitest!

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

    if (Level == 3 || Level > 5)
    {
        // The caller supplied an invalid level for EnumPrinters.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (Level == 1 && Flags & PRINTER_ENUM_NAME && !Name)
    {
        // The caller wants information about this Print Provider.
        // spoolss packs this into an array of information about all Print Providers.
        dwErrorCode = _DumpLevel1PrintProviderInformation(pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
        goto Cleanup;
    }

    // Check the supplied Name parameter (if any).
    // This may return a Computer Name string we later prepend to the output.
    dwErrorCode = _LocalEnumPrintersCheckName(Flags, Name, wszComputerName, &cchComputerName);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Count the required buffer size and the number of printers.
    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        // TODO: If PRINTER_ENUM_SHARED is given, add this Printer if it's shared instead of just ignoring it.
        if (Flags & PRINTER_ENUM_SHARED)
        {
            FIXME("Printer Sharing is not supported yet, returning no printers!\n");
            continue;
        }

        pfnGetPrinterLevels[Level](pPrinter, NULL, NULL, pcbNeeded, cchComputerName, wszComputerName);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the Printer information.
    pPrinterInfoEnd = &pPrinterEnum[*pcbNeeded];

    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        // TODO: If PRINTER_ENUM_SHARED is given, add this Printer if it's shared instead of just ignoring it.
        if (Flags & PRINTER_ENUM_SHARED)
            continue;

        pfnGetPrinterLevels[Level](pPrinter, &pPrinterEnum, &pPrinterInfoEnd, NULL, cchComputerName, wszComputerName);
        (*pcReturned)++;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalGetPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    // We never prepend a Computer Name to the output, but need to provide an empty string,
    // because this variable is passed to StringCbCopyExW.
    const WCHAR wszDummyComputerName[] = L"";

    DWORD dwErrorCode;
    PBYTE pPrinterEnd;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    TRACE("LocalGetPrinter(%p, %lu, %p, %lu, %p)\n", hPrinter, Level, pPrinter, cbBuf, pcbNeeded);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    if (Level > 9)
    {
        // The caller supplied an invalid level for GetPrinter.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;
    pfnGetPrinterLevels[Level](pPrinterHandle->pPrinter, NULL, NULL, pcbNeeded, 0, wszDummyComputerName);

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the Printer information.
    pPrinterEnd = &pPrinter[*pcbNeeded];
    pfnGetPrinterLevels[Level](pPrinterHandle->pPrinter, &pPrinter, &pPrinterEnd, NULL, 0, wszDummyComputerName);
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

static DWORD
_LocalOpenPortHandle(PWSTR pwszPortName, PHANDLE phPrinter)
{
    BOOL bReturnValue;
    DWORD dwErrorCode;
    HANDLE hPort;
    PLOCAL_HANDLE pHandle = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PORT_HANDLE pPortHandle = NULL;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    // Look for this port in our Print Monitor Port list.
    pPort = FindPort(pwszPortName);
    if (!pPort)
    {
        // The supplied port is unknown to all our Print Monitors.
        dwErrorCode = ERROR_INVALID_NAME;
        goto Failure;
    }

    pPrintMonitor = pPort->pPrintMonitor;

    // Call the monitor's OpenPort function.
    if (pPrintMonitor->bIsLevel2)
        bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnOpenPort(pPrintMonitor->hMonitor, pwszPortName, &hPort);
    else
        bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnOpenPort(pwszPortName, &hPort);

    if (!bReturnValue)
    {
        // The OpenPort function failed. Return its last error.
        dwErrorCode = GetLastError();
        goto Failure;
    }

    // Create a new generic handle.
    pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
    if (!pHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
    }

    // Create a new LOCAL_PORT_HANDLE.
    pPortHandle = DllAllocSplMem(sizeof(LOCAL_PORT_HANDLE));
    if (!pPortHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
    }

    pPortHandle->hPort = hPort;
    pPortHandle->pPort = pPort;

    // Make the generic handle a Port handle.
    pHandle->HandleType = HandleType_Port;
    pHandle->pSpecificHandle = pPortHandle;

    // Return it.
    *phPrinter = (HANDLE)pHandle;
    return ERROR_SUCCESS;

Failure:
    if (pHandle)
        DllFreeSplMem(pHandle);

    if (pPortHandle)
        DllFreeSplMem(pPortHandle);

    return dwErrorCode;
}

static DWORD
_LocalOpenPrinterHandle(PWSTR pwszPrinterName, PWSTR pwszJobParameter, PHANDLE phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    DWORD dwErrorCode;
    DWORD dwJobID;
    PLOCAL_HANDLE pHandle = NULL;
    PLOCAL_JOB pJob;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle = NULL;
    WCHAR wszFullPath[MAX_PATH];

    // Retrieve the printer from the list.
    pPrinter = LookupElementSkiplist(&PrinterList, &pwszPrinterName, NULL);
    if (!pPrinter)
    {
        // The printer does not exist.
        dwErrorCode = ERROR_INVALID_NAME;
        goto Failure;
    }

    // Create a new generic handle.
    pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
    if (!pHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
    }

    // Create a new LOCAL_PRINTER_HANDLE.
    pPrinterHandle = DllAllocSplMem(sizeof(LOCAL_PRINTER_HANDLE));
    if (!pPrinterHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
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
            goto Failure;
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
    if (pwszJobParameter)
    {
        // The "Job " string has to follow now.
        if (wcsncmp(pwszJobParameter, L"Job ", 4) != 0)
        {
            dwErrorCode = ERROR_INVALID_NAME;
            goto Failure;
        }

        // Skip the "Job " string.
        pwszJobParameter += 4;

        // Skip even more whitespace.
        while (*pwszJobParameter == ' ')
            ++pwszJobParameter;

        // Finally extract the desired Job ID.
        dwJobID = wcstoul(pwszJobParameter, NULL, 10);
        if (!IS_VALID_JOB_ID(dwJobID))
        {
            // The user supplied an invalid Job ID.
            dwErrorCode = ERROR_INVALID_NAME;
            goto Failure;
        }

        // Look for this job in the Global Job List.
        pJob = LookupElementSkiplist(&GlobalJobList, &dwJobID, NULL);
        if (!pJob || pJob->pPrinter != pPrinter)
        {
            // The user supplied a non-existing Job ID or the Job ID does not belong to the supplied printer name.
            dwErrorCode = ERROR_INVALID_PRINTER_NAME;
            goto Failure;
        }

        // Try to open its SPL file.
        GetJobFilePath(L"SPL", dwJobID, wszFullPath);
        pPrinterHandle->hSPLFile = CreateFileW(wszFullPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (pPrinterHandle->hSPLFile == INVALID_HANDLE_VALUE)
        {
            dwErrorCode = GetLastError();
            ERR("CreateFileW failed with error %lu for \"%S\"!\n", dwErrorCode, wszFullPath);
            goto Failure;
        }

        // Associate the job to our Printer Handle, but don't set bStartedDoc.
        // This prevents the caller from doing further StartDocPrinter, WritePrinter, etc. calls on it.
        pPrinterHandle->pJob = pJob;
    }

    // Make the generic handle a Printer handle.
    pHandle->HandleType = HandleType_Printer;
    pHandle->pSpecificHandle = pPrinterHandle;

    // Keep the Printer alive for as long as this handle exists.
    pPrinter->cRefs++;

    // Return it.
    *phPrinter = (HANDLE)pHandle;
    return ERROR_SUCCESS;

Failure:
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

    return dwErrorCode;
}

static DWORD
_LocalOpenPrintServerHandle(PHANDLE phPrinter)
{
    PLOCAL_HANDLE pHandle;

    // Create a new generic handle.
    pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
    if (!pHandle)
    {
        ERR("DllAllocSplMem failed!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Make the generic handle a Print Server handle.
    pHandle->HandleType = HandleType_PrintServer;
    pHandle->pSpecificHandle = NULL;

    // Return it.
    *phPrinter = (HANDLE)pHandle;
    return ERROR_SUCCESS;
}

static DWORD
_LocalOpenXcvHandle(PWSTR pwszParameter, PHANDLE phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    ACCESS_MASK GrantedAccess;
    BOOL bReturnValue;
    DWORD dwErrorCode;
    HANDLE hXcv;
    PLOCAL_HANDLE pHandle = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLOCAL_XCV_HANDLE pXcvHandle = NULL;

    // Skip the "Xcv" string.
    pwszParameter += 3;

    // Is XcvMonitor or XcvPort requested?
    if (wcsncmp(pwszParameter, L"Monitor ", 8) == 0)
    {
        // Skip the "Monitor " string.
        pwszParameter += 8;

        // Look for this monitor in our Print Monitor list.
        pPrintMonitor = FindPrintMonitor(pwszParameter);
        if (!pPrintMonitor)
        {
            // The caller supplied a non-existing Monitor name.
            dwErrorCode = ERROR_INVALID_NAME;
            ERR("OpenXcvHandle failed on Monitor name! %lu\n", dwErrorCode);
            goto Failure;
        }
    }
    else if (wcsncmp(pwszParameter, L"Port ", 5) == 0)
    {
        // Skip the "Port " string.
        pwszParameter += 5;

        // Look for this port in our Print Monitor Port list.
        pPort = FindPort(pwszParameter);
        if (!pPort)
        {
            // The supplied port is unknown to all our Print Monitors.
            dwErrorCode = ERROR_INVALID_NAME;
            ERR("OpenXcvHandle failed on Port name! %lu\n", dwErrorCode);
            goto Failure;
        }

        pPrintMonitor = pPort->pPrintMonitor;
    }
    else
    {
        dwErrorCode = ERROR_INVALID_NAME;
        ERR("OpenXcvHandle failed on bad name! %lu\n", dwErrorCode);
        goto Failure;
    }

    // Determine the access to grant on the port.
    // Print Monitors gate their administrative Xcv operations (AddPort, DeletePort,
    // ConfigurePort, ...) on SERVER_ACCESS_ADMINISTER, so passing a fixed
    // SERVER_EXECUTE here made every one of them fail with ERROR_ACCESS_DENIED.
    GrantedAccess = SERVER_EXECUTE;

    if (pDefault && pDefault->DesiredAccess)
    {
        GrantedAccess = pDefault->DesiredAccess;

        // Map the generic rights the same way the print server object does.
        if (GrantedAccess & GENERIC_READ)
            GrantedAccess = (GrantedAccess & ~GENERIC_READ) | SERVER_READ;

        if (GrantedAccess & GENERIC_WRITE)
            GrantedAccess = (GrantedAccess & ~GENERIC_WRITE) | SERVER_WRITE;

        if (GrantedAccess & GENERIC_EXECUTE)
            GrantedAccess = (GrantedAccess & ~GENERIC_EXECUTE) | SERVER_EXECUTE;

        if (GrantedAccess & GENERIC_ALL)
            GrantedAccess = (GrantedAccess & ~GENERIC_ALL) | SERVER_ALL_ACCESS;
    }

    // Call the monitor's XcvOpenPort function.
    if (pPrintMonitor->bIsLevel2)
        bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvOpenPort(pPrintMonitor->hMonitor, pwszParameter, GrantedAccess, &hXcv);
    else
        bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvOpenPort(pwszParameter, GrantedAccess, &hXcv);

    if (!bReturnValue)
    {
        // The XcvOpenPort function failed. Return its last error.
        dwErrorCode = GetLastError();
        ERR("XcvOpenPort function failed! %lu\n", dwErrorCode);
        goto Failure;
    }

    // Create a new generic handle.
    pHandle = DllAllocSplMem(sizeof(LOCAL_HANDLE));
    if (!pHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
    }

    // Create a new LOCAL_XCV_HANDLE.
    pXcvHandle = DllAllocSplMem(sizeof(LOCAL_XCV_HANDLE));
    if (!pXcvHandle)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Failure;
    }

    pXcvHandle->hXcv = hXcv;
    pXcvHandle->pPrintMonitor = pPrintMonitor;

    // Make the generic handle a Xcv handle.
    pHandle->HandleType = HandleType_Xcv;
    pHandle->pSpecificHandle = pXcvHandle;

    // Return it.
    *phPrinter = (HANDLE)pHandle;
    ERR("OpenXcvHandle Success! %p\n", pXcvHandle);
    return ERROR_SUCCESS;

Failure:
    if (pHandle)
        DllFreeSplMem(pHandle);

    if (pXcvHandle)
        DllFreeSplMem(pXcvHandle);

    return dwErrorCode;
}

//
// Dead API
//
DWORD WINAPI
LocalPrinterMessageBox(HANDLE hPrinter, DWORD Error, HWND hWnd, LPWSTR pText, LPWSTR pCaption, DWORD dwType)
{
    SetLastError(ERROR_INVALID_HANDLE); // Yes....
    return 0;
}

BOOL WINAPI
LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    DWORD cchComputerName;
    DWORD cchFirstParameter;
    DWORD dwErrorCode;
    PWSTR p = lpPrinterName;
    PWSTR pwszFirstParameter = NULL;
    PWSTR pwszSecondParameter;
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    TRACE("LocalOpenPrinter(%S, %p, %p)\n", lpPrinterName, phPrinter, pDefault);

    ASSERT(phPrinter);
    *phPrinter = NULL;

    if (!lpPrinterName)
    {
        // The caller wants a Print Server handle and provided a NULL string.
        dwErrorCode = _LocalOpenPrintServerHandle(phPrinter);
        goto Cleanup;
    }

    // Skip any server name in the first parameter.
    // Does lpPrinterName begin with two backslashes to indicate a server name?
    if (lpPrinterName[0] == L'\\' && lpPrinterName[1] == L'\\')
    {
        // Skip these two backslashes.
        lpPrinterName += 2;

        // Look for the terminating null character or closing backslash.
        p = lpPrinterName;
        while (*p != L'\0' && *p != L'\\')
            p++;

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
            dwErrorCode = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        // If lpPrinterName is only "\\COMPUTERNAME" with nothing more, the caller wants a handle to the local Print Server.
        if (!*p)
        {
            // The caller wants a Print Server handle and provided a string like:
            //    "\\COMPUTERNAME"
            dwErrorCode = _LocalOpenPrintServerHandle(phPrinter);
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
        dwErrorCode = ERROR_INVALID_NAME;
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

    // Now we can finally check the type of handle actually requested.
    if (pwszFirstParameter && pwszSecondParameter && wcsncmp(pwszSecondParameter, L"Port", 4) == 0)
    {
        // The caller wants a port handle and provided a string like:
        //    "LPT1:, Port"
        //    "\\COMPUTERNAME\LPT1:, Port"
        dwErrorCode = _LocalOpenPortHandle(pwszFirstParameter, phPrinter);
    }
    else if (!pwszFirstParameter && pwszSecondParameter && wcsncmp(pwszSecondParameter, L"Xcv", 3) == 0)
    {
        // The caller wants an Xcv handle and provided a string like:
        //    ", XcvMonitor Local Port"
        //    "\\COMPUTERNAME\, XcvMonitor Local Port"
        //    ", XcvPort LPT1:"
        //    "\\COMPUTERNAME\, XcvPort LPT1:"
        FIXME("OpenXcvHandle : %S\n",pwszSecondParameter);
        dwErrorCode = _LocalOpenXcvHandle(pwszSecondParameter, phPrinter, pDefault);
    }
    else
    {
        // The caller wants a Printer or Printer Job handle and provided a string like:
        //    "HP DeskJet"
        //    "\\COMPUTERNAME\HP DeskJet"
        //    "HP DeskJet, Job 5"
        //    "\\COMPUTERNAME\HP DeskJet, Job 5"
        dwErrorCode = _LocalOpenPrinterHandle(pwszFirstParameter, pwszSecondParameter, phPrinter, pDefault);
    }

Cleanup:
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

    TRACE("LocalReadPrinter(%p, %p, %lu, %p)\n", hPrinter, pBuf, cbBuf, pNoBytesRead);

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

    TRACE("LocalStartDocPrinter(%p, %lu, %p)\n", hPrinter, Level, pDocInfo);

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

    TRACE("LocalStartPagePrinter(%p)\n", hPrinter);

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

    TRACE("LocalWritePrinter(%p, %p, %lu, %p)\n", hPrinter, pBuf, cbBuf, pcWritten);

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

    TRACE("LocalEndPagePrinter(%p)\n", hPrinter);

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

    TRACE("LocalEndDocPrinter(%p)\n", hPrinter);

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

static void
_LocalClosePortHandle(PLOCAL_PORT_HANDLE pPortHandle)
{
    FIXME("LocalClosePortHandle\n");
    // Call the monitor's ClosePort function.
    if (pPortHandle->pPort->pPrintMonitor->bIsLevel2)
        ((PMONITOR2)pPortHandle->pPort->pPrintMonitor->pMonitor)->pfnClosePort(pPortHandle->hPort);
    else
        ((LPMONITOREX)pPortHandle->pPort->pPrintMonitor->pMonitor)->Monitor.pfnClosePort(pPortHandle->hPort);
}

/**
 * @name _FreeLocalPrinter
 *
 * Releases all resources held by a LOCAL_PRINTER structure.
 */
static void
_FreeLocalPrinter(PLOCAL_PRINTER pPrinter)
{
    if (pPrinter->pDefaultDevMode)
        DllFreeSplMem(pPrinter->pDefaultDevMode);

    if (pPrinter->pwszDefaultDatatype)
        DllFreeSplStr(pPrinter->pwszDefaultDatatype);

    if (pPrinter->pwszDescription)
        DllFreeSplStr(pPrinter->pwszDescription);

    if (pPrinter->pwszLocation)
        DllFreeSplStr(pPrinter->pwszLocation);

    if (pPrinter->pwszPrinterDriver)
        DllFreeSplStr(pPrinter->pwszPrinterDriver);

    if (pPrinter->pwszPrinterName)
        DllFreeSplStr(pPrinter->pwszPrinterName);

    DllFreeSplMem(pPrinter);
}

/**
 * @name ReleaseLocalPrinter
 *
 * Drops one handle reference on a Printer and frees it when the last handle to
 * a Printer that LocalDeletePrinter marked for deletion goes away.
 */
VOID
ReleaseLocalPrinter(PLOCAL_PRINTER pPrinter)
{
    if (!pPrinter)
        return;

    if (pPrinter->cRefs)
        pPrinter->cRefs--;

    if (pPrinter->cRefs == 0 && pPrinter->bPendingDeletion)
        _FreeLocalPrinter(pPrinter);
}

/**
 * @name _IsValidPrinterName
 *
 * Checks a Printer name against the restrictions the spooler puts on it.
 * A comma or a backslash would make the name ambiguous in the many places
 * where printers, drivers and ports are stored as comma separated lists.
 */
static BOOL
_IsValidPrinterName(PCWSTR pwszPrinterName)
{
    if (!pwszPrinterName || !*pwszPrinterName)
        return FALSE;

    if (wcslen(pwszPrinterName) > MAX_PRINTER_NAME)
        return FALSE;

    if (wcschr(pwszPrinterName, L',') || wcschr(pwszPrinterName, L'\\'))
        return FALSE;

    return TRUE;
}

/**
 * @name _RegSetSZ
 *
 * Writes a REG_SZ value, substituting an empty string for a missing one.
 * Every value read back by InitializePrinterList has to be present, otherwise
 * the Printer would be skipped the next time the spooler starts.
 */
static LONG
_RegSetSZ(HKEY hKey, PCWSTR pwszValueName, PCWSTR pwszValue)
{
    if (!pwszValue)
        pwszValue = L"";

    return RegSetValueExW(hKey, pwszValueName, 0, REG_SZ, (const BYTE*)pwszValue, (wcslen(pwszValue) + 1) * sizeof(WCHAR));
}

static LONG
_RegSetDWORD(HKEY hKey, PCWSTR pwszValueName, DWORD dwValue)
{
    return RegSetValueExW(hKey, pwszValueName, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
}

/**
 * @name _CreateDefaultDevMode
 *
 * Builds a minimal DEVMODEW for a Printer that was added without one.
 */
static PDEVMODEW
_CreateDefaultDevMode(PCWSTR pwszPrinterName)
{
    PDEVMODEW pDevMode;

    pDevMode = DllAllocSplMem(sizeof(DEVMODEW));
    if (!pDevMode)
    {
        ERR("DllAllocSplMem failed!\n");
        return NULL;
    }

    ZeroMemory(pDevMode, sizeof(DEVMODEW));
    StringCchCopyW(pDevMode->dmDeviceName, _countof(pDevMode->dmDeviceName), pwszPrinterName);
    pDevMode->dmSpecVersion = DM_SPECVERSION;
    pDevMode->dmSize = sizeof(DEVMODEW);
    pDevMode->dmDriverExtra = 0;

    return pDevMode;
}

/**
 * @name _IsPrinterDriverInstalled
 *
 * Checks whether a Printer Driver of the given name is installed for the
 * local print environment.
 */
static BOOL
_IsPrinterDriverInstalled(PCWSTR pwszDriverName)
{
    HKEY hDriverKey;
    HKEY hDriversKey;
    LONG lStatus;

    hDriversKey = open_driver_reg(NULL);
    if (!hDriversKey)
        return FALSE;

    lStatus = RegOpenKeyExW(hDriversKey, pwszDriverName, 0, KEY_READ, &hDriverKey);
    if (lStatus == ERROR_SUCCESS)
        RegCloseKey(hDriverKey);

    RegCloseKey(hDriversKey);
    return (lStatus == ERROR_SUCCESS);
}

/**
 * @name LocalAddPrinter
 *
 * Creates a new local Printer and returns an open handle to it.
 * This is the fpAddPrinter entry of the local Print Provider.
 */
HANDLE WINAPI
LocalAddPrinter(PWSTR pName, DWORD Level, PBYTE pPrinterInfo)
{
    BOOL bCreatedKey = FALSE;
    DWORD dwDisposition;
    DWORD dwErrorCode;
    HANDLE hPrinter = NULL;
    HKEY hPrinterKey = NULL;
    PDEVMODEW pDevMode = NULL;
    PLOCAL_PORT pPort;
    PLOCAL_PRINTER pPrinter = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PPRINTER_INFO_2W pInfo2 = (PPRINTER_INFO_2W)pPrinterInfo;
    PWSTR pwszDatatype;
    PWSTR pwszPrintProcessor;

    TRACE("LocalAddPrinter(%S, %lu, %p)\n", pName, Level, pPrinterInfo);

    // Only Level 2 carries enough information to create a Printer.
    if (Level != 2)
    {
        ERR("Level = %lu, unsupported!\n", Level);
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (!pInfo2)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!_IsValidPrinterName(pInfo2->pPrinterName))
    {
        ERR("Invalid Printer name!\n");
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;
        goto Cleanup;
    }

    // A Printer of this name must not exist yet.
    if (LookupElementSkiplist(&PrinterList, &pInfo2->pPrinterName, NULL))
    {
        dwErrorCode = ERROR_PRINTER_ALREADY_EXISTS;
        goto Cleanup;
    }

    // Find the Print Processor, defaulting to "WinPrint" like Windows does.
    pwszPrintProcessor = pInfo2->pPrintProcessor;
    if (!pwszPrintProcessor || !*pwszPrintProcessor)
        pwszPrintProcessor = L"WinPrint";

    pPrintProcessor = FindPrintProcessor(pwszPrintProcessor);
    if (!pPrintProcessor)
    {
        ERR("Invalid Print Processor \"%S\"!\n", pwszPrintProcessor);
        dwErrorCode = ERROR_UNKNOWN_PRINTPROCESSOR;
        goto Cleanup;
    }

    // Find the datatype, defaulting to "RAW".
    pwszDatatype = pInfo2->pDatatype;
    if (!pwszDatatype || !*pwszDatatype)
        pwszDatatype = L"RAW";

    if (!FindDatatype(pPrintProcessor, pwszDatatype))
    {
        ERR("Invalid datatype \"%S\"!\n", pwszDatatype);
        dwErrorCode = ERROR_INVALID_DATATYPE;
        goto Cleanup;
    }

    // Find the port.
    pPort = FindPort(pInfo2->pPortName);
    if (!pPort)
    {
        // The Port List is only built while the spooler starts up. Installers
        // routinely create their port first (through XcvData or a Print Monitor
        // of their own) and add the Printer right afterwards, so re-read the
        // ports from the monitors before giving up.
        RefreshPortList();
        pPort = FindPort(pInfo2->pPortName);
    }

    if (!pPort)
    {
        ERR("Invalid Port \"%S\"!\n", pInfo2->pPortName);
        dwErrorCode = ERROR_UNKNOWN_PORT;
        goto Cleanup;
    }

    // The driver has to be installed already.
    if (!pInfo2->pDriverName || !_IsPrinterDriverInstalled(pInfo2->pDriverName))
    {
        ERR("Invalid Printer Driver \"%S\"!\n", pInfo2->pDriverName);
        dwErrorCode = ERROR_UNKNOWN_PRINTER_DRIVER;
        goto Cleanup;
    }

    // Take the supplied DevMode or synthesize a default one.
    if (pInfo2->pDevMode)
        pDevMode = DuplicateDevMode(pInfo2->pDevMode);
    else
        pDevMode = _CreateDefaultDevMode(pInfo2->pPrinterName);

    if (!pDevMode)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Create the Printer's registry key.
    dwErrorCode = (DWORD)RegCreateKeyExW(hPrintersKey, pInfo2->pPrinterName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hPrinterKey, &dwDisposition);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW failed for Printer \"%S\" with status %lu!\n", pInfo2->pPrinterName, dwErrorCode);
        goto Cleanup;
    }

    if (dwDisposition != REG_CREATED_NEW_KEY)
    {
        // A key of that name was already there, so it is not ours to remove again.
        dwErrorCode = ERROR_PRINTER_ALREADY_EXISTS;
        goto Cleanup;
    }

    bCreatedKey = TRUE;

    // Write everything InitializePrinterList reads back on the next startup.
    _RegSetSZ(hPrinterKey, L"Name", pInfo2->pPrinterName);
    _RegSetSZ(hPrinterKey, L"Share Name", pInfo2->pShareName);
    _RegSetSZ(hPrinterKey, L"Port", pPort->pwszName);
    _RegSetSZ(hPrinterKey, L"Printer Driver", pInfo2->pDriverName);
    _RegSetSZ(hPrinterKey, L"Description", pInfo2->pComment);
    _RegSetSZ(hPrinterKey, L"Location", pInfo2->pLocation);
    _RegSetSZ(hPrinterKey, L"Print Processor", pwszPrintProcessor);
    _RegSetSZ(hPrinterKey, L"Datatype", pwszDatatype);
    _RegSetSZ(hPrinterKey, L"Parameters", pInfo2->pParameters);
    _RegSetSZ(hPrinterKey, L"Separator File", pInfo2->pSepFile);
    _RegSetDWORD(hPrinterKey, L"Attributes", pInfo2->Attributes);
    _RegSetDWORD(hPrinterKey, L"Priority", pInfo2->Priority);
    _RegSetDWORD(hPrinterKey, L"Default Priority", pInfo2->DefaultPriority);
    _RegSetDWORD(hPrinterKey, L"StartTime", pInfo2->StartTime);
    _RegSetDWORD(hPrinterKey, L"UntilTime", pInfo2->UntilTime);
    _RegSetDWORD(hPrinterKey, L"Status", 0);
    RegSetValueExW(hPrinterKey, L"Default DevMode", 0, REG_BINARY, (const BYTE*)pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);

    // Create the in-memory Printer.
    pPrinter = DllAllocSplMem(sizeof(LOCAL_PRINTER));
    if (!pPrinter)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    ZeroMemory(pPrinter, sizeof(LOCAL_PRINTER));
    pPrinter->pwszPrinterName = AllocSplStr(pInfo2->pPrinterName);
    pPrinter->pwszPrinterDriver = AllocSplStr(pInfo2->pDriverName);
    pPrinter->pwszDescription = AllocSplStr(pInfo2->pComment ? pInfo2->pComment : L"");
    pPrinter->pwszLocation = AllocSplStr(pInfo2->pLocation ? pInfo2->pLocation : L"");
    pPrinter->pwszDefaultDatatype = AllocSplStr(pwszDatatype);
    pPrinter->pDefaultDevMode = pDevMode;
    pPrinter->pPrintProcessor = pPrintProcessor;

    // The Printer owns the DevMode from here on.
    pDevMode = NULL;
    pPrinter->pPort = pPort;
    pPrinter->dwAttributes = pInfo2->Attributes;
    pPrinter->dwStatus = 0;

    if (!pPrinter->pwszPrinterName || !pPrinter->pwszPrinterDriver || !pPrinter->pwszDescription ||
        !pPrinter->pwszLocation || !pPrinter->pwszDefaultDatatype)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    InitializePrinterJobList(pPrinter);

    if (!InsertElementSkiplist(&PrinterList, pPrinter))
    {
        ERR("InsertElementSkiplist failed for Printer \"%S\"!\n", pPrinter->pwszPrinterName);
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Hand back an open handle to the new Printer, just like Windows does.
    dwErrorCode = _LocalOpenPrinterHandle(pPrinter->pwszPrinterName, NULL, &hPrinter, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("_LocalOpenPrinterHandle failed for Printer \"%S\" with error %lu!\n", pPrinter->pwszPrinterName, dwErrorCode);
        DeleteElementSkiplist(&PrinterList, pPrinter);
        goto Cleanup;
    }

    // The Printer is now owned by the Printer List.
    pPrinter = NULL;
    dwErrorCode = ROUTER_SUCCESS;

Cleanup:
    if (hPrinterKey)
        RegCloseKey(hPrinterKey);

    // Roll back the registry key, but only when this call is the one that created it.
    if (dwErrorCode != ROUTER_SUCCESS && bCreatedKey)
        RegDeleteKeyW(hPrintersKey, pInfo2->pPrinterName);

    if (pPrinter)
        _FreeLocalPrinter(pPrinter);

    if (pDevMode)
        DllFreeSplMem(pDevMode);

    SetLastError(dwErrorCode);
    return hPrinter;
}

/**
 * @name LocalDeletePrinter
 *
 * Deletes the Printer a handle refers to.
 * This is the fpDeletePrinter entry of the local Print Provider.
 */
BOOL WINAPI
LocalDeletePrinter(HANDLE hPrinter)
{
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    TRACE("LocalDeletePrinter(%p)\n", hPrinter);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;
    pPrinter = pPrinterHandle->pPrinter;

    if (pPrinter->bPendingDeletion)
    {
        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Windows refuses to delete a Printer that still has queued jobs.
    if (pPrinter->JobList.NodeCount)
    {
        ERR("Printer \"%S\" still has %lu job(s)!\n", pPrinter->pwszPrinterName, pPrinter->JobList.NodeCount);
        dwErrorCode = ERROR_PRINTER_HAS_JOBS_QUEUED;
        goto Cleanup;
    }

    // Delete the Printer's registry key.
    RegDeleteKeyW(hPrintersKey, pPrinter->pwszPrinterName);

    // Take it out of the Printer List so that it is neither enumerated nor
    // opened anymore, and mark it so the structure itself is released once the
    // last handle referring to it is closed.
    DeleteElementSkiplist(&PrinterList, pPrinter);
    pPrinter->bPendingDeletion = TRUE;
    pPrinter->dwStatus |= PRINTER_STATUS_PENDING_DELETION;

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/**
 * @name LocalSetPrinter
 *
 * Changes the configuration or the state of a Printer.
 * This is the fpSetPrinter entry of the local Print Provider.
 */
BOOL WINAPI
LocalSetPrinter(HANDLE hPrinter, DWORD Level, PBYTE pPrinterInfo, DWORD Command)
{
    DWORD dwErrorCode;
    HKEY hPrinterKey = NULL;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;
    PLOCAL_PRINTER pPrinter;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    TRACE("LocalSetPrinter(%p, %lu, %p, %lu)\n", hPrinter, Level, pPrinterInfo, Command);

    if (!pHandle || pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;
    pPrinter = pPrinterHandle->pPrinter;

    // A command always takes precedence over the supplied structure.
    if (Command)
    {
        switch (Command)
        {
            case PRINTER_CONTROL_PAUSE:
                pPrinter->dwStatus |= PRINTER_STATUS_PAUSED;
                break;

            case PRINTER_CONTROL_RESUME:
                pPrinter->dwStatus &= ~PRINTER_STATUS_PAUSED;
                break;

            case PRINTER_CONTROL_PURGE:
                // Nothing is queued that we could purge yet.
                break;

            case PRINTER_CONTROL_SET_STATUS:
                if (!pPrinterInfo)
                {
                    dwErrorCode = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }

                pPrinter->dwStatus = ((PPRINTER_INFO_6)pPrinterInfo)->dwStatus;
                break;

            default:
                dwErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
        }

        dwErrorCode = (DWORD)RegOpenKeyExW(hPrintersKey, pPrinter->pwszPrinterName, 0, KEY_SET_VALUE, &hPrinterKey);
        if (dwErrorCode == ERROR_SUCCESS)
            _RegSetDWORD(hPrinterKey, L"Status", pPrinter->dwStatus);

        dwErrorCode = ERROR_SUCCESS;
        goto Cleanup;
    }

    dwErrorCode = (DWORD)RegOpenKeyExW(hPrintersKey, pPrinter->pwszPrinterName, 0, KEY_SET_VALUE, &hPrinterKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed for Printer \"%S\" with status %lu!\n", pPrinter->pwszPrinterName, dwErrorCode);
        goto Cleanup;
    }

    switch (Level)
    {
        case 2:
        {
            PLOCAL_PORT pPort;
            PLOCAL_PRINT_PROCESSOR pPrintProcessor;
            PPRINTER_INFO_2W pInfo2 = (PPRINTER_INFO_2W)pPrinterInfo;

            if (!pInfo2)
            {
                dwErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            // Renaming a Printer is not supported yet, so the name is ignored here.
            if (pInfo2->pPortName && *pInfo2->pPortName)
            {
                pPort = FindPort(pInfo2->pPortName);
                if (!pPort)
                {
                    RefreshPortList();
                    pPort = FindPort(pInfo2->pPortName);
                }

                if (!pPort)
                {
                    dwErrorCode = ERROR_UNKNOWN_PORT;
                    goto Cleanup;
                }

                pPrinter->pPort = pPort;
                _RegSetSZ(hPrinterKey, L"Port", pPort->pwszName);
            }

            if (pInfo2->pPrintProcessor && *pInfo2->pPrintProcessor)
            {
                pPrintProcessor = FindPrintProcessor(pInfo2->pPrintProcessor);
                if (!pPrintProcessor)
                {
                    dwErrorCode = ERROR_UNKNOWN_PRINTPROCESSOR;
                    goto Cleanup;
                }

                pPrinter->pPrintProcessor = pPrintProcessor;
                _RegSetSZ(hPrinterKey, L"Print Processor", pInfo2->pPrintProcessor);
            }

            if (pInfo2->pDatatype && *pInfo2->pDatatype)
            {
                if (!FindDatatype(pPrinter->pPrintProcessor, pInfo2->pDatatype))
                {
                    dwErrorCode = ERROR_INVALID_DATATYPE;
                    goto Cleanup;
                }

                DllFreeSplStr(pPrinter->pwszDefaultDatatype);
                pPrinter->pwszDefaultDatatype = AllocSplStr(pInfo2->pDatatype);
                _RegSetSZ(hPrinterKey, L"Datatype", pInfo2->pDatatype);
            }

            if (pInfo2->pDriverName && *pInfo2->pDriverName)
            {
                if (!_IsPrinterDriverInstalled(pInfo2->pDriverName))
                {
                    dwErrorCode = ERROR_UNKNOWN_PRINTER_DRIVER;
                    goto Cleanup;
                }

                DllFreeSplStr(pPrinter->pwszPrinterDriver);
                pPrinter->pwszPrinterDriver = AllocSplStr(pInfo2->pDriverName);
                _RegSetSZ(hPrinterKey, L"Printer Driver", pInfo2->pDriverName);
            }

            if (pInfo2->pComment)
            {
                DllFreeSplStr(pPrinter->pwszDescription);
                pPrinter->pwszDescription = AllocSplStr(pInfo2->pComment);
                _RegSetSZ(hPrinterKey, L"Description", pInfo2->pComment);
            }

            if (pInfo2->pLocation)
            {
                DllFreeSplStr(pPrinter->pwszLocation);
                pPrinter->pwszLocation = AllocSplStr(pInfo2->pLocation);
                _RegSetSZ(hPrinterKey, L"Location", pInfo2->pLocation);
            }

            if (pInfo2->pShareName)
                _RegSetSZ(hPrinterKey, L"Share Name", pInfo2->pShareName);

            if (pInfo2->pSepFile)
                _RegSetSZ(hPrinterKey, L"Separator File", pInfo2->pSepFile);

            if (pInfo2->pParameters)
                _RegSetSZ(hPrinterKey, L"Parameters", pInfo2->pParameters);

            if (pInfo2->pDevMode)
            {
                PDEVMODEW pDevMode = DuplicateDevMode(pInfo2->pDevMode);

                if (!pDevMode)
                {
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                if (pPrinter->pDefaultDevMode)
                    DllFreeSplMem(pPrinter->pDefaultDevMode);

                pPrinter->pDefaultDevMode = pDevMode;
                RegSetValueExW(hPrinterKey, L"Default DevMode", 0, REG_BINARY, (const BYTE*)pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);
            }

            pPrinter->dwAttributes = pInfo2->Attributes;
            _RegSetDWORD(hPrinterKey, L"Attributes", pInfo2->Attributes);
            _RegSetDWORD(hPrinterKey, L"Priority", pInfo2->Priority);
            _RegSetDWORD(hPrinterKey, L"Default Priority", pInfo2->DefaultPriority);
            _RegSetDWORD(hPrinterKey, L"StartTime", pInfo2->StartTime);
            _RegSetDWORD(hPrinterKey, L"UntilTime", pInfo2->UntilTime);

            dwErrorCode = ERROR_SUCCESS;
            break;
        }

        case 6:
        {
            PPRINTER_INFO_6 pInfo6 = (PPRINTER_INFO_6)pPrinterInfo;

            if (!pInfo6)
            {
                dwErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pPrinter->dwStatus = pInfo6->dwStatus;
            _RegSetDWORD(hPrinterKey, L"Status", pPrinter->dwStatus);
            dwErrorCode = ERROR_SUCCESS;
            break;
        }

        case 8:
        case 9:
        {
            PPRINTER_INFO_9W pInfo9 = (PPRINTER_INFO_9W)pPrinterInfo;
            PDEVMODEW pDevMode;

            if (!pInfo9 || !pInfo9->pDevMode)
            {
                dwErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pDevMode = DuplicateDevMode(pInfo9->pDevMode);
            if (!pDevMode)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if (pPrinter->pDefaultDevMode)
                DllFreeSplMem(pPrinter->pDefaultDevMode);

            pPrinter->pDefaultDevMode = pDevMode;
            RegSetValueExW(hPrinterKey, L"Default DevMode", 0, REG_BINARY, (const BYTE*)pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);
            dwErrorCode = ERROR_SUCCESS;
            break;
        }

        default:
            FIXME("Unsupported level %lu!\n", Level);
            dwErrorCode = ERROR_INVALID_LEVEL;
            break;
    }

Cleanup:
    if (hPrinterKey)
        RegCloseKey(hPrinterKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

static void
_LocalClosePrinterHandle(PLOCAL_PRINTER_HANDLE pPrinterHandle)
{
    FIXME("LocalClosePrinterHandle\n");
    // Terminate any started job.
    if (pPrinterHandle->pJob)
        FreeJob(pPrinterHandle->pJob);

    // Free memory for the fields.
    DllFreeSplMem(pPrinterHandle->pDevMode);
    DllFreeSplStr(pPrinterHandle->pwszDatatype);

    // This releases the Printer itself if it was deleted while we held a handle.
    ReleaseLocalPrinter(pPrinterHandle->pPrinter);
}

static void
_LocalCloseXcvHandle(PLOCAL_XCV_HANDLE pXcvHandle)
{
    // Call the monitor's XcvClosePort function.
    if (pXcvHandle->pPrintMonitor->bIsLevel2)
        ((PMONITOR2)pXcvHandle->pPrintMonitor->pMonitor)->pfnXcvClosePort(pXcvHandle->hXcv);
    else
        ((LPMONITOREX)pXcvHandle->pPrintMonitor->pMonitor)->Monitor.pfnXcvClosePort(pXcvHandle->hXcv);
}

BOOL WINAPI
LocalClosePrinter(HANDLE hPrinter)
{
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;

    FIXME("LocalClosePrinter(%p)\n", hPrinter);

    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pHandle->HandleType == HandleType_Port)
    {
        _LocalClosePortHandle(pHandle->pSpecificHandle);
    }
    else if (pHandle->HandleType == HandleType_Printer)
    {
        _LocalClosePrinterHandle(pHandle->pSpecificHandle);
    }
    else if (pHandle->HandleType == HandleType_PrintServer)
    {
        // Nothing to do.
    }
    else if (pHandle->HandleType == HandleType_Xcv)
    {
        _LocalCloseXcvHandle(pHandle->pSpecificHandle);
    }
    FIXME("LocalClosePrinter 1\n");
    // Free memory for the handle and the specific handle (if any).
    if (pHandle->pSpecificHandle)
        DllFreeSplMem(pHandle->pSpecificHandle);
    FIXME("LocalClosePrinter 2\n");
    DllFreeSplMem(pHandle);
    FIXME("LocalClosePrinter 3\n");
    return TRUE;
}
