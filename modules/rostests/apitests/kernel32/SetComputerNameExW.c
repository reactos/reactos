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

START_TEST(SetComputerNameExW)
{
    static const WCHAR szNewName[] = L"SRVROSTEST";
    LONG Error;
    BOOL ret;
    HKEY hKeyHN, hKeyCN;
    DWORD cbData;
    WCHAR szNVHostNameOld[MAX_PATH], szNVHostNameNew[MAX_PATH];
    WCHAR szHostNameOld[MAX_PATH], szHostNameNew[MAX_PATH];
    WCHAR szComputerNameOld[MAX_PATH], szComputerNameNew[MAX_PATH];

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
    ret = SetComputerNameExW(ComputerNamePhysicalDnsHostname, szNewName);
    ok_int(ret, TRUE);

    /* Get New NV Hostname */
    szNVHostNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szNVHostNameNew);
    Error = RegQueryValueExW(hKeyHN, L"NV Hostname", NULL, NULL, (LPBYTE)szNVHostNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szNVHostNameNew[0], "szNVHostNameNew is empty\n");
    ok(lstrcmpW(szNVHostNameNew, szNewName) == 0,
       "szNVHostNameNew '%S' should be szNewName '%S'\n", szNVHostNameNew, szNewName);

    /* Get New Hostname */
    szHostNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szHostNameNew);
    Error = RegQueryValueExW(hKeyHN, L"Hostname", NULL, NULL, (LPBYTE)szHostNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szHostNameNew[0], "szHostNameNew is empty\n");
    ok(lstrcmpW(szHostNameNew, szHostNameOld) == 0,
       "szHostNameNew '%S' should be szHostNameOld '%S'\n", szHostNameNew, szHostNameOld);

    /* Get New Computer Name */
    szComputerNameNew[0] = UNICODE_NULL;
    cbData = sizeof(szComputerNameNew);
    Error = RegQueryValueExW(hKeyCN, L"ComputerName", NULL, NULL, (LPBYTE)szComputerNameNew, &cbData);
    ok_long(Error, ERROR_SUCCESS);
    ok(szComputerNameNew[0], "szComputerNameNew is empty\n");
    ok(lstrcmpW(szComputerNameNew, szNewName) == 0,
       "szComputerNameNew '%S' should be szNewName '%S'\n", szComputerNameNew, szNewName);

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

    /* Close keys */
    Error = RegCloseKey(hKeyHN);
    ok_long(Error, ERROR_SUCCESS);
    Error = RegCloseKey(hKeyCN);
    ok_long(Error, ERROR_SUCCESS);
}
