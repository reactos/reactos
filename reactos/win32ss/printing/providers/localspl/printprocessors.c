/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


// Global Variables
RTL_GENERIC_TABLE PrintProcessorTable;

/**
 * @name _OpenEnvironment
 *
 * Checks a supplied pEnvironment variable for validity and opens its registry key.
 *
 * @param pEnvironment
 * The pEnvironment variable to check. Can be NULL to use the current environment.
 *
 * @param hKey
 * On success, this variable will contain a HKEY to the opened registry key of the environment.
 * You can use it for further tasks and have to close it with RegCloseKey.
 *
 * @return
 * TRUE if the environment is valid and a registry key was opened, FALSE otherwise.
 * In case of failure, Last Error will be set to ERROR_INVALID_ENVIRONMENT.
 */
static BOOL
_OpenEnvironment(PCWSTR pEnvironment, PHKEY hKey)
{
    const WCHAR wszEnvironmentsKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Environments\\";
    const DWORD cchEnvironmentsKey = sizeof(wszEnvironmentsKey) / sizeof(WCHAR) - 1;

    BOOL bReturnValue = FALSE;
    DWORD cchEnvironment;
    LONG lStatus;
    PWSTR pwszEnvironmentKey = NULL;

    // Use the current environment if none was supplied.
    if (!pEnvironment)
        pEnvironment = wszCurrentEnvironment;

    // Construct the registry key of the demanded environment.
    cchEnvironment = wcslen(pEnvironment);
    pwszEnvironmentKey = HeapAlloc(hProcessHeap, 0, (cchEnvironmentsKey + cchEnvironment + 1) * sizeof(WCHAR));
    if (!pwszEnvironmentKey)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    CopyMemory(pwszEnvironmentKey, wszEnvironmentsKey, cchEnvironmentsKey * sizeof(WCHAR));
    CopyMemory(&pwszEnvironmentKey[cchEnvironmentsKey], pEnvironment, (cchEnvironment + 1) * sizeof(WCHAR));

    // Open the registry key.
    lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, pwszEnvironmentKey, 0, KEY_READ, hKey);
    if (lStatus == ERROR_FILE_NOT_FOUND)
    {
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        goto Cleanup;
    }
    else if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    bReturnValue = TRUE;

Cleanup:
    if (pwszEnvironmentKey)
        HeapFree(hProcessHeap, 0, pwszEnvironmentKey);

    return bReturnValue;
}

/**
 * @name _PrinterTableCompareRoutine
 *
 * RTL_GENERIC_COMPARE_ROUTINE for the Print Processor Table.
 * Does a case-insensitive comparison, because e.g. LocalEnumPrintProcessorDatatypes doesn't match the case when looking for Print Processors.
 */
static RTL_GENERIC_COMPARE_RESULTS NTAPI
_PrintProcessorTableCompareRoutine(PRTL_GENERIC_TABLE Table, PVOID FirstStruct, PVOID SecondStruct)
{
    PLOCAL_PRINT_PROCESSOR A = (PLOCAL_PRINT_PROCESSOR)FirstStruct;
    PLOCAL_PRINT_PROCESSOR B = (PLOCAL_PRINT_PROCESSOR)SecondStruct;

    int iResult = wcsicmp(A->pwszName, B->pwszName);

    if (iResult < 0)
        return GenericLessThan;
    else if (iResult > 0)
        return GenericGreaterThan;
    else
        return GenericEqual;
}

/**
 * @name _DatatypeTableCompareRoutine
 *
 * RTL_GENERIC_COMPARE_ROUTINE for the Datatype Table.
 * Does a case-insensitive comparison, because e.g. LocalOpenPrinter doesn't match the case when looking for Datatypes.
 */
static RTL_GENERIC_COMPARE_RESULTS NTAPI
_DatatypeTableCompareRoutine(PRTL_GENERIC_TABLE Table, PVOID FirstStruct, PVOID SecondStruct)
{
    PWSTR A = (PWSTR)FirstStruct;
    PWSTR B = (PWSTR)SecondStruct;

    int iResult = wcsicmp(A, B);

    if (iResult < 0)
        return GenericLessThan;
    else if (iResult > 0)
        return GenericGreaterThan;
    else
        return GenericEqual;
}

/**
 * @name InitializePrintProcessorTable
 *
 * Initializes a RTL_GENERIC_TABLE of locally available Print Processors.
 * The table is searchable by name and returns pointers to the functions of the loaded Print Processor DLL.
 */
void
InitializePrintProcessorTable()
{
    DWORD cbDatatypes;
    DWORD cbFileName;
    DWORD cchPrintProcessorPath;
    DWORD cchMaxSubKey;
    DWORD cchPrintProcessorName;
    DWORD dwDatatypes;
    DWORD dwSubKeys;
    DWORD i;
    DWORD j;
    HINSTANCE hinstPrintProcessor;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    HKEY hSubSubKey = NULL;
    LONG lStatus;
    PDATATYPES_INFO_1W pDatatypesInfo1 = NULL;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor = NULL;
    PWSTR pwszDatatype = NULL;
    PWSTR pwszPrintProcessorName = NULL;
    WCHAR wszFileName[MAX_PATH];
    WCHAR wszPrintProcessorPath[MAX_PATH];

    // Initialize an empty table for our Print Processors.
    // We will search it by Print Processor name.
    RtlInitializeGenericTable(&PrintProcessorTable, _PrintProcessorTableCompareRoutine, GenericTableAllocateRoutine, GenericTableFreeRoutine, NULL);
    
    // Prepare the path to the Print Processor directory.
    if (!LocalGetPrintProcessorDirectory(NULL, NULL, 1, (PBYTE)wszPrintProcessorPath, sizeof(wszPrintProcessorPath), &cchPrintProcessorPath))
        goto Cleanup;

    cchPrintProcessorPath /= sizeof(WCHAR);
    wszPrintProcessorPath[cchPrintProcessorPath++] = L'\\';

    // Open the environment registry key.
    if (!_OpenEnvironment(NULL, &hKey))
        goto Cleanup;

    // Open the "Print Processors" subkey.
    lStatus = RegOpenKeyExW(hKey, L"Print Processors", 0, KEY_READ, &hSubKey);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Get the number of Print Processors and maximum sub key length.
    lStatus = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwSubKeys, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Allocate a temporary buffer for the Print Processor names.
    pwszPrintProcessorName = HeapAlloc(hProcessHeap, 0, (cchMaxSubKey + 1) * sizeof(WCHAR));
    if (!pwszPrintProcessorName)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
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
                HeapFree(hProcessHeap, 0, pPrintProcessor->pwszName);

            HeapFree(hProcessHeap, 0, pPrintProcessor);
            pPrintProcessor = NULL;
        }

        if (pDatatypesInfo1)
        {
            HeapFree(hProcessHeap, 0, pDatatypesInfo1);
            pDatatypesInfo1 = NULL;
        }

        // Get the name of this Print Processor.
        cchPrintProcessorName = cchMaxSubKey;
        lStatus = RegEnumKeyExW(hSubKey, i, pwszPrintProcessorName, &cchPrintProcessorName, NULL, NULL, NULL, NULL);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %ld!\n", lStatus);
            continue;
        }

        // Open this Print Processor's registry key.
        lStatus = RegOpenKeyExW(hSubKey, pwszPrintProcessorName, 0, KEY_READ, &hSubSubKey);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Print Processor \"%S\" with status %ld!\n", pwszPrintProcessorName, lStatus);
            continue;
        }

        // Get the file name of the Print Processor.
        cbFileName = sizeof(wszFileName);
        lStatus = RegQueryValueExW(hSubSubKey, L"Driver", NULL, NULL, (PBYTE)wszFileName, &cbFileName);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed for Print Processor \"%S\" with status %ld!\n", pwszPrintProcessorName, lStatus);
            continue;
        }

        // Verify that our buffer is large enough.
        if (cchPrintProcessorPath + cbFileName / sizeof(WCHAR) > MAX_PATH)
        {
            ERR("Print Processor directory \"%S\" for Print Processor \"%S\" is too long!\n", wszFileName, pwszPrintProcessorName);
            continue;
        }

        // Construct the full path to the Print Processor.
        CopyMemory(&wszPrintProcessorPath[cchPrintProcessorPath], wszFileName, cbFileName);

        // Try to load it.
        hinstPrintProcessor = LoadLibraryW(wszPrintProcessorPath);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("LoadLibraryW failed for \"%S\" with error %lu!\n", wszPrintProcessorPath, GetLastError());
            continue;
        }

        // Create a new LOCAL_PRINT_PROCESSOR structure for it.
        pPrintProcessor = HeapAlloc(hProcessHeap, 0, sizeof(LOCAL_PRINT_PROCESSOR));
        pPrintProcessor->pwszName = DuplicateStringW(pwszPrintProcessorName);

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
        pPrintProcessor->pfnEnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, &cbDatatypes, &dwDatatypes);
        pDatatypesInfo1 = HeapAlloc(hProcessHeap, 0, cbDatatypes);
        if (!pDatatypesInfo1)
        {
            ERR("HeapAlloc failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        if (!pPrintProcessor->pfnEnumPrintProcessorDatatypesW(NULL, NULL, 1, (PBYTE)pDatatypesInfo1, cbDatatypes, &cbDatatypes, &dwDatatypes))
        {
            ERR("EnumPrintProcessorDatatypesW failed for Print Processor \"%S\" with error %lu!\n", wszPrintProcessorPath, GetLastError());
            continue;
        }

        // Add the supported datatypes to the datatype table.
        RtlInitializeGenericTable(&pPrintProcessor->DatatypeTable, _DatatypeTableCompareRoutine, GenericTableAllocateRoutine, GenericTableFreeRoutine, NULL);

        for (j = 0; j < dwDatatypes; j++)
        {
            pwszDatatype = DuplicateStringW(pDatatypesInfo1->pName);

            if (!RtlInsertElementGenericTable(&pPrintProcessor->DatatypeTable, pDatatypesInfo1->pName, sizeof(PWSTR), NULL))
            {
                ERR("RtlInsertElementGenericTable failed for iteration %lu with error %lu!\n", j, GetLastError());
                goto Cleanup;
            }

            ++pDatatypesInfo1;
        }

        // Add the Print Processor to the table.
        if (!RtlInsertElementGenericTable(&PrintProcessorTable, pPrintProcessor, sizeof(LOCAL_PRINT_PROCESSOR), NULL))
        {
            ERR("RtlInsertElementGenericTable failed for iteration %lu with error %lu!\n", i, GetLastError());
            goto Cleanup;
        }

        // Don't let the cleanup routines free this.
        pwszDatatype = NULL;
        pPrintProcessor = NULL;
    }

Cleanup:
    if (pwszDatatype)
        HeapFree(hProcessHeap, 0, pwszDatatype);

    if (pDatatypesInfo1)
        HeapFree(hProcessHeap, 0, pDatatypesInfo1);

    if (pPrintProcessor)
    {
        if (pPrintProcessor->pwszName)
            HeapFree(hProcessHeap, 0, pPrintProcessor->pwszName);

        HeapFree(hProcessHeap, 0, pPrintProcessor);
    }

    if (pwszPrintProcessorName)
        HeapFree(hProcessHeap, 0, pwszPrintProcessorName);

    if (hSubSubKey)
        RegCloseKey(hSubSubKey);

    if (hSubKey)
        RegCloseKey(hSubKey);

    if (hKey)
        RegCloseKey(hKey);
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
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;

    // Sanity checks
    if (Level != 1)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    // Try to find the Print Processor.
    pPrintProcessor = RtlLookupElementGenericTable(&PrintProcessorTable, pPrintProcessorName);
    if (!pPrintProcessor)
    {
        SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
        return FALSE;
    }

    // Call its EnumPrintProcessorDatatypesW function.
    return pPrintProcessor->pfnEnumPrintProcessorDatatypesW(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
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
    BOOL bReturnValue = FALSE;
    DWORD cchMaxSubKey;
    DWORD cchPrintProcessor;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    LONG lStatus;
    PBYTE pCurrentOutputPrintProcessor;
    PBYTE pCurrentOutputPrintProcessorInfo;
    PRINTPROCESSOR_INFO_1W PrintProcessorInfo1;
    PWSTR pwszEnvironmentKey = NULL;
    PWSTR pwszTemp = NULL;

    // Sanity checks
    if (Level != 1)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        goto Cleanup;
    }

    if (!pcbNeeded || !pcReturned)
    {
        // This error must be caught by RPC and returned as RPC_X_NULL_REF_POINTER.
        ERR("pcbNeeded or pcReturned is NULL!\n");
        goto Cleanup;
    }

    // Verify pEnvironment and open its registry key.
    if (!_OpenEnvironment(pEnvironment, &hKey))
        goto Cleanup;

    // Open the "Print Processors" subkey.
    lStatus = RegOpenKeyExW(hKey, L"Print Processors", 0, KEY_READ, &hSubKey);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Get the number of Print Processors and maximum sub key length.
    lStatus = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, pcReturned, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Allocate a temporary buffer to let RegEnumKeyExW succeed.
    pwszTemp = HeapAlloc(hProcessHeap, 0, (cchMaxSubKey + 1) * sizeof(WCHAR));
    if (!pwszTemp)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Determine the required size of the output buffer.
    *pcbNeeded = 0;

    for (i = 0; i < *pcReturned; i++)
    {
        // RegEnumKeyExW sucks! Unlike similar API functions, it only returns the actual numbers of characters copied when you supply a buffer large enough.
        // So use pwszTemp with its size cchMaxSubKey for this.
        cchPrintProcessor = cchMaxSubKey;
        lStatus = RegEnumKeyExW(hSubKey, i, pwszTemp, &cchPrintProcessor, NULL, NULL, NULL, NULL);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %ld!\n", lStatus);
            goto Cleanup;
        }

        *pcbNeeded += sizeof(PRINTPROCESSOR_INFO_1W) + (cchPrintProcessor + 1) * sizeof(WCHAR);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    // Put the Print Processor strings right after the last PRINTPROCESSOR_INFO_1W structure.
    pCurrentOutputPrintProcessorInfo = pPrintProcessorInfo;
    pCurrentOutputPrintProcessor = pPrintProcessorInfo + *pcReturned * sizeof(PRINTPROCESSOR_INFO_1W);

    // Copy over all Print Processors.
    for (i = 0; i < *pcReturned; i++)
    {
        // This isn't really correct, but doesn't cause any harm, because we've extensively checked the size of the supplied buffer above.
        cchPrintProcessor = cchMaxSubKey;

        // Copy the Print Processor name.
        lStatus = RegEnumKeyExW(hSubKey, i, (PWSTR)pCurrentOutputPrintProcessor, &cchPrintProcessor, NULL, NULL, NULL, NULL);
        if (lStatus != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed with status %ld!\n", lStatus);
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
    SetLastError(ERROR_SUCCESS);
    bReturnValue = TRUE;

Cleanup:
    if (pwszTemp)
        HeapFree(hProcessHeap, 0, pwszTemp);

    if (pwszEnvironmentKey)
        HeapFree(hProcessHeap, 0, pwszEnvironmentKey);

    if (hSubKey)
        RegCloseKey(hSubKey);

    if (hKey)
        RegCloseKey(hKey);

    return bReturnValue;
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
 * Alternatively, NULL to output the Print Processor directory of the current environment.
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
LocalGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    const WCHAR wszPath[] = L"\\PRTPROCS\\";
    const DWORD cchPath = sizeof(wszPath) / sizeof(WCHAR) - 1;

    BOOL bReturnValue = FALSE;
    DWORD cbDataWritten;
    HKEY hKey = NULL;
    LONG lStatus;
    PWSTR pwszEnvironmentKey = NULL;

    // Sanity checks
    if (Level != 1)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        goto Cleanup;
    }

    if (!pcbNeeded)
    {
        // This error must be caught by RPC and returned as RPC_X_NULL_REF_POINTER.
        ERR("pcbNeeded is NULL!\n");
        goto Cleanup;
    }

    // Verify pEnvironment and open its registry key.
    if (!_OpenEnvironment(pEnvironment, &hKey))
        goto Cleanup;

    // Determine the size of the required buffer.
    lStatus = RegQueryValueExW(hKey, L"Directory", NULL, NULL, NULL, pcbNeeded);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    *pcbNeeded += cchSpoolDirectory;
    *pcbNeeded += cchPath;

    // Is the supplied buffer large enough?
    if (cbBuf < *pcbNeeded)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    // Copy the path to the "prtprocs" directory into pPrintProcessorInfo
    CopyMemory(pPrintProcessorInfo, wszSpoolDirectory, cchSpoolDirectory * sizeof(WCHAR));
    CopyMemory(&pPrintProcessorInfo[cchSpoolDirectory], wszPath, cchPath * sizeof(WCHAR));

    // Get the directory name from the registry.
    lStatus = RegQueryValueExW(hKey, L"Directory", NULL, NULL, &pPrintProcessorInfo[cchSpoolDirectory + cchPath], &cbDataWritten);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // We've finished successfully!
    SetLastError(ERROR_SUCCESS);
    bReturnValue = TRUE;

Cleanup:
    if (pwszEnvironmentKey)
        HeapFree(hProcessHeap, 0, pwszEnvironmentKey);

    if (hKey)
        RegCloseKey(hKey);

    return bReturnValue;
}
