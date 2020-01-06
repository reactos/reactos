/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#define __ROS_LONG64__

#include <apitest.h>

#include <stdio.h>

#define WIN32_NO_STATUS
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>

#include "../services_apitest.h"

//#define NDEBUG
#include <debug.h>

/**
 * We don't link against winspool, so we don't have GetDefaultPrinterW.
 * We can easily implement a similar function ourselves though.
 */
PWSTR
GetDefaultPrinterFromRegistry(VOID)
{
    static const WCHAR wszWindowsKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
    static const WCHAR wszDeviceValue[] = L"Device";

    DWORD cbNeeded;
    DWORD dwErrorCode;
    HKEY hWindowsKey = NULL;
    PWSTR pwszDevice;
    PWSTR pwszComma;
    PWSTR pwszReturnValue = NULL;

    // Open the registry key where the default printer for the current user is stored.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_CURRENT_USER, wszWindowsKey, 0, KEY_READ, &hWindowsKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        skip("RegOpenKeyExW failed with status %u!\n", dwErrorCode);
        goto Cleanup;
    }

    // Determine the size of the required buffer.
    dwErrorCode = (DWORD)RegQueryValueExW(hWindowsKey, wszDeviceValue, NULL, NULL, NULL, &cbNeeded);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        skip("RegQueryValueExW failed with status %u!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate it.
    pwszDevice = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    if (!pwszDevice)
    {
        skip("HeapAlloc failed!\n");
        goto Cleanup;
    }

    // Now get the actual value.
    dwErrorCode = RegQueryValueExW(hWindowsKey, wszDeviceValue, NULL, NULL, (PBYTE)pwszDevice, &cbNeeded);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        skip("RegQueryValueExW failed with status %u!\n", dwErrorCode);
        goto Cleanup;
    }

    // We get a string "<Printer Name>,winspool,<Port>:".
    // Extract the printer name from it.
    pwszComma = wcschr(pwszDevice, L',');
    if (!pwszComma)
    {
        skip("Found no or invalid default printer: %S!\n", pwszDevice);
        goto Cleanup;
    }

    // Return the default printer.
    *pwszComma = 0;
    pwszReturnValue = pwszDevice;

Cleanup:
    if (hWindowsKey)
        RegCloseKey(hWindowsKey);

    return pwszReturnValue;
}

BOOL
GetLocalsplFuncs(LPPRINTPROVIDOR pp)
{
    HMODULE hLocalspl;
    PInitializePrintProvidor pfnInitializePrintProvidor;

    // Get us a handle to the loaded localspl.dll.
    hLocalspl = GetModuleHandleW(L"localspl");
    if (!hLocalspl)
    {
        skip("GetModuleHandleW failed with error %u!\n", GetLastError());
        return FALSE;
    }

    // Get a pointer to its InitializePrintProvidor function.
    pfnInitializePrintProvidor = (PInitializePrintProvidor)GetProcAddress(hLocalspl, "InitializePrintProvidor");
    if (!pfnInitializePrintProvidor)
    {
        skip("GetProcAddress failed with error %u!\n", GetLastError());
        return FALSE;
    }

    // Get localspl's function pointers.
    if (!pfnInitializePrintProvidor(pp, sizeof(PRINTPROVIDOR), NULL))
    {
        skip("pfnInitializePrintProvidor failed with error %u!\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

PVOID
GetSpoolssFunc(const char* FunctionName)
{
    HMODULE hSpoolss;

    // Get us a handle to the loaded spoolss.dll.
    hSpoolss = GetModuleHandleW(L"spoolss");
    if (!hSpoolss)
    {
        skip("GetModuleHandleW failed with error %u!\n", GetLastError());
        return FALSE;
    }

    return GetProcAddress(hSpoolss, FunctionName);
}
