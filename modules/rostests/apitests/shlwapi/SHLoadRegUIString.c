/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHLoadRegUIStringA/W
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <shlwapi.h>

typedef HRESULT (WINAPI *FN_SHLoadRegUIStringA)(HKEY hkey, LPCSTR value, LPSTR buf, DWORD size);
typedef HRESULT (WINAPI *FN_SHLoadRegUIStringW)(HKEY hkey, LPCWSTR value, LPWSTR buf, DWORD size);

static FN_SHLoadRegUIStringA pSHLoadRegUIStringA = NULL;
static FN_SHLoadRegUIStringW pSHLoadRegUIStringW = NULL;

static void test_SHLoadRegUIStringA(HKEY hKey)
{
    HRESULT hr;
    CHAR szTestValue[MAX_PATH];
    CHAR szBuff[MAX_PATH];

    GetWindowsDirectoryA(szTestValue, _countof(szTestValue));
    lstrcatA(szTestValue, "\\TEST");

    hr = pSHLoadRegUIStringA(hKey, "TestValue1", szBuff, _countof(szBuff));
    ok_long(hr, S_OK);
    ok_str(szBuff, szTestValue);

    hr = pSHLoadRegUIStringA(hKey, "TestValue2", szBuff, _countof(szBuff));
    ok_long(hr, S_OK);
    ok_str(szBuff, "Test string one.");
}

static void test_SHLoadRegUIStringW(HKEY hKey)
{
    HRESULT hr;
    WCHAR szTestValue[MAX_PATH];
    WCHAR szBuff[MAX_PATH];

    GetWindowsDirectoryW(szTestValue, _countof(szTestValue));
    lstrcatW(szTestValue, L"\\TEST");

    hr = pSHLoadRegUIStringW(hKey, L"TestValue1", szBuff, _countof(szBuff));
    ok_long(hr, S_OK);
    ok_wstr(szBuff, L"%WINDIR%\\TEST");

    hr = pSHLoadRegUIStringW(hKey, L"TestValue2", szBuff, _countof(szBuff));
    ok_long(hr, S_OK);
    ok_wstr(szBuff, L"Test string one.");
}

START_TEST(SHLoadRegUIString)
{
    LONG error;
    HKEY hKey;
    DWORD cbValue;
    static const WCHAR s_szTestValue1[] = L"%WINDIR%\\TEST";
    static const WCHAR s_szTestValue2[] = L"@shlwapi_resource_dll.dll%EmptyEnvVar%,-3";
    HMODULE hSHLWAPI;

    SetEnvironmentVariableW(L"EmptyEnvVar", L"");

    /* Get procedures */
    hSHLWAPI = GetModuleHandleW(L"shlwapi");
    pSHLoadRegUIStringA = (FN_SHLoadRegUIStringA)GetProcAddress(hSHLWAPI, (LPCSTR)438);
    pSHLoadRegUIStringW = (FN_SHLoadRegUIStringW)GetProcAddress(hSHLWAPI, (LPCSTR)439);
    if (!pSHLoadRegUIStringA || !pSHLoadRegUIStringW)
    {
        skip("No procedure found\n");
        return;
    }

    /* Open registry key and write some test values */
    error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software", 0, KEY_READ | KEY_WRITE, &hKey);
    ok_long(error, ERROR_SUCCESS);

    cbValue = (lstrlenW(s_szTestValue1) + 1) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, L"TestValue1", 0, REG_SZ, (LPBYTE)s_szTestValue1, cbValue);
    ok_long(error, ERROR_SUCCESS);

    cbValue = (lstrlenW(s_szTestValue2) + 1) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, L"TestValue2", 0, REG_SZ, (LPBYTE)s_szTestValue2, cbValue);
    ok_long(error, ERROR_SUCCESS);

    /* The main dish */
    test_SHLoadRegUIStringA(hKey);
    test_SHLoadRegUIStringW(hKey);

    /* Delete the test values and close the key */
    RegDeleteValueW(hKey, L"TestValue1");
    RegCloseKey(hKey);
}
