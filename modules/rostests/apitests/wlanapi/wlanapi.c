/*
 * Wlanapi - tests
 *
 * Copyright 2009 Christoph von Wittich
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <apitest.h>

#include <wlanapi.h>

static const GUID InterfaceGuid = {0x439b20af, 0x8955, 0x405b, {0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43}};

static void WlanOpenHandle_test(void)
{
    DWORD ret;
    DWORD dwNegotiatedVersion;
    HANDLE hClientHandle;

    /* correct call to determine if WlanSvc is running */
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, &hClientHandle);
    if (ret == ERROR_SERVICE_EXISTS)
    {
        skip("Skipping wlanapi tests, WlanSvc is not running\n");
        return;
    }
    ok(ret == ERROR_SUCCESS, "WlanOpenHandle failed, error %ld\n", ret);
    WlanCloseHandle(hClientHandle, NULL);

    /* invalid pdwNegotiatedVersion */
    ret = WlanOpenHandle(1, NULL, NULL, &hClientHandle);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* invalid hClientHandle */
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* invalid pReserved */
    ret = WlanOpenHandle(1, (PVOID) 1, &dwNegotiatedVersion, &hClientHandle);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

static void WlanCloseHandle_test(void)
{
    DWORD ret;
    HANDLE hClientHandle = (HANDLE)(ULONG_PTR)0xdeadbeefdeadbeef;

    /* invalid pReserved */
    ret = WlanCloseHandle(hClientHandle, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanCloseHandle(NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanCloseHandle(hClientHandle, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "expected failure\n");
}

static void WlanConnect_test(void)
{
    DWORD ret;
    WLAN_CONNECTION_PARAMETERS pConnectParams;

    /* invalid pReserved */
    ret = WlanConnect((HANDLE) -1, &InterfaceGuid, &pConnectParams, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanConnect((HANDLE) -1, NULL, &pConnectParams, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanConnect(NULL, &InterfaceGuid, &pConnectParams, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid connection parameters */
    ret = WlanConnect((HANDLE) -1, &InterfaceGuid, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanDisconnect_test(void)
{
    DWORD ret;

    /* invalid pReserved */
    ret = WlanDisconnect((HANDLE) -1, &InterfaceGuid, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanDisconnect((HANDLE) -1, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanDisconnect(NULL, &InterfaceGuid, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanScan_test(void)
{
    DWORD ret;
    DOT11_SSID Ssid;
    WLAN_RAW_DATA RawData;

    /* invalid pReserved */
    ret = WlanScan((HANDLE) -1, &InterfaceGuid, &Ssid, &RawData, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanScan((HANDLE) -1, NULL, &Ssid, &RawData, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanScan(NULL, &InterfaceGuid, &Ssid, &RawData, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanRenameProfile_test(void)
{
    DWORD ret;

    /* invalid pReserved */
    ret = WlanRenameProfile((HANDLE) -1, &InterfaceGuid, L"test", L"test1", (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanRenameProfile((HANDLE) -1, NULL, L"test", L"test1", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid strOldProfileName */
    ret = WlanRenameProfile((HANDLE) -1, &InterfaceGuid, NULL, L"test1", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid strNewProfileName */
    ret = WlanRenameProfile((HANDLE) -1, &InterfaceGuid, L"test", NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanDeleteProfile_test(void)
{
    DWORD ret;

    /* invalid pReserved */
    ret = WlanDeleteProfile((HANDLE) -1, &InterfaceGuid, L"test", (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanDeleteProfile((HANDLE) -1, NULL, L"test", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid strProfileName */
    ret = WlanDeleteProfile((HANDLE) -1, &InterfaceGuid, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanGetProfile_test(void)
{
    DWORD ret;
    WCHAR *strProfileXml;

    /* invalid pReserved */
    ret = WlanGetProfile((HANDLE) -1, &InterfaceGuid, L"", (PVOID) 1, &strProfileXml, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanGetProfile((HANDLE) -1, NULL, L"test", NULL, &strProfileXml, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid pstrProfileXml */
    ret = WlanGetProfile((HANDLE) -1, &InterfaceGuid, L"test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanEnumInterfaces_test(void)
{
    DWORD ret;
    PWLAN_INTERFACE_INFO_LIST pInterfaceList;

    /* invalid pReserved */
    ret = WlanEnumInterfaces((HANDLE) -1, (PVOID) 1, &pInterfaceList);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid pInterfaceList */
    ret = WlanEnumInterfaces((HANDLE) -1, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}

static void WlanGetInterfaceCapability_test(void)
{
    DWORD ret;
    PWLAN_INTERFACE_CAPABILITY pInterfaceCapability;

    /* invalid pReserved */
    ret = WlanGetInterfaceCapability((HANDLE) -1, &InterfaceGuid, (PVOID) 1, &pInterfaceCapability);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid InterfaceGuid */
    ret = WlanGetInterfaceCapability((HANDLE) -1, NULL, NULL, &pInterfaceCapability);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid pInterfaceCapability */
    ret = WlanGetInterfaceCapability((HANDLE) -1, &InterfaceGuid, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
}


START_TEST(wlanapi)
{
    WlanOpenHandle_test();
    WlanCloseHandle_test();
    WlanConnect_test();
    WlanDisconnect_test();
    WlanScan_test();
    WlanRenameProfile_test();
    WlanDeleteProfile_test();
    WlanGetProfile_test();
    WlanEnumInterfaces_test();
    WlanGetInterfaceCapability_test();
}
