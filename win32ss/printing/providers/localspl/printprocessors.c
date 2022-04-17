/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"


// Local Variables
static LIST_ENTRY _PrintProcessorList;

/**
 * @name _OpenEnvironment
 *
 * Checks a supplied pEnvironment variable for validity and opens its registry key.
 *
 * @param pEnvironment
 * The pEnvironment variable to check.
 *
 * @param hKey
 * On success, this variable will contain a HKEY to the opened registry key of the environment.
 * You can use it for further tasks and have to close it with RegCloseKey.
 *
 * @return
 * A Windows Error Code indicating success or failure.
 */
static DWORD
_OpenEnvironment(PCWSTR pEnvironment, PHKEY hKey)
{
    const WCHAR wszEnvironmentsKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Environments\\";
    const DWORD cchEnvironmentsKey = _countof(wszEnvironmentsKey) - 1;

    DWORD cchEnvironment;
    DWORD dwErrorCode;
    PWSTR pwszEnvironmentKey = NULL;

    // Sanity checks
    if (!pEnvironment)
    {
        dwErrorCode = ERROR_INVALID_ENVIRONMENT;
        goto Cleanup;
    }

    // Construct the registry key of the demanded environment.
    cchEnvironment = wcslen(pEnvironment);
    pwszEnvironmentKey = DllAllocSplMem((cchEnvironmentsKey + cchEnvironment + 1) * sizeof(WCHAR));
    if (!pwszEnvironmentKey)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    CopyMemory(pwszEnvironmentKey, wszEnvironmentsKey, cchEnvironmentsKey * sizeof(WCHAR));
    CopyMemory(&pwszEnvironmentKey[cchEnvironmentsKey], pEnvironment, (cchEnvironment + 1) * sizeof(WCHAR));

    // Open the registry key.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, pwszEnvironmentKey, 0, KEY_READ, hKey);
    if (dwErrorCode == ERROR_FILE_NOT_FOUND)
    {
        dwErrorCode = ERROR_INVALID_ENVIRONMENT;
        goto Cleanup;
    }
    else if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

Cleanup:
    if (pwszEnvironmentKey)
        DllFreeSplMem(pwszEnvironmentKey);

    return dwErrorCode;
}

BOOL
FindDatatype(const PLOCAL_PRINT_PROCESSOR pPrintProcessor, PCWSTR pwszDatatype)
{
    DWORD i;
    PDATATYPES_INFO_1W pCurrentDatatype = pPrintProcessor->pDatatypesInfo1;

    TRACE("FindDatatype(%p, %S)\n", pPrintProcessor, pwszDatatype);

    if (!pwszDatatype)
        return FALSE;

    for (i = 0; i < pPrintProcessor->dwDatatypeCount; i++)
    {
        if (wcsicmp(pCurrentDatatype->pName, pwszDatatype) == 0)
            return TRUE;

        ++pCurrentDatatype;
    }

    return FALSE;
}

PLOCAL_PRINT_PROCESSOR
FindPrintProcessor(PCWSTR pwszName)
{
    PLIST_ENTRY pEntry;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;

    TRACE("FindPrintProcessor(%S)\n", pwszName);

    if (!pwszName)
        return NULL;

    for (pEntry = _PrintProcessorList.Flink; pEntry != &_PrintProcessorList; pEntry = pEntry->Flink)
    {
        pPrintProcessor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_PROCESSOR, Entry);

        if (wcsicmp(pPrintProcessor->pwszName, pwszName) == 0)
            return pPrintProcessor;
    }

    return NULL;
}

/**
 * @name InitializePrintProcessorList
 *
 * Initializes a singly linked list of locally available Print Processors.
 */
BOOL
InitializePrintProcessorList(void)
{
    DWORD cbDatatypes;
    DWORD cbFileName;
    DWORD cchPrintProcessorPath;
    DWORD cchMaxSubKey;
    DWORD cchPrintProcessorName;
    DWORD dwErrorCode;
    DWORD dwSubKeys;
    DWORD i;
    HINSTANCE hinstPrintProcessor;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    HKEY hSubSubKey = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor = NULL;
    WCHAR wszFileName[MAX_PATH];
    WCHAR wszPrintProcessorPath[MAX_PATH];

    TRACE("InitializePrintProcessorList()\n");

    // Initialize an empty list for our Print Processors.
    InitializeListHead(&_PrintProcessorList);

    // Prepare the path to the Print Processor directory.
    if (!LocalGetPrintProcessorDirectory(NULL, (PWSTR)wszCurrentEnvironment, 1, (PBYTE)wszPrintProcessorPath, sizeof(wszPrintProcessorPath), &cchPrintProcessorPath))
    {
        dwErrorCode = GetLastError();
        goto Cleanup;
    }

    // LocalGetPrintProcessorDirectory returns the number of copied bytes. Convert this into a number of characters without the terminating null-character.
    cchPrintProcessorPath /= sizeof(WCHAR);
    --cchPrintProcessorPath;

    // Append a trailing backslash.
    wszPrintProcessorPath[cchPrintProcessorPath] = L'\\';
    ++cchPrintProcessorPath;

    // Open the environment registry key.
    dwErrorCode = _OpenEnvironment(wszCurrentEnvironment, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Open the "Print Processors" subkey.
    dwErrorCode = (DWORD)RegOpenKeyExW(hKey, L"Print Processors", 0, KEY_READ, &hSubKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of Print Processors and maximum sub key length.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwSubKeys, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Loop through all available local Print Processors.
    for (i = 0; i < dwSubKeys; i++)
    {
        // Cleanup tasks from the previous run
        if (hSubSubKey)
        {
            RegCloseKey(hSubSubKey);
            hSubSubKey = NULL;
        }

        if (pPrintProcessor)
        {
            if (pPrintProcessor->pwszName)
                DllFreeSplStr(pPrintProcessor->pwszName);

            if (pPrintProcessor->pDatatypesInfo1)
                DllFreeSplMem(pPrintProcessor->pDatatypesInfo1);

            DllFreeSplMem(pPrintProcessor);
            pPrintProcessor = NULL;
        }

        // Create a new LOCAL_PRINT_PROCESSOR structure for it.
        pPrintProcessor = DllAllocSplMem(sizeof(LOCAL_PRINT_PROCESSOR));
        if (!pPrintProcessor)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        // Allocate memory for the Print Monitor Name.
        pPrintProcessor->pwszName = DllAllocSplMem((cchMaxSubKey + 1) * sizeof(WCHAR));
        if (!pPrintProcessor->pwszName)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        // Get the name of this Print Processor.
        cchPrintProcessorName = cchMaxSubKey + 1;
        dwErrorCode = (DWORD)RegEnumKeyExW(hSubKey, i, pPrintProcessor->pwszName, &cchPrintProcessorName, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %ld!\n", dwErrorCode);
            continue;
        }

        // Open this Print Processor's registry key.
        dwErrorCode = (DWORD)RegOpenKeyExW(hSubKey, pPrintProcessor->pwszName, 0, KEY_READ, &hSubSubKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Print Processor \"%S\" with status %lu!\n", pPrintProcessor->pwszName, dwErrorCode);
            continue;
        }

        // Get the file name of the Print Processor.
        cbFileName = sizeof(wszFileName);
        dwErrorCode = (DWORD)RegQueryValueExW(hSubSubKey, L"Driver", NULL, NULL, (PBYTE)wszFileName, &cbFileName);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed for Print Processor \"%S\" with status %lu!\n", pPrintProcessor->pwszName, dwErrorCode);
            continue;
        }

        // Verify that our buffer is large enough.
        if (cchPrintProcessorPath + cbFileName / sizeof(WCHAR) > MAX_PATH)
        {
            ERR("Print Processor directory \"%S\" for Print Processor \"%S\" is too long!\n", wszFileName, pPrintProcessor->pwszName);
            continue;
        }

        // Construct the full path to the Print Processor.
        CopyMemory(&wszPrintProcessorPath[cchPrintProcessorPath], wszFileName, cbFileName);

        // Try to load it.
        hinstPrintProcessor = LoadLibraryW(wszPrintProcessorPath);
        if (!hinstPrintProcessor)
        {
            ERR("LoadLibraryW failed for \"%S\" with error %lu!\n", wszPrintProcessorPath, GetLastError());
            continue;
        }

        // Get and verify all its function pointers.
        pPrintProcessor->pfnClosePrintProcessor = (PClosePrintProcessor)GetProcAddress(hinstPrintProcessor, "ClosePrintProcessor");
        if (!pPrintProcessor->pfnClosePrintProcessor)
        {
            ERR("Print Processor \"%S\" exports no ClosePrintProcessor!\n", wszPrintProcessorPath);
            continue;
        }

        pPrintProcessor->pfnControlPrintProcessor = (PControlPrintProcessor)GetProcAddress(hinstPrintProcessor, "ControlPrintProcessor");
        if (!pPrintProcessor->pfnControlPrintProcessor)
        {
            ERR("Print Processor \"%S\" exports no ControlPrintProcessor!\n", wszPrintProcessorPath);
            continue;
        }

        pPrintProcessor->pfnEnumPrintProcessorDatatypesW = (PEnumPrintProcessorDatatypesW)GetProcAddress(hinstPrintProcessor, "EnumPrintProcessorDatatypesW");
        if (!pPrintProcessor->pfnEnumPrintProcessorDatatypesW)
        {
            ERR("Print Processor \"%S\" exports no EnumPrintProcessorDatatypesW!\n", wszPrintProcessorPath);
            continue;
        }

        pPrintProcessor->pfnGetPrintProcessorCapabilities = (PGetPrintProcessorCapabilities)GetProcAddress(hinstPrintProcessor, "GetPrintProcessorCapabilities");
        if (!pPrintProcessor->pfnGetPrintProcessorCapabilities)
        {
            ERR("Print Processor \"%S\" exports no GetPrintProcessorCapabilities!\n", wszPrintProcessorPath);
            continue;
        }

        pPrintProcessor->pfnOpenPrintProcessor = (POpenPrintProcessor)GetProcAddress(hinstPrintProcessor, "OpenPrintProcessor");
        if (!pPrintProcessor->pfnOpenPrintProcessor)
        {
            ERR("Print Processor \"%S\" exports no OpenPrintProcessor!\n", wszPrintProcessorPath);
            continue;
        }

        pPrintProcessor->pfnPrintDocumentOnPrintProcessor = (PPrintDocumentOnPrintProcessor)GetProcAddress(hinstPrintProcessor, "PrintDocumentOnPrintProcessor");
        if (!pPrintProcessor->pfnPrintDocumentOnPrintProcessor)
        {
            ERR("Print Processor \"%S\" exports no PrintDocumentOnPrintProcessor!\n", wszPrintProcessorPath);
            continue;
        }

        // Get all supported datatypes.
        pPrintProcessor->pfnEnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, &cbDatatypes, &pPrintProcessor->dwDatatypeCount);
        pPrintProcessor->pDatatypesInfo1 = DllAllocSplMem(cbDatatypes);
        if (!pPrintProcessor->pDatatypesInfo1)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        if (!pPrintProcessor->pfnEnumPrintProcessorDatatypesW(NULL, NULL, 1, (PBYTE)pPrintProcessor->pDatatypesInfo1, cbDatatypes, &cbDatatypes, &pPrintProcessor->dwDatatypeCount))
        {
            ERR("EnumPrintProcessorDatatypesW failed for Print Processor \"%S\" with error %lu!\n", wszPrintProcessorPath, GetLastError());
            continue;
        }

        // Add the Print Processor to the list.
        InsertTailList(&_PrintProcessorList, &pPrintProcessor->Entry);

        // Don't let the cleanup routines free this.
        pPrintProcessor = NULL;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (hSubSubKey)
        RegCloseKey(hSubSubKey);

    if (pPrintProcessor)
    {
        if (pPrintProcessor->pwszName)
            DllFreeSplStr(pPrintProcessor->pwszName);

        if (pPrintProcessor->pDatatypesInfo1)
            DllFreeSplMem(pPrintProcessor->pDatatypesInfo1);

        DllFreeSplMem(pPrintProcessor);
    }

    // Outside the loop
    if (hSubKey)
        RegCloseKey(hSubKey);

    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/**
 * @name LocalEnumPrintProcessorDatatypes
 *
 * Obtains an array of all datatypes supported by a particular Print Processor.
 * Print Provider function for EnumPrintProcessorDatatypesA/EnumPrintProcessorDatatypesW.
 *
 * @param pName
 * Server Name. Ignored here, because every caller of LocalEnumPrintProcessorDatatypes is interested in the local directory.
 *
 * @param pPrintProcessorName
 * The (case-insensitive) name of the Print Processor to query.
 *
 * @param Level
 * The level of the structure supplied through pDatatypes. This must be 1.
 *
 * @param pDatatypes
 * Pointer to the buffer that receives an array of DATATYPES_INFO_1W structures.
 * Can be NULL if you just want to know the required size of the buffer.
 *
 * @param cbBuf
 * Size of the buffer you supplied for pDatatypes, in bytes.
 *
 * @param pcbNeeded
 * Pointer to a variable that receives the required size of the buffer for pDatatypes, in bytes.
 * This parameter mustn't be NULL!
 *
 * @param pcReturned
 * Pointer to a variable that receives the number of elements of the DATATYPES_INFO_1W array.
 * This parameter mustn't be NULL!
 *
 * @return
 * TRUE if we successfully copied the array into pDatatypes, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
LocalEnumPrintProcessorDatatypes(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD dwErrorCode;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;

    TRACE("LocalEnumPrintProcessorDatatypes(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Try to find the Print Processor.
    pPrintProcessor = FindPrintProcessor(pPrintProcessorName);
    if (!pPrintProcessor)
    {
        dwErrorCode = ERROR_UNKNOWN_PRINTPROCESSOR;
        goto Cleanup;
    }

    // Call its EnumPrintProcessorDatatypesW function.
    if (pPrintProcessor->pfnEnumPrintProcessorDatatypesW(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned))
        dwErrorCode = ERROR_SUCCESS;
    else
        dwErrorCode = GetLastError();

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/**
 * @name LocalEnumPrintProcessors
 *
 * Obtains an array of all available Print Processors on this computer.
 * Print Provider function for EnumPrintProcessorsA/EnumPrintProcessorsW.
 *
 * @param pName
 * Server Name. Ignored here, because every caller of LocalEnumPrintProcessors is interested in the local directory.
 *
 * @param pEnvironment
 * One of the predefined operating system and architecture "environment" strings (like "Windows NT x86").
 * Alternatively, NULL to output the Print Processor directory of the current environment.
 *
 * @param Level
 * The level of the structure supplied through pPrintProcessorInfo. This must be 1.
 *
 * @param pPrintProcessorInfo
 * Pointer to the buffer that receives an array of PRINTPROCESSOR_INFO_1W structures.
 * Can be NULL if you just want to know the required size of the buffer.
 *
 * @param cbBuf
 * Size of the buffer you supplied for pPrintProcessorInfo, in bytes.
 *
 * @param pcbNeeded
 * Pointer to a variable that receives the required size of the buffer for pPrintProcessorInfo, in bytes.
 * This parameter mustn't be NULL!
 *
 * @param pcReturned
 * Pointer to a variable that receives the number of elements of the PRINTPROCESSOR_INFO_1W array.
 * This parameter mustn't be NULL!
 *
 * @return
 * TRUE if we successfully copied the array into pPrintProcessorInfo, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
LocalEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD cchMaxSubKey;
    DWORD cchPrintProcessor;
    DWORD dwErrorCode;
    DWORD dwPrintProcessorCount;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    PBYTE pCurrentOutputPrintProcessor;
    PBYTE pCurrentOutputPrintProcessorInfo;
    PRINTPROCESSOR_INFO_1W PrintProcessorInfo1;
    PWSTR pwszTemp = NULL;

    TRACE("LocalEnumPrintProcessors(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (!pcbNeeded || !pcReturned)
    {
        // This error is also caught by RPC and returned as RPC_X_NULL_REF_POINTER.
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Verify pEnvironment and open its registry key.
    // We use the registry and not the PrintProcessorList here, because the caller may request information about a different environment.
    dwErrorCode = _OpenEnvironment(pEnvironment, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Open the "Print Processors" subkey.
    dwErrorCode = (DWORD)RegOpenKeyExW(hKey, L"Print Processors", 0, KEY_READ, &hSubKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of Print Processors and maximum sub key length.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwPrintProcessorCount, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate a temporary buffer to let RegEnumKeyExW succeed.
    pwszTemp = DllAllocSplMem((cchMaxSubKey + 1) * sizeof(WCHAR));
    if (!pwszTemp)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    // Determine the required size of the output buffer.
    *pcbNeeded = 0;

    for (i = 0; i < dwPrintProcessorCount; i++)
    {
        // RegEnumKeyExW sucks! Unlike similar API functions, it only returns the actual numbers of characters copied when you supply a buffer large enough.
        // So use pwszTemp with its size cchMaxSubKey for this.
        cchPrintProcessor = cchMaxSubKey + 1;
        dwErrorCode = (DWORD)RegEnumKeyExW(hSubKey, i, pwszTemp, &cchPrintProcessor, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        *pcbNeeded += sizeof(PRINTPROCESSOR_INFO_1W) + (cchPrintProcessor + 1) * sizeof(WCHAR);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Put the Print Processor strings right after the last PRINTPROCESSOR_INFO_1W structure.
    pCurrentOutputPrintProcessorInfo = pPrintProcessorInfo;
    pCurrentOutputPrintProcessor = pPrintProcessorInfo + dwPrintProcessorCount * sizeof(PRINTPROCESSOR_INFO_1W);

    // Copy over all Print Processors.
    for (i = 0; i < dwPrintProcessorCount; i++)
    {
        // This isn't really correct, but doesn't cause any harm, because we've extensively checked the size of the supplied buffer above.
        cchPrintProcessor = cchMaxSubKey + 1;

        // Copy the Print Processor name.
        dwErrorCode = (DWORD)RegEnumKeyExW(hSubKey, i, (PWSTR)pCurrentOutputPrintProcessor, &cchPrintProcessor, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        // Fill and copy the PRINTPROCESSOR_INFO_1W structure belonging to this Print Processor.
        PrintProcessorInfo1.pName = (PWSTR)pCurrentOutputPrintProcessor;
        CopyMemory(pCurrentOutputPrintProcessorInfo, &PrintProcessorInfo1, sizeof(PRINTPROCESSOR_INFO_1W));

        // Advance to the next PRINTPROCESSOR_INFO_1W location and string location in the output buffer.
        pCurrentOutputPrintProcessor += (cchPrintProcessor + 1) * sizeof(WCHAR);
        pCurrentOutputPrintProcessorInfo += sizeof(PRINTPROCESSOR_INFO_1W);
    }

    // We've finished successfully!
    *pcReturned = dwPrintProcessorCount;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszTemp)
        DllFreeSplMem(pwszTemp);

    if (hSubKey)
        RegCloseKey(hSubKey);

    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/**
 * @name LocalGetPrintProcessorDirectory
 *
 * Obtains the path to the local Print Processor directory.
 * Print Provider function for GetPrintProcessorDirectoryA/GetPrintProcessorDirectoryW.
 *
 * @param pName
 * Server Name. Ignored here, because every caller of LocalGetPrintProcessorDirectory is interested in the local directory.
 *
 * @param pEnvironment
 * One of the predefined operating system and architecture "environment" strings (like "Windows NT x86").
 *
 * @param Level
 * The level of the (non-existing) structure supplied through pPrintProcessorInfo. This must be 1.
 *
 * @param pPrintProcessorInfo
 * Pointer to the buffer that receives the full path to the Print Processor directory.
 * Can be NULL if you just want to know the required size of the buffer.
 *
 * @param cbBuf
 * Size of the buffer you supplied for pPrintProcessorInfo, in bytes.
 *
 * @param pcbNeeded
 * Pointer to a variable that receives the required size of the buffer for pPrintProcessorInfo, in bytes.
 * This parameter mustn't be NULL!
 *
 * @return
 * TRUE if we successfully copied the directory into pPrintProcessorInfo, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
LocalGetPrintProcessorDirectory(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    const WCHAR wszPath[] = L"\\PRTPROCS\\";
    const DWORD cchPath = _countof(wszPath) - 1;

    DWORD cbDirectoryName;
    DWORD dwErrorCode;
    HKEY hKey = NULL;
    PWSTR pwszDirectory = (PWSTR)pPrintProcessorInfo;

    TRACE("LocalGetPrintProcessorDirectory(%S, %S, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);

    // Verify pEnvironment and open its registry key.
    dwErrorCode = _OpenEnvironment(pEnvironment, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("_OpenEnvironment failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Determine the size of the required buffer.
    dwErrorCode = (DWORD)RegQueryValueExW(hKey, L"Directory", NULL, NULL, NULL, &cbDirectoryName);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    *pcbNeeded = (cchSpoolDirectory + cchPath) * sizeof(WCHAR) + cbDirectoryName;

    // Is the supplied buffer large enough?
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy the path to the "prtprocs" directory into pPrintProcessorInfo
    CopyMemory(pwszDirectory, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&pwszDirectory[cchSpoolDirectory], wszPath, cchPath * sizeof(WCHAR));

    // Get the directory name from the registry.
    dwErrorCode = (DWORD)RegQueryValueExW(hKey, L"Directory", NULL, NULL, (PBYTE)&pwszDirectory[cchSpoolDirectory + cchPath], &cbDirectoryName);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We've finished successfully!
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
