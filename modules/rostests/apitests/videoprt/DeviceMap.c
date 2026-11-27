/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for HKLM\HARDWARE\DEVICEMAP\VIDEO entries created by videoprt
 * COPYRIGHT:   Copyright 2026 Noel <beebix@schmusemail.de>
 */

#include "precomp.h"

START_TEST(DeviceMap)
{
    static const WCHAR MachinePrefix[] = L"\\Registry\\Machine\\";
    static const WCHAR ControlVideoPrefix[] = L"System\\CurrentControlSet\\Control\\Video\\{";
    HKEY hVideoMap = NULL, hTargetKey = NULL;
    WCHAR RegPath[512];
    PCWSTR SubKey;
    DWORD dwType = 0, cbData;
    size_t ExpectedLen, StrLen;
    BOOL HasPrefix;
    LONG rc;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", 0, KEY_READ, &hVideoMap);
    ok(rc == ERROR_SUCCESS, "RegOpenKeyExW(HARDWARE\\DEVICEMAP\\VIDEO) failed: %ld\n", rc);
    if (rc != ERROR_SUCCESS)
    {
        skip("Cannot query DEVICEMAP\\VIDEO\n");
        return;
    }

    RtlZeroMemory(RegPath, sizeof(RegPath));
    cbData = sizeof(RegPath) - sizeof(WCHAR);
    rc = RegQueryValueExW(hVideoMap, L"\\Device\\Video0", NULL, &dwType, (LPBYTE)RegPath, &cbData);
    RegCloseKey(hVideoMap);
    ok(rc == ERROR_SUCCESS && dwType == REG_SZ,
       "Query \\Device\\Video0 failed: rc=%ld type=%lu\n", rc, dwType);
    if (rc != ERROR_SUCCESS)
    {
        skip("No \\Device\\Video0 value\n");
        return;
    }

    /* Ensure there is no embedded NUL before the end of the REG_SZ data */
    ExpectedLen = cbData / sizeof(WCHAR);
    if (ExpectedLen > 0 && RegPath[ExpectedLen - 1] == UNICODE_NULL)
        ExpectedLen--;
    StrLen = wcslen(RegPath);
    ok(StrLen > 0 && StrLen == ExpectedLen,
       "\\Device\\Video0 has embedded NUL (wcslen=%Iu, expected=%Iu, path='%S')\n",
       StrLen, ExpectedLen, RegPath);

    HasPrefix = (_wcsnicmp(RegPath, MachinePrefix, _countof(MachinePrefix) - 1) == 0);
    ok(HasPrefix, "\\Device\\Video0 missing \\Registry\\Machine\\ prefix: '%S'\n", RegPath);
    if (!HasPrefix)
    {
        skip("Cannot open target key\n");
        return;
    }

    SubKey = RegPath + _countof(MachinePrefix) - 1;
    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, &hTargetKey);
    ok(rc == ERROR_SUCCESS, "Target key 'HKLM\\%S' could not be opened: %ld\n", SubKey, rc);
    if (rc != ERROR_SUCCESS)
    {
        skip("Cannot query target key\n");
        return;
    }

    if (_wcsnicmp(SubKey, ControlVideoPrefix, _countof(ControlVideoPrefix) - 1) == 0)
    {
        DWORD DrvLen = 0;
        rc = RegQueryValueExW(hTargetKey, L"InstalledDisplayDrivers", NULL, NULL, NULL, &DrvLen);
        ok(rc == ERROR_SUCCESS && DrvLen > 0,
           "Target key 'HKLM\\%S' missing InstalledDisplayDrivers (rc=%ld)\n", SubKey, rc);
    }
    else
    {
        skip("Target key 'HKLM\\%S' does not use Control\\Video\\{GUID} layout\n", SubKey);
    }

    RegCloseKey(hTargetKey);
}
