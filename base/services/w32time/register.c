/*
 * PROJECT:     ReactOS W32Time Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Service Registration
 * COPYRIGHT:   Copyright 2021 Eric Kohl <ekohl@reactos.org>
 */

#include "w32time.h"
#include <debug.h>
#include <strsafe.h>

#include "resource.h"

static
PWSTR
ReadString(
    _In_ UINT uID)
{
    HINSTANCE hInstance;
    int nLength;
    PWSTR pszString, pszPtr;

    hInstance = GetModuleHandleW(L"w32time");
    if (hInstance == NULL)
        return NULL;

    nLength = LoadStringW(hInstance, uID, (LPWSTR)&pszPtr, 0);
    if (nLength == 0)
        return NULL;

    pszString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
    if (pszString == NULL)
        return NULL;

    wcsncpy(pszString, pszPtr, nLength);

    return pszString;
}


static
HRESULT
RegisterService(VOID)
{
    SC_HANDLE hServiceManager = NULL;
    SC_HANDLE hService = NULL;
    HKEY hKey = NULL;
    PWSTR pszValue;
    PWSTR pszDisplayName = NULL, pszDescription = NULL;
    DWORD dwDisposition, dwError;
    SERVICE_DESCRIPTIONW ServiceDescription;
    HRESULT hresult = S_OK;

    DPRINT("RegisterService()\n");

    hServiceManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hServiceManager == NULL)
    {
        DPRINT1("OpenSCManager() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    pszDisplayName = ReadString(IDS_DISPLAYNAME);
    if (pszDisplayName == NULL)
    {
        DPRINT1("ReadString(IDS_DISPLAYNAME) failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    pszDescription = ReadString(IDS_DESCRIPTION);
    if (pszDescription == NULL)
    {
        DPRINT1("ReadString(IDS_DESCRIPTION) failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    hService = CreateServiceW(hServiceManager,
                              L"W32Time",
                              pszDisplayName,
                              GENERIC_WRITE,
                              SERVICE_WIN32_SHARE_PROCESS,
                              SERVICE_AUTO_START,
                              SERVICE_ERROR_NORMAL,
                              L"%SystemRoot%\\system32\\svchost.exe -k netsvcs",
                              L"Time", NULL, NULL, L"LocalSystem", NULL);
    if (hService == NULL)
    {
        DPRINT1("CreateService() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    ServiceDescription.lpDescription = pszDescription;
    if (ChangeServiceConfig2W(hService,
                              SERVICE_CONFIG_DESCRIPTION,
                              &ServiceDescription) == FALSE)
    {
        DPRINT1("ChangeServiceConfig2() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\W32Time\\Parameters",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyEx() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    pszValue = L"%SystemRoot%\\system32\\w32time.dll";
    dwError = RegSetValueExW(hKey,
                             L"ServiceDll",
                             0,
                             REG_EXPAND_SZ,
                             (LPBYTE)pszValue,
                             (wcslen(pszValue) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

    pszValue = L"SvchostEntry_W32Time";
    dwError = RegSetValueExW(hKey,
                             L"ServiceMain",
                             0,
                             REG_SZ,
                             (LPBYTE)pszValue,
                             (wcslen(pszValue) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx() failed!\n");
        hresult = E_FAIL;
        goto done;
    }

done:
    if (hKey)
        RegCloseKey(hKey);

    if (hService)
        CloseServiceHandle(hService);

    if (hServiceManager)
        CloseServiceHandle(hServiceManager);

    if (pszDescription)
        HeapFree(GetProcessHeap(), 0, pszDescription);

    if (pszDisplayName)
        HeapFree(GetProcessHeap(), 0, pszDisplayName);

    return hresult;
}


static
HRESULT
SetParametersValues(VOID)
{
    HKEY hKey = NULL;
    PWSTR pszValue;
    DWORD dwDisposition, dwError;
    HRESULT hresult = S_OK;

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\W32Time\\Parameters",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyEx() failed!\n");
        goto done;
    }

    pszValue = L"NTP";
    dwError = RegSetValueExW(hKey,
                             L"Type",
                             0,
                             REG_SZ,
                             (LPBYTE)pszValue,
                             (wcslen(pszValue) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx() failed!\n");
        goto done;
    }

done:
    if (hKey)
        RegCloseKey(hKey);

    return hresult;
}


static
HRESULT
SetNtpClientValues(VOID)
{
    HKEY hKey = NULL;
    DWORD dwValue;
    DWORD dwDisposition, dwError;
    HRESULT hresult = S_OK;

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyEx() failed!\n");
        goto done;
    }

    dwValue = 0x00093a80;
    dwError = RegSetValueExW(hKey,
                             L"SpecialPollInterval",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwValue,
                             sizeof(dwValue));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx() failed!\n");
        goto done;
    }

done:
    if (hKey)
        RegCloseKey(hKey);

    return hresult;
}


HRESULT
WINAPI
DllRegisterServer(VOID)
{
    HRESULT hresult;

    hresult = RegisterService();
    if (FAILED(hresult))
        return hresult;

    hresult = SetParametersValues();
    if (FAILED(hresult))
        return hresult;

    hresult = SetNtpClientValues();

    return hresult;
}


HRESULT
WINAPI
DllUnregisterServer(VOID)
{
    DPRINT1("DllUnregisterServer()\n");
    return S_OK;
}
