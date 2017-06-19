/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

LONG WINAPI
AdvancedDocumentPropertiesW(HWND hWnd, HANDLE hPrinter, PWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD WINAPI
GetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD WINAPI
GetPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName, LPCSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    DWORD cbUnicodeData;
    DWORD cch;
    DWORD dwReturnValue;
    DWORD dwType;
    POSVERSIONINFOEXA pInfoA;
    POSVERSIONINFOEXW pInfoW;
    PVOID pUnicodeData = NULL;
    PWSTR pwszKeyName = NULL;
    PWSTR pwszValueName = NULL;

    if (pKeyName)
    {
        // Convert pKeyName to a Unicode string pwszKeyName
        cch = strlen(pKeyName);

        pwszKeyName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszKeyName)
        {
            dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, pwszKeyName, cch + 1);
    }

    if (pValueName)
    {
        // Convert pValueName to a Unicode string pwszValueName
        cch = strlen(pValueName);

        pwszValueName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszValueName)
        {
            dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pValueName, -1, pwszValueName, cch + 1);
    }

    // We need the data type information, even if no pData was passed.
    if (!pType)
        pType = &dwType;

    // Call GetPrinterDataExW for the first time.
    // If we're lucky, the supplied buffer is already large enough and we don't need to do the expensive RPC call a second time.
    dwReturnValue = GetPrinterDataExW(hPrinter, pwszKeyName, pwszValueName, pType, pData, nSize, pcbNeeded);

    // If a critical error occurred, just return it. We cannot do anything else in this case.
    if (dwReturnValue != ERROR_SUCCESS && dwReturnValue != ERROR_MORE_DATA)
        goto Cleanup;

    // Save the needed buffer size for the Unicode data. We may alter *pcbNeeded for an ANSI buffer size.
    cbUnicodeData = *pcbNeeded;

    if (*pType == REG_SZ || *pType == REG_MULTI_SZ || *pType == REG_EXPAND_SZ)
    {
        // This is a string that needs to be converted from Unicode to ANSI.
        // Output the required buffer size for the ANSI string.
        *pcbNeeded /= sizeof(WCHAR);
    }
    else if (*pType == REG_NONE)
    {
        if (cbUnicodeData == sizeof(OSVERSIONINFOW) && wcsicmp(pwszValueName, SPLREG_OS_VERSION) == 0)
        {
            // This is a Unicode OSVERSIONINFOW structure that needs to be converted to an ANSI OSVERSIONINFOA.
            *pcbNeeded = sizeof(OSVERSIONINFOA);
        }
        else if (cbUnicodeData == sizeof(OSVERSIONINFOEXW) && wcsicmp(pwszValueName, SPLREG_OS_VERSIONEX) == 0)
        {
            // This is a Unicode OSVERSIONINFOEXW structure that needs to be converted to an ANSI OSVERSIONINFOEXA.
            *pcbNeeded = sizeof(OSVERSIONINFOEXA);
        }
        else
        {
            // Other REG_NONE value, nothing to do.
            goto Cleanup;
        }
    }

    // Check if the supplied buffer is large enough for the ANSI data.
    if (nSize < *pcbNeeded)
    {
        dwReturnValue = ERROR_MORE_DATA;
        goto Cleanup;
    }

    // Allocate a temporary buffer for the Unicode data.
    pUnicodeData = HeapAlloc(hProcessHeap, 0, cbUnicodeData);
    if (!pUnicodeData)
    {
        dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
        ERR("HeapAlloc failed!\n");
        goto Cleanup;
    }

    if (dwReturnValue == ERROR_SUCCESS)
    {
        // ERROR_SUCCESS: The buffer is large enough for the ANSI and the Unicode string,
        // so the Unicode string has been copied into pData. Copy it to pUnicodeData.
        CopyMemory(pUnicodeData, pData, cbUnicodeData);
    }
    else
    {
        // ERROR_MORE_DATA: The buffer is large enough for the ANSI string, but not for the Unicode string.
        // We have to call GetPrinterDataExW again with the temporary buffer.
        dwReturnValue = GetPrinterDataExW(hPrinter, pwszKeyName, pwszValueName, NULL, (PBYTE)pUnicodeData, cbUnicodeData, &cbUnicodeData);
        if (dwReturnValue != ERROR_SUCCESS)
            goto Cleanup;
    }

    if (*pType == REG_SZ || *pType == REG_MULTI_SZ || *pType == REG_EXPAND_SZ)
    {
        // Convert the Unicode string to ANSI.
        WideCharToMultiByte(CP_ACP, 0, (PWSTR)pUnicodeData, -1, (PSTR)pData, *pcbNeeded, NULL, NULL);
    }
    else
    {
        // This is a REG_NONE with either OSVERSIONINFOW or OSVERSIONINFOEXW.
        // Copy the fields and convert the Unicode CSD Version string to ANSI.
        pInfoW = (POSVERSIONINFOEXW)pUnicodeData;
        pInfoA = (POSVERSIONINFOEXA)pData;
        pInfoA->dwMajorVersion = pInfoW->dwMajorVersion;
        pInfoA->dwMinorVersion = pInfoW->dwMinorVersion;
        pInfoA->dwBuildNumber = pInfoW->dwBuildNumber;
        pInfoA->dwPlatformId = pInfoW->dwPlatformId;
        WideCharToMultiByte(CP_ACP, 0, pInfoW->szCSDVersion, -1, pInfoA->szCSDVersion, sizeof(pInfoA->szCSDVersion), NULL, NULL);

        if (cbUnicodeData == sizeof(OSVERSIONINFOW))
        {
            pInfoA->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        }
        else
        {
            pInfoA->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
            pInfoA->wServicePackMajor = pInfoW->wServicePackMajor;
            pInfoA->wServicePackMinor = pInfoW->wServicePackMinor;
            pInfoA->wSuiteMask = pInfoW->wSuiteMask;
            pInfoA->wProductType = pInfoW->wProductType;
            pInfoA->wReserved = pInfoW->wReserved;
        }
    }

Cleanup:
    if (pwszKeyName)
        HeapFree(hProcessHeap, 0, pwszKeyName);

    if (pwszValueName)
        HeapFree(hProcessHeap, 0, pwszValueName);

    if (pUnicodeData)
        HeapFree(hProcessHeap, 0, pUnicodeData);

    return dwReturnValue;
}

DWORD WINAPI
GetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName, LPCWSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    const WCHAR wszEmptyString[] = L"";

    BYTE DummyData;
    DWORD dwErrorCode;
    DWORD dwType = REG_NONE;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    // Sanity checks
    if (!pHandle)
        return ERROR_INVALID_HANDLE;

    // Yes, instead of declaring these pointers unique in the IDL file (and perfectly accepting NULL pointers this way),
    // Windows does it differently for GetPrinterDataExW and points them to empty variables.
    if (!pKeyName)
        pKeyName = wszEmptyString;

    if (!pType)
        pType = &dwType;

    if (!pData && !nSize)
        pData = &DummyData;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrinterDataEx(pHandle->hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwErrorCode;
}

DWORD WINAPI
GetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD WINAPI
SetPrinterDataA(HANDLE hPrinter, PSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    return SetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, Type, pData, cbData);
}

DWORD WINAPI
SetPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName, LPCSTR pValueName, DWORD Type, LPBYTE pData, DWORD cbData)
{
    DWORD cch;
    DWORD dwReturnValue;
    PWSTR pwszKeyName = NULL;
    PWSTR pwszValueName = NULL;
    PWSTR pUnicodeData = NULL;

    if (pKeyName)
    {
        // Convert pKeyName to a Unicode string pwszKeyName
        cch = strlen(pKeyName);

        pwszKeyName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszKeyName)
        {
            dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, pwszKeyName, cch + 1);
    }

    if (pValueName)
    {
        // Convert pValueName to a Unicode string pwszValueName
        cch = strlen(pValueName);

        pwszValueName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszValueName)
        {
            dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pValueName, -1, pwszValueName, cch + 1);
    }

    if (Type == REG_SZ || Type == REG_MULTI_SZ || Type == REG_EXPAND_SZ)
    {
        // Convert pData to a Unicode string pUnicodeData.
        pUnicodeData = HeapAlloc(hProcessHeap, 0, cbData * sizeof(WCHAR));
        if (!pUnicodeData)
        {
            dwReturnValue = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, (PCSTR)pData, -1, pUnicodeData, cbData);

        pData = (PBYTE)pUnicodeData;
        cbData *= sizeof(WCHAR);
    }

    dwReturnValue = SetPrinterDataExW(hPrinter, pwszKeyName, pwszValueName, Type, pData, cbData);

Cleanup:
    if (pwszKeyName)
        HeapFree(hProcessHeap, 0, pwszKeyName);

    if (pwszValueName)
        HeapFree(hProcessHeap, 0, pwszValueName);

    if (pUnicodeData)
        HeapFree(hProcessHeap, 0, pUnicodeData);

    return dwReturnValue;
}

DWORD WINAPI
SetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName, LPCWSTR pValueName, DWORD Type, LPBYTE pData, DWORD cbData)
{
    const WCHAR wszEmptyString[] = L"";

    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    // Sanity checks
    if (!pHandle)
        return ERROR_INVALID_HANDLE;

    if (!pKeyName)
        pKeyName = wszEmptyString;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSetPrinterDataEx(pHandle->hPrinter, pKeyName, pValueName, Type, pData, cbData);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwErrorCode;
}

DWORD WINAPI
SetPrinterDataW(HANDLE hPrinter, PWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    return SetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, Type, pData, cbData);
}
