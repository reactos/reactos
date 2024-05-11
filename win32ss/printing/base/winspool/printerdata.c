/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

LONG WINAPI
AdvancedSetupDialog(HWND hWnd, INT Unknown, PDEVMODEA pDevModeInput, PDEVMODEA pDevModeOutput)
{
   HANDLE hPrinter;
   LONG Ret = -1;

    TRACE("AdvancedSetupDialog(%p, %d, %p, %p)\n", hWnd, Unknown, pDevModeOutput, pDevModeInput);

    if ( OpenPrinterA( (LPSTR)pDevModeInput->dmDeviceName, &hPrinter, NULL ) )
    {
        Ret = AdvancedDocumentPropertiesA( hWnd, hPrinter, (PSTR)pDevModeInput->dmDeviceName, pDevModeOutput, pDevModeInput );
        ClosePrinter(hPrinter);
    }
    return Ret;
}

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
    LPWSTR  valuenameW = NULL;
    INT     len;
    DWORD   res;

    TRACE("DeletePrinterDataA(%p, %s)\n", hPrinter, pValueName);

    if (pValueName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pValueName, -1, NULL, 0);
        valuenameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pValueName, -1, valuenameW, len);
    }

    res = DeletePrinterDataW( hPrinter, valuenameW );

    if (valuenameW) HeapFree(GetProcessHeap(), 0, valuenameW);

    return res;

}

DWORD WINAPI
DeletePrinterDataExA(HANDLE hPrinter, PCSTR pKeyName, PCSTR pValueName)
{
    LPWSTR  keynameW = NULL;
    LPWSTR  valuenameW = NULL;
    INT     len;
    DWORD   res;

    TRACE("DeletePrinterDataExA(%p, %s, %s)\n", hPrinter, pKeyName, pValueName);

    if (pKeyName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, NULL, 0);
        keynameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, keynameW, len);
    }

    if (pValueName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pValueName, -1, NULL, 0);
        valuenameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pValueName, -1, valuenameW, len);
    }

    res = DeletePrinterDataExW( hPrinter, keynameW, valuenameW );

    if (keynameW) HeapFree(GetProcessHeap(), 0, keynameW);
    if (valuenameW) HeapFree(GetProcessHeap(), 0, valuenameW);

    return res;
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
    LPWSTR  keynameW = NULL;
    INT     len;
    DWORD   res;

    TRACE("DeletePrinterKeyA(%p, %s)\n", hPrinter, pKeyName);

    if (pKeyName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, NULL, 0);
        keynameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pKeyName, -1, keynameW, len);
    }

    res = DeletePrinterKeyW( hPrinter, keynameW );

    if (keynameW) HeapFree(GetProcessHeap(), 0, keynameW);

    return res;
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
    INT	    len;
    LPWSTR  pKeyNameW;
    DWORD   ret, dwIndex, dwBufSize;
    HANDLE  hHeap;
    LPSTR   pBuffer;

    TRACE("EnumPrinterDataExA(%p, %s, %p, %lu, %p, %p)\n", hPrinter, pKeyName, pEnumValues, cbEnumValues, pcbEnumValues, pnEnumValues);

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    len = MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, NULL, 0);
    if (len == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %i\n", ret);
	return ret;
    }

    hHeap = GetProcessHeap ();
    if (hHeap == NULL)
    {
	ERR ("GetProcessHeap failed\n");
	return ERROR_OUTOFMEMORY;
    }

    pKeyNameW = HeapAlloc (hHeap, 0, len * sizeof (WCHAR));
    if (pKeyNameW == NULL)
    {
	ERR ("Failed to allocate %i bytes from process heap\n",
             (LONG)(len * sizeof (WCHAR)));
	return ERROR_OUTOFMEMORY;
    }

    if (MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, pKeyNameW, len) == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %i\n", ret);
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	return ret;
    }

    ret = EnumPrinterDataExW (hPrinter, pKeyNameW, pEnumValues, cbEnumValues, pcbEnumValues, pnEnumValues);

    if (ret != ERROR_SUCCESS)
    {
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	TRACE ("EnumPrinterDataExW returned %i\n", ret);
	return ret;
    }

    if (HeapFree (hHeap, 0, pKeyNameW) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	return ret;
    }

    if (*pnEnumValues == 0)	/* empty key */
	return ERROR_SUCCESS;

    dwBufSize = 0;
    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev = &((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	if (dwBufSize < ppev->cbValueName)
	    dwBufSize = ppev->cbValueName;

	if ( dwBufSize < ppev->cbData &&
	    (ppev->dwType == REG_SZ || ppev->dwType == REG_EXPAND_SZ || ppev->dwType == REG_MULTI_SZ))
	    dwBufSize = ppev->cbData;
    }

    FIXME ("Largest Unicode name or value is %i bytes\n", dwBufSize);

    pBuffer = HeapAlloc (hHeap, 0, dwBufSize);
    if (pBuffer == NULL)
    {
	ERR ("Failed to allocate %i bytes from process heap\n", dwBufSize);
	return ERROR_OUTOFMEMORY;
    }

    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev =
		&((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	len = WideCharToMultiByte (CP_ACP, 0, ppev->pValueName,
		ppev->cbValueName / sizeof (WCHAR), pBuffer, dwBufSize, NULL,
		NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %i\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pValueName, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);

	if (ppev->dwType != REG_SZ && ppev->dwType != REG_EXPAND_SZ &&
		ppev->dwType != REG_MULTI_SZ)
	    continue;

	len = WideCharToMultiByte (CP_ACP, 0, (LPWSTR) ppev->pData,
		ppev->cbData / sizeof (WCHAR), pBuffer, dwBufSize, NULL, NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %i\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pData, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);
	TRACE ("  (only first string of REG_MULTI_SZ printed)\n");
    }

    if (HeapFree (hHeap, 0, pBuffer) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	return ret;
    }

    return ERROR_SUCCESS;
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
        if (cbUnicodeData == sizeof(OSVERSIONINFOW) && _wcsicmp(pwszValueName, SPLREG_OS_VERSION) == 0)
        {
            // This is a Unicode OSVERSIONINFOW structure that needs to be converted to an ANSI OSVERSIONINFOA.
            *pcbNeeded = sizeof(OSVERSIONINFOA);
        }
        else if (cbUnicodeData == sizeof(OSVERSIONINFOEXW) && _wcsicmp(pwszValueName, SPLREG_OS_VERSIONEX) == 0)
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
