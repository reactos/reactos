/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD WINAPI
LocalGetPrinterData(HANDLE hPrinter, PWSTR pValueName, PDWORD pType, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    TRACE("LocalGetPrinterData(%p, %S, %p, %p, %lu, %p)\n", hPrinter, pValueName, pType, pData, nSize, pcbNeeded);

    // The ReactOS Printing Stack forwards all GetPrinterData calls to GetPrinterDataEx as soon as possible.
    // This function may only be called if localspl.dll is used together with Windows Printing Stack components.
    WARN("This function should never be called!\n");
    return LocalGetPrinterDataEx(hPrinter, L"PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

static DWORD
_MakePrinterSubKey(PLOCAL_PRINTER_HANDLE pPrinterHandle, PCWSTR pKeyName, PWSTR* ppwszSubKey)
{
    const WCHAR wszBackslash[] = L"\\";

    size_t cbSubKey;
    PWSTR p;

    // Sanity check
    if (!pKeyName || !*pKeyName)
        return ERROR_INVALID_PARAMETER;

    // Allocate a buffer for the subkey "PrinterName\KeyName".
    cbSubKey = (wcslen(pPrinterHandle->pPrinter->pwszPrinterName) + 1 + wcslen(pKeyName) + 1) * sizeof(WCHAR);
    *ppwszSubKey = DllAllocSplMem(cbSubKey);
    if (!*ppwszSubKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Concatenate the subkey.
    p = *ppwszSubKey;
    StringCbCopyExW(p, cbSubKey, pPrinterHandle->pPrinter->pwszPrinterName, &p, &cbSubKey, 0);
    StringCbCopyExW(p, cbSubKey, wszBackslash, &p, &cbSubKey, 0);
    StringCbCopyExW(p, cbSubKey, pKeyName, &p, &cbSubKey, 0);

    return ERROR_SUCCESS;
}

static DWORD
_LocalGetPrinterHandleData(PLOCAL_PRINTER_HANDLE pPrinterHandle, PCWSTR pKeyName, PCWSTR pValueName, PDWORD pType, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    HKEY hKey = NULL;
    PWSTR pwszSubKey = NULL;

    dwErrorCode = _MakePrinterSubKey(pPrinterHandle, pKeyName, &pwszSubKey);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Open the subkey.
    dwErrorCode = (DWORD)RegOpenKeyExW(hPrintersKey, pwszSubKey, 0, KEY_READ, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed for \"%S\" with error %lu!\n", pwszSubKey, dwErrorCode);
        goto Cleanup;
    }

    // Query the desired value.
    *pcbNeeded = nSize;
    dwErrorCode = (DWORD)RegQueryValueExW(hKey, pValueName, NULL, pType, pData, pcbNeeded);

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    if (pwszSubKey)
        DllFreeSplMem(pwszSubKey);

    return dwErrorCode;
}

static DWORD
_LocalGetPrintServerHandleData(PCWSTR pValueName, PDWORD pType, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;

    if (_wcsicmp(pValueName, SPLREG_DEFAULT_SPOOL_DIRECTORY) == 0 ||
        _wcsicmp(pValueName, SPLREG_PORT_THREAD_PRIORITY) == 0 ||
        _wcsicmp(pValueName, SPLREG_SCHEDULER_THREAD_PRIORITY) == 0 ||
        _wcsicmp(pValueName, SPLREG_BEEP_ENABLED) == 0 ||
        _wcsicmp(pValueName, SPLREG_ALLOW_USER_MANAGEFORMS) == 0)
    {
        *pcbNeeded = nSize;
        return (DWORD)RegQueryValueExW(hPrintersKey, pValueName, NULL, pType, pData, pcbNeeded);
    }
    else if (_wcsicmp(pValueName, SPLREG_PORT_THREAD_PRIORITY_DEFAULT) == 0 ||
        _wcsicmp(pValueName, SPLREG_SCHEDULER_THREAD_PRIORITY_DEFAULT) == 0)
    {
        // Store a DWORD value as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Apparently, these values don't serve a purpose anymore.
        *((PDWORD)pData) = 0;
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_NET_POPUP) == 0 ||
        _wcsicmp(pValueName, SPLREG_RETRY_POPUP) == 0 ||
        _wcsicmp(pValueName, SPLREG_NET_POPUP_TO_COMPUTER) == 0 ||
        _wcsicmp(pValueName, SPLREG_EVENT_LOG) == 0 ||
        _wcsicmp(pValueName, SPLREG_RESTART_JOB_ON_POOL_ERROR) == 0 ||
        _wcsicmp(pValueName, SPLREG_RESTART_JOB_ON_POOL_ENABLED) == 0)
    {
        HKEY hKey;

        dwErrorCode = (DWORD)RegOpenKeyExW(hPrintKey, L"Providers", 0, KEY_READ, &hKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for \"Providers\" with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        *pcbNeeded = nSize;
        dwErrorCode = (DWORD)RegQueryValueExW(hKey, pValueName, NULL, pType, pData, pcbNeeded);
        RegCloseKey(hKey);
        return dwErrorCode;
    }
    else if (_wcsicmp(pValueName, SPLREG_MAJOR_VERSION) == 0)
    {
        // Store a DWORD value as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Apparently, these values don't serve a purpose anymore.
        *((PDWORD)pData) = dwSpoolerMajorVersion;
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_MINOR_VERSION) == 0)
    {
        // Store a DWORD value as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Apparently, these values don't serve a purpose anymore.
        *((PDWORD)pData) = dwSpoolerMinorVersion;
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_ARCHITECTURE) == 0)
    {
        // Store a string as REG_NONE with the length of the environment name string.
        *pType = REG_NONE;
        *pcbNeeded = cbCurrentEnvironment;
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Copy the environment name as the output value for SPLREG_ARCHITECTURE.
        CopyMemory(pData, wszCurrentEnvironment, cbCurrentEnvironment);
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_OS_VERSION) == 0)
    {
        POSVERSIONINFOW pInfo = (POSVERSIONINFOW)pData;

        // Store the OSVERSIONINFOW structure as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(OSVERSIONINFOW);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Return OS version information.
        pInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        GetVersionExW(pInfo);
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_OS_VERSIONEX) == 0)
    {
        POSVERSIONINFOEXW pInfo = (POSVERSIONINFOEXW)pData;

        // Store the OSVERSIONINFOEXW structure as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(OSVERSIONINFOEXW);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Return extended OS version information.
        pInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        GetVersionExW((POSVERSIONINFOW)pInfo);
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_DS_PRESENT) == 0)
    {
        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo;

        // We want to store a REG_DWORD value.
        *pType = REG_DWORD;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Get information about the domain membership of this computer.
        dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE*)&pInfo);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("DsRoleGetPrimaryDomainInformation failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        // Return whether this computer is a workstation or server inside a domain.
        *((PDWORD)pData) = (pInfo->MachineRole == DsRole_RoleMemberWorkstation || pInfo->MachineRole == DsRole_RoleMemberServer);
        DsRoleFreeMemory(pInfo);
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_DS_PRESENT_FOR_USER) == 0)
    {
        DWORD cch;
        PWSTR p;
        WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        WCHAR wszUserSam[UNLEN + 1];

        // We want to store a REG_DWORD value.
        *pType = REG_DWORD;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Get the local Computer Name.
        cch = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameW(wszComputerName, &cch))
        {
            dwErrorCode = GetLastError();
            ERR("GetComputerNameW failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        // Get the User Name in the SAM format.
        // This could either be:
        //     COMPUTERNAME\User
        //     DOMAINNAME\User
        cch = UNLEN + 1;
        if (!GetUserNameExW(NameSamCompatible, wszUserSam, &cch))
        {
            dwErrorCode = GetLastError();
            ERR("GetUserNameExW failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        // Terminate the SAM-formatted User Name at the backslash.
        p = wcschr(wszUserSam, L'\\');
        *p = 0;

        // Compare it with the Computer Name.
        // If they differ, this User is part of a domain.
        *((PDWORD)pData) = (wcscmp(wszUserSam, wszComputerName) != 0);
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_REMOTE_FAX) == 0)
    {
        // Store a DWORD value as REG_NONE.
        *pType = REG_NONE;
        *pcbNeeded = sizeof(DWORD);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // TODO: We don't support any fax service yet, but let's return the same value as Windows Server 2003 here.
        *((PDWORD)pData) = 1;
        return ERROR_SUCCESS;
    }
    else if (_wcsicmp(pValueName, SPLREG_DNS_MACHINE_NAME) == 0)
    {
        DWORD cchDnsName = 0;

        // Get the length of the fully-qualified computer DNS name.
        GetComputerNameExW(ComputerNameDnsFullyQualified, NULL, &cchDnsName);
        dwErrorCode = GetLastError();
        if (dwErrorCode != ERROR_MORE_DATA)
        {
            ERR("GetComputerNameExW failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        // Check if our supplied buffer is large enough.
        *pType = REG_SZ;
        *pcbNeeded = cchDnsName * sizeof(WCHAR);
        if (nSize < *pcbNeeded)
            return ERROR_MORE_DATA;

        // Get the actual DNS name.
        if (!GetComputerNameExW(ComputerNameDnsFullyQualified, (PWSTR)pData, &cchDnsName))
        {
            dwErrorCode = GetLastError();
            ERR("GetComputerNameExW failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        // Lowercase the output just like Windows does.
        _wcslwr((PWSTR)pData);
        return ERROR_SUCCESS;
    }
    else
    {
        // For all other, unknown settings, we just return ERROR_INVALID_PARAMETER.
        // That also includes SPLREG_WEBSHAREMGMT, which is supported in Windows Server 2003 according to the documentation,
        // but is actually not!
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD WINAPI
LocalGetPrinterDataEx(HANDLE hPrinter, PCWSTR pKeyName, PCWSTR pValueName, PDWORD pType, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    BYTE Temp;
    DWORD dwErrorCode;
    DWORD dwTemp;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;

    TRACE("LocalGetPrinterDataEx(%p, %S, %S, %p, %p, %lu, %p)\n", hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);

    // Even if GetPrinterDataExW in winspool ensures that the RPC function is never called without a valid pointer for pType,
    // it's officially optional. Windows' fpGetPrinterDataEx also works with NULL for pType!
    // Ensure here that it is always set to simplify the code later.
    if (!pType)
        pType = &dwTemp;

    // pData is later fed to RegQueryValueExW in many cases. When calling it with zero buffer size, RegQueryValueExW returns a
    // different error code based on whether pData is NULL or something else.
    // Ensure here that ERROR_MORE_DATA is always returned.
    if (!pData)
        pData = &Temp;

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }
    else if (!pcbNeeded)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
    }
    else if (pHandle->HandleType == HandleType_Printer)
    {
        dwErrorCode = _LocalGetPrinterHandleData(pHandle->pSpecificHandle, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);
    }
    else if (pHandle->HandleType == HandleType_PrintServer)
    {
        dwErrorCode = _LocalGetPrintServerHandleData(pValueName, pType, pData, nSize, pcbNeeded);
    }
    else
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }

    SetLastError(dwErrorCode);
    return dwErrorCode;
}

DWORD WINAPI
LocalSetPrinterData(HANDLE hPrinter, PWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    TRACE("LocalSetPrinterData(%p, %S, %lu, %p, %lu)\n", hPrinter, pValueName, Type, pData, cbData);

    // The ReactOS Printing Stack forwards all SetPrinterData calls to SetPrinterDataEx as soon as possible.
    // This function may only be called if localspl.dll is used together with Windows Printing Stack components.
    WARN("This function should never be called!\n");
    return LocalSetPrinterDataEx(hPrinter, L"PrinterDriverData", pValueName, Type, pData, cbData);
}

static DWORD
_LocalSetPrinterHandleData(PLOCAL_PRINTER_HANDLE pPrinterHandle, PCWSTR pKeyName, PCWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    DWORD dwErrorCode;
    HKEY hKey = NULL;
    PWSTR pwszSubKey = NULL;

    dwErrorCode = _MakePrinterSubKey(pPrinterHandle, pKeyName, &pwszSubKey);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Open the subkey.
    dwErrorCode = (DWORD)RegOpenKeyExW(hPrintersKey, pwszSubKey, 0, KEY_SET_VALUE, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed for \"%S\" with error %lu!\n", pwszSubKey, dwErrorCode);
        goto Cleanup;
    }

    // Set the value.
    dwErrorCode = (DWORD)RegSetValueExW(hKey, pValueName, 0, Type, pData, cbData);

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    if (pwszSubKey)
        DllFreeSplMem(pwszSubKey);

    return dwErrorCode;
}

static DWORD
_LocalSetPrintServerHandleData(PCWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    DWORD dwErrorCode;

    if (_wcsicmp(pValueName, SPLREG_DEFAULT_SPOOL_DIRECTORY) == 0 ||
        _wcsicmp(pValueName, SPLREG_PORT_THREAD_PRIORITY) == 0 ||
        _wcsicmp(pValueName, SPLREG_SCHEDULER_THREAD_PRIORITY) == 0 ||
        _wcsicmp(pValueName, SPLREG_BEEP_ENABLED) == 0 ||
        _wcsicmp(pValueName, SPLREG_ALLOW_USER_MANAGEFORMS) == 0)
    {
        return (DWORD)RegSetValueExW(hPrintersKey, pValueName, 0, Type, pData, cbData);
    }
    else if (_wcsicmp(pValueName, SPLREG_NET_POPUP) == 0 ||
        _wcsicmp(pValueName, SPLREG_RETRY_POPUP) == 0 ||
        _wcsicmp(pValueName, SPLREG_NET_POPUP_TO_COMPUTER) == 0 ||
        _wcsicmp(pValueName, SPLREG_EVENT_LOG) == 0 ||
        _wcsicmp(pValueName, SPLREG_RESTART_JOB_ON_POOL_ERROR) == 0 ||
        _wcsicmp(pValueName, SPLREG_RESTART_JOB_ON_POOL_ENABLED) == 0 ||
        _wcsicmp(pValueName, L"NoRemotePrinterDrivers") == 0)
    {
        HKEY hKey;

        dwErrorCode = (DWORD)RegOpenKeyExW(hPrintKey, L"Providers", 0, KEY_SET_VALUE, &hKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for \"Providers\" with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }

        dwErrorCode = (DWORD)RegSetValueExW(hKey, pValueName, 0, Type, pData, cbData);
        RegCloseKey(hKey);
        return dwErrorCode;
    }
    else if (_wcsicmp(pValueName, SPLREG_WEBSHAREMGMT) == 0)
    {
        WARN("Attempting to set WebShareMgmt, which is based on IIS and therefore not supported. Returning fake success!\n");
        return ERROR_SUCCESS;
    }
    else
    {
        // For all other, unknown settings, we just return ERROR_INVALID_PARAMETER.
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD WINAPI
LocalSetPrinterDataEx(HANDLE hPrinter, PCWSTR pKeyName, PCWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    DWORD dwErrorCode;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hPrinter;

    TRACE("LocalSetPrinterDataEx(%p, %S, %S, %lu, %p, %lu)\n", hPrinter, pKeyName, pValueName, Type, pData, cbData);

    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }
    else if (pHandle->HandleType == HandleType_Printer)
    {
        dwErrorCode = _LocalSetPrinterHandleData(pHandle->pSpecificHandle, pKeyName, pValueName, Type, pData, cbData);
    }
    else if (pHandle->HandleType == HandleType_PrintServer)
    {
        dwErrorCode = _LocalSetPrintServerHandleData(pValueName, Type, pData, cbData);
    }
    else
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }

    SetLastError(dwErrorCode);
    return dwErrorCode;
}
