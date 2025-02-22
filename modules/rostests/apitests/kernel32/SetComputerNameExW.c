/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for the SetComputerNameExW API
 * COPYRIGHT:   Victor Martinez Calvo (victor.martinez@reactos.org)
 *              Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/rtltypes.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winreg.h>

static HKEY OpenHostNameKey(void)
{
    static const WCHAR
        RegHostNameKey[] = L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    HKEY hKey = NULL;
    LONG Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, RegHostNameKey, 0, KEY_ALL_ACCESS, &hKey);
    if (!Error)
        return hKey;
    return NULL;
}

static HKEY OpenComputerNameKey(void)
{
    static const WCHAR
        RegComputerNameKey[] = L"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName";
    HKEY hKey = NULL;
    LONG Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, RegComputerNameKey, 0, KEY_ALL_ACCESS, &hKey);
    if (!Error)
        return hKey;
    return NULL;
}

static void DoTestComputerName(HKEY hKeyHN, HKEY hKeyCN, LPCWSTR pszNewName, BOOL bValid)
{
    LONG Error;
    BOOL ret;
    DWORD cbData;
    WCHAR szNVHostNameOld[MAX_PATH], szNVHostNameNew[MAX_PATH];
    WCHAR szHostNameOld[MAX_PATH], szHostNameNew[MAX_PATH];
    WCHAR szComputerNameOld[MAX_PATH], szComputerNameNew[MAX_PATH];

    trace("Testing '%S':\n", pszNewName);

    /* Get Old NV Hostname */
    szNVHostNameOld[0] = UNICODE_NULL;
    cbData = sizeof(szNVHostNameOld);
    Error = RegQueryValueExW(hKeyHN, L"NV Hostname", NULL, NULL, (LPBYTE)szNVHostNameOld, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szNVHostNameOld[0], "szNVHostNameOld is empty\n");

    /* Get Old Hostname */
    szHostNameOld[0] = UNICODE_NULL;
    cbData = sizeof(szHostNameOld);
    Error = RegQueryValueExW(hKeyHN, L"Hostname", NULL, NULL, (LPBYTE)szHostNameOld, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szHostNameOld[0], "szHostNameOld is empty\n");

    /* Get Old Computer Name */
    szComputerNameOld[0] = UNICODE_NULL;
    cbData = sizeof(szComputerNameOld);
    Error = RegQueryValueExW(hKeyCN, L"ComputerName", NULL, NULL, (LPBYTE)szComputerNameOld, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szComputerNameOld[0], "szComputerNameOld is empty\n");

    /* Change the value */
    SetLastError(0xDEADFACE);
    ret = SetComputerNameExW(ComputerNamePhysicalDnsHostname, pszNewName);
    ok_int(ret, bValid);
    Error = GetLastError();
    if (bValid)
        ok_long(Error, ERROR_SUCCESS);
    else
        ok_long(Error, ERROR_INVALID_PARAMETER);

    /* Get New NV Hostname */
    szNVHostNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szNVHostNameNew);
    Error = RegQueryValueExW(hKeyHN, L"NV Hostname", NULL, NULL, (LPBYTE)szNVHostNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    if (bValid)
    {
        ok(szNVHostNameNew[0], "szNVHostNameNew is empty\n");
        ok(lstrcmpW(szNVHostNameNew, pszNewName) == 0,
           "szNVHostNameNew '%S' should be pszNewName '%S'\n", szNVHostNameNew, pszNewName);
    }

    /* Get New Hostname */
    szHostNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szHostNameNew);
    Error = RegQueryValueExW(hKeyHN, L"Hostname", NULL, NULL, (LPBYTE)szHostNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    if (bValid)
    {
        ok(szHostNameNew[0], "szHostNameNew is empty\n");
        ok(lstrcmpW(szHostNameNew, szHostNameOld) == 0,
           "szHostNameNew '%S' should be szHostNameOld '%S'\n", szHostNameNew, szHostNameOld);
    }

    /* Get New Computer Name */
    szComputerNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szComputerNameNew);
    Error = RegQueryValueExW(hKeyCN, L"ComputerName", NULL, NULL, (LPBYTE)szComputerNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    if (bValid)
    {
        ok(szComputerNameNew[0], "szComputerNameNew is empty\n");
        if (lstrlenW(pszNewName) > MAX_COMPUTERNAME_LENGTH)
        {
            WCHAR szTruncatedNewName[MAX_COMPUTERNAME_LENGTH + 1];
            lstrcpynW(szTruncatedNewName, pszNewName, ARRAYSIZE(szTruncatedNewName));
            ok(lstrcmpiW(szComputerNameNew, szTruncatedNewName) == 0,
               "szComputerNameNew '%S' should be szTruncatedNewName '%S'\n",
               szComputerNameNew, szTruncatedNewName);
        }
        else
        {
            ok(lstrcmpiW(szComputerNameNew, pszNewName) == 0,
               "szComputerNameNew '%S' should be pszNewName '%S'\n",
               szComputerNameNew, pszNewName);
        }
    }

    /* Restore the registry values */
    cbData = (lstrlenW(szNVHostNameOld) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKeyHN, L"NV Hostname", 0, REG_SZ, (LPBYTE)szNVHostNameOld, cbData);
    ok_long(Error, ERROR_SUCCESS);

    cbData = (lstrlenW(szHostNameOld) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKeyHN, L"Hostname", 0, REG_SZ, (LPBYTE)szHostNameOld, cbData);
    ok_long(Error, ERROR_SUCCESS);

    cbData = (lstrlenW(szComputerNameOld) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKeyCN, L"ComputerName", 0, REG_SZ, (LPBYTE)szComputerNameOld, cbData);
    ok_long(Error, ERROR_SUCCESS);
}

START_TEST(SetComputerNameExW)
{
    HKEY hKeyHN, hKeyCN;
    static const WCHAR ValidSymbols[] = L"-_";
    static const WCHAR InvalidSymbols[] = L"\"/\\[]:|<>+=;,?";
    WCHAR szName[32];
    INT i, cchValidSymbols, cchInvalidSymbols;
    LONG Error;

    /* Open keys */
    hKeyHN = OpenHostNameKey();
    ok(hKeyHN != NULL, "hKeyHN is NULL\n");
    hKeyCN = OpenComputerNameKey();
    ok(hKeyCN != NULL, "hKeyCN is NULL\n");
    if (!hKeyHN || !hKeyCN)
    {
        if (hKeyHN)
            RegCloseKey(hKeyHN);
        if (hKeyCN)
            RegCloseKey(hKeyCN);
        skip("Unable to open keys. Missing Admin rights?\n");
        return;
    }

    cchValidSymbols = lstrlenW(ValidSymbols);
    cchInvalidSymbols = lstrlenW(InvalidSymbols);

    /* Test names */

    DoTestComputerName(hKeyHN, hKeyCN, L"SRVROSTEST", TRUE);
    DoTestComputerName(hKeyHN, hKeyCN, L"SrvRosTest", TRUE);

    DoTestComputerName(hKeyHN, hKeyCN, L"", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"a", TRUE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A", TRUE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1", FALSE);   // numeric-only
    DoTestComputerName(hKeyHN, hKeyCN, L"@", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L".", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L" ", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\t", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\b", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L" a", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L" A", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L" 1", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\ta", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\tA", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\t1", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\ba", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\bA", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"\b1", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"a ", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A ", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1 ", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"a\t", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A\t", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1\t", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"a\b", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A\b", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1\b", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"a c", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A C", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1 c", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"a\tc", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A\tC", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1\tc", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"a\bc", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"A\bC", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"1\bc", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, ValidSymbols, TRUE);
    DoTestComputerName(hKeyHN, hKeyCN, InvalidSymbols, FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"123456", FALSE);   // numeric-only
    DoTestComputerName(hKeyHN, hKeyCN, L"123.456", FALSE);
    DoTestComputerName(hKeyHN, hKeyCN, L"123X456", TRUE);
    DoTestComputerName(hKeyHN, hKeyCN, L"123X.456", FALSE);

    DoTestComputerName(hKeyHN, hKeyCN, L"ThisIsLongLongComputerName", TRUE);

    for (i = 0; i < cchValidSymbols; ++i)
    {
        szName[0] = ValidSymbols[i];
        szName[1] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, TRUE);
    }

    for (i = 0; i < cchValidSymbols; ++i)
    {
        szName[0] = L'a';
        szName[1] = ValidSymbols[i];
        szName[2] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, TRUE);
    }

    for (i = 0; i < cchValidSymbols; ++i)
    {
        szName[0] = L'A';
        szName[1] = ValidSymbols[i];
        szName[2] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, TRUE);
    }

    for (i = 0; i < cchValidSymbols; ++i)
    {
        szName[0] = L'1';
        szName[1] = ValidSymbols[i];
        szName[2] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, TRUE);
    }

    for (i = 0; i < cchInvalidSymbols; ++i)
    {
        szName[0] = L'A';
        szName[1] = InvalidSymbols[i];
        szName[2] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, FALSE);
    }

    for (i = 0; i < cchInvalidSymbols; ++i)
    {
        szName[0] = L'1';
        szName[1] = InvalidSymbols[i];
        szName[2] = UNICODE_NULL;
        DoTestComputerName(hKeyHN, hKeyCN, szName, FALSE);
    }

    /* Close keys */
    Error = RegCloseKey(hKeyHN);
    ok_long(Error, ERROR_SUCCESS);
    Error = RegCloseKey(hKeyCN);
    ok_long(Error, ERROR_SUCCESS);
}
