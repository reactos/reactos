/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

LONG WINAPI
AdvancedDocumentPropertiesA(HWND hWnd, HANDLE hPrinter, PSTR pDeviceName, PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput)
{
    TRACE("AdvancedDocumentPropertiesA(%p, %p, %s, %p, %p)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput);
    UNIMPLEMENTED;
    return 0;
}

LONG WINAPI
AdvancedDocumentPropertiesW(HWND hWnd, HANDLE hPrinter, PWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput)
{
    TRACE("AdvancedDocumentPropertiesW(%p, %p, %S, %p, %p)\n", hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput);
    UNIMPLEMENTED;
    return 0;
}

DWORD WINAPI
DeletePrinterDataA(HANDLE hPrinter, PSTR pValueName)
{
    TRACE("DeletePrinterDataA(%p, %s)\n", hPrinter, pValueName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
DeletePrinterDataExA(HANDLE hPrinter, PCSTR pKeyName, PCSTR pValueName)
{
    TRACE("DeletePrinterDataExA(%p, %s, %s)\n", hPrinter, pKeyName, pValueName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
DeletePrinterDataExW(HANDLE hPrinter, PCWSTR pKeyName, PCWSTR pValueName)
{
    TRACE("DeletePrinterDataExW(%p, %S, %S)\n", hPrinter, pKeyName, pValueName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
DeletePrinterDataW(HANDLE hPrinter, PWSTR pValueName)
{
    TRACE("DeletePrinterDataW(%p, %S)\n", hPrinter, pValueName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
DeletePrinterKeyA(HANDLE hPrinter, PCSTR pKeyName)
{
    TRACE("DeletePrinterKeyA(%p, %s)\n", hPrinter, pKeyName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
DeletePrinterKeyW(HANDLE hPrinter, PCWSTR pKeyName)
{
    TRACE("DeletePrinterKeyW(%p, %S)\n", hPrinter, pKeyName);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterDataA(HANDLE hPrinter, DWORD dwIndex, PSTR pValueName, DWORD cbValueName, PDWORD pcbValueName, PDWORD pType, PBYTE pData, DWORD cbData, PDWORD pcbData)
{
    TRACE("EnumPrinterDataA(%p, %lu, %s, %lu, %p, %p, %p, %lu, %p)\n", hPrinter, dwIndex, pValueName, cbValueName, pcbValueName, pType, pData, cbData, pcbData);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterDataExA(HANDLE hPrinter, PCSTR pKeyName, PBYTE pEnumValues, DWORD cbEnumValues, PDWORD pcbEnumValues, PDWORD pnEnumValues)
{
    TRACE("EnumPrinterDataExA(%p, %s, %p, %lu, %p, %p)\n", hPrinter, pKeyName, pEnumValues, cbEnumValues, pcbEnumValues, pnEnumValues);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterDataExW(HANDLE hPrinter, PCWSTR pKeyName, PBYTE pEnumValues, DWORD cbEnumValues, PDWORD pcbEnumValues, PDWORD pnEnumValues)
{
    TRACE("EnumPrinterDataExW(%p, %S, %p, %lu, %p, %p)\n", hPrinter, pKeyName, pEnumValues, cbEnumValues, pcbEnumValues, pnEnumValues);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterDataW(HANDLE hPrinter, DWORD dwIndex, PWSTR pValueName, DWORD cbValueName, PDWORD pcbValueName, PDWORD pType, PBYTE pData, DWORD cbData, PDWORD pcbData)
{
    TRACE("EnumPrinterDataW(%p, %lu, %S, %lu, %p, %p, %p, %lu, %p)\n", hPrinter, dwIndex, pValueName, cbValueName, pcbValueName, pType, pData, cbData, pcbData);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterKeyA(HANDLE hPrinter, PCSTR pKeyName, PSTR pSubkey, DWORD cbSubkey, PDWORD pcbSubkey)
{
    TRACE("EnumPrinterKeyA(%p, %s, %s, %lu, %p)\n", hPrinter, pKeyName, pSubkey, cbSubkey, pcbSubkey);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
EnumPrinterKeyW(HANDLE hPrinter, PCWSTR pKeyName, PWSTR pSubkey, DWORD cbSubkey, PDWORD pcbSubkey)
{
    TRACE("EnumPrinterKeyW(%p, %S, %S, %lu, %p)\n", hPrinter, pKeyName, pSubkey, cbSubkey, pcbSubkey);
    UNIMPLEMENTED;
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI
GetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    TRACE("GetPrinterDataA(%p, %s, %p, %p, %lu, %p)\n", hPrinter, pValueName, pType, pData, nSize, pcbNeeded);
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

    TRACE("GetPrinterDataExA(%p, %s, %s, %p, %p, %lu, %p)\n", hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);

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

    TRACE("GetPrinterDataExW(%p, %S, %S, %p, %p, %lu, %p)\n", hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);

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
    TRACE("GetPrinterDataW(%p, %S, %p, %p, %lu, %p)\n", hPrinter, pValueName, pType, pData, nSize, pcbNeeded);
    return GetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD WINAPI
SetPrinterDataA(HANDLE hPrinter, PSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    TRACE("SetPrinterDataA(%p, %s, %lu, %p, %lu)\n", hPrinter, pValueName, Type, pData, cbData);
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

    TRACE("SetPrinterDataExA(%p, %s, %s, %lu, %p, %lu)\n", hPrinter, pKeyName, pValueName, Type, pData, cbData);

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

    TRACE("SetPrinterDataExW(%p, %S, %S, %lu, %p, %lu)\n", hPrinter, pKeyName, pValueName, Type, pData, cbData);

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
    TRACE("SetPrinterDataW(%p, %S, %lu, %p, %lu)\n", hPrinter, pValueName, Type, pData, cbData);
    return SetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, Type, pData, cbData);
}
