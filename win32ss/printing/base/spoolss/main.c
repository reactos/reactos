/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
HANDLE hProcessHeap;
LIST_ENTRY PrintProviderList;


static DWORD
_AddPrintProviderToList(PCWSTR pwszFileName)
{
    DWORD dwErrorCode;
    HINSTANCE hinstPrintProvider;
    PInitializePrintProvidor pfnInitializePrintProvidor;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider = NULL;

    // Try to load it.
    hinstPrintProvider = LoadLibraryW(pwszFileName);
    if (!hinstPrintProvider)
    {
        dwErrorCode = GetLastError();
        ERR("LoadLibraryW failed for \"%S\" with error %lu!\n", pwszFileName, dwErrorCode);
        goto Cleanup;
    }

    // Get the initialization routine.
    pfnInitializePrintProvidor = (PInitializePrintProvidor)GetProcAddress(hinstPrintProvider, "InitializePrintProvidor");
    if (!pfnInitializePrintProvidor)
    {
        dwErrorCode = GetLastError();
        ERR("GetProcAddress failed for \"%S\" with error %lu!\n", pwszFileName, dwErrorCode);
        goto Cleanup;
    }

    // Create a new SPOOLSS_PRINT_PROVIDER structure for it.
    pPrintProvider = DllAllocSplMem(sizeof(SPOOLSS_PRINT_PROVIDER));
    if (!pPrintProvider)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    // Call the Print Provider initialization function.
    if (!pfnInitializePrintProvidor(&pPrintProvider->PrintProvider, sizeof(PRINTPROVIDOR), NULL))
    {
        dwErrorCode = GetLastError();
        ERR("InitializePrintProvidor failed for \"%S\" with error %lu!\n", pwszFileName, dwErrorCode);
        goto Cleanup;
    }

    // Add this Print Provider to the list.
    InsertTailList(&PrintProviderList, &pPrintProvider->Entry);

    // Don't let the cleanup routine free this.
    pPrintProvider = NULL;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pPrintProvider)
        DllFreeSplMem(pPrintProvider);

    return dwErrorCode;
}

static BOOL
_InitializePrintProviderList()
{
    DWORD cbFileName;
    DWORD cchMaxSubKey;
    DWORD cchPrintProviderName;
    DWORD dwErrorCode;
    DWORD dwSubKeys;
    DWORD i;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    PWSTR pwszPrintProviderName = NULL;
    WCHAR wszFileName[MAX_PATH];

    // Initialize an empty list for our Print Providers.
    InitializeListHead(&PrintProviderList);

    // First add the Local Spooler.
    // This one must exist and must be the first one in the list.
    dwErrorCode = _AddPrintProviderToList(L"localspl");
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("The Local Spooler could not be loaded!\n");
        goto Cleanup;
    }

    // Now add additional Print Providers from the registry.
    // First of all, open the key containing print providers.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers", 0, KEY_READ, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of Print Providers and maximum sub key length.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwSubKeys, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate a temporary buffer for the Print Provider names.
    pwszPrintProviderName = DllAllocSplMem((cchMaxSubKey + 1) * sizeof(WCHAR));
    if (!pwszPrintProviderName)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    // Loop through all available Print Providers.
    for (i = 0; i < dwSubKeys; i++)
    {
        // Cleanup tasks from the previous run
        if (hSubKey)
        {
            RegCloseKey(hSubKey);
            hSubKey = NULL;
        }

        // Get the name of this Print Provider.
        cchPrintProviderName = cchMaxSubKey + 1;
        dwErrorCode = (DWORD)RegEnumKeyExW(hKey, i, pwszPrintProviderName, &cchPrintProviderName, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed for iteration %lu with status %lu!\n", i, dwErrorCode);
            continue;
        }

        // Open this Print Provider's registry key.
        dwErrorCode = (DWORD)RegOpenKeyExW(hKey, pwszPrintProviderName, 0, KEY_READ, &hSubKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Print Provider \"%S\" with status %lu!\n", pwszPrintProviderName, dwErrorCode);
            continue;
        }

        // Get the file name of the Print Provider.
        cbFileName = MAX_PATH * sizeof(WCHAR);
        dwErrorCode = (DWORD)RegQueryValueExW(hKey, L"Driver", NULL, NULL, (PBYTE)wszFileName, &cbFileName);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed with status %lu!\n", dwErrorCode);
            continue;
        }

        // Load and add it to the list.
        dwErrorCode = _AddPrintProviderToList(wszFileName);
        if (dwErrorCode != ERROR_SUCCESS)
            continue;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (hSubKey)
        RegCloseKey(hSubKey);

    // Outside the loop
    if (pwszPrintProviderName)
        DllFreeSplMem(pwszPrintProviderName);

    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			hProcessHeap = GetProcessHeap();
            break;
    }

    return TRUE;
}

BOOL WINAPI
InitializeRouter(HANDLE SpoolerStatusHandle)
{
    return _InitializePrintProviderList();
}

BOOL WINAPI
SplInitializeWinSpoolDrv(PVOID* pTable)
{
    HINSTANCE hWinspool;
    int i;

    hWinspool = LoadLibraryW(L"winspool.drv");
    if (!hWinspool)
    {
        ERR("Could not load winspool.drv, last error is %lu!\n", GetLastError());
        return FALSE;
    }

    // Get the function pointers which are meant to be returned by this function.
    pTable[0] = GetProcAddress(hWinspool, "OpenPrinterW");
    pTable[1] = GetProcAddress(hWinspool, "ClosePrinter");
    pTable[2] = GetProcAddress(hWinspool, "SpoolerDevQueryPrintW");
    pTable[3] = GetProcAddress(hWinspool, "SpoolerPrinterEvent");
    pTable[4] = GetProcAddress(hWinspool, "DocumentPropertiesW");
    pTable[5] = GetProcAddress(hWinspool, (LPSTR)212);
    pTable[6] = GetProcAddress(hWinspool, (LPSTR)213);
    pTable[7] = GetProcAddress(hWinspool, (LPSTR)214);
    pTable[8] = GetProcAddress(hWinspool, (LPSTR)215);

    // Verify that all calls succeeded.
    for (i = 0; i < 9; i++)
        if (!pTable[i])
            return FALSE;

    return TRUE;
}

BOOL WINAPI
SplIsUpgrade()
{
	return FALSE;
}

BOOL WINAPI
SpoolerInit()
{
    // Nothing to do here yet
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI
BuildOtherNamesFromMachineName(LPVOID * ptr1, LPVOID * ptr2)
{
    FIXME("(%p, %p) stub\n", ptr1, ptr2);

    *ptr1 = NULL;
    *ptr2 = NULL;
    return FALSE;
}

