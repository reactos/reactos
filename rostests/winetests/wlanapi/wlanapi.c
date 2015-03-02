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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wlanapi.h"

#include "wine/test.h"

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
    ok(ret == ERROR_SUCCESS, "WlanOpenHandle failed, error %d\n", ret);
    WlanCloseHandle(hClientHandle, NULL);

    /* invalid pdwNegotiatedVersion */
    ret = WlanOpenHandle(1, NULL, NULL, &hClientHandle);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* invalid hClientHandle */
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* invalid pReserved */
    ret = WlanOpenHandle(1, (PVOID) 1, &dwNegotiatedVersion, &hClientHandle);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
}

static void WlanCloseHandle_test(void)
{
    DWORD ret;
    HANDLE hClientHandle = (HANDLE) 0xdeadbeef;

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
    DWORD dwNegotiatedVersion;
    HANDLE hClientHandle;
    WLAN_CONNECTION_PARAMETERS pConnectParams;
    const GUID InterfaceGuid = {0x439b20af, 0x8955, 0x405b, {0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43}};
    
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, &hClientHandle);
    if (ret != ERROR_SUCCESS)
    {
        skip("WlanOpenHandle failed. Skipping wlanapi_WlanConnect tests\n");
        return;
    }
    
    /* invalid pReserved */
    ret = WlanConnect(hClientHandle, &InterfaceGuid, &pConnectParams, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    /* invalid InterfaceGuid */
    ret = WlanConnect(hClientHandle, NULL, &pConnectParams, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    /* invalid hClientHandle */
    ret = WlanConnect(NULL, &InterfaceGuid, &pConnectParams, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid connection parameters */
    ret = WlanConnect(hClientHandle, &InterfaceGuid, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    WlanCloseHandle(hClientHandle, NULL);    
}

static void WlanDisconnect_test(void)
{
    DWORD ret;
    DWORD dwNegotiatedVersion;
    HANDLE hClientHandle;
    const GUID InterfaceGuid = {0x439b20af, 0x8955, 0x405b, {0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43}};
    
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, &hClientHandle);
    if (ret != ERROR_SUCCESS)
    {
        skip("WlanOpenHandle failed. Skipping wlanapi_WlanDisconnect tests\n");
        return;
    }
    
    /* invalid pReserved */
    ret = WlanDisconnect(hClientHandle, &InterfaceGuid, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    /* invalid InterfaceGuid */
    ret = WlanDisconnect(hClientHandle, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanDisconnect(NULL, &InterfaceGuid, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    WlanCloseHandle(hClientHandle, NULL);    
}

static void WlanScan_test(void)
{
    DWORD ret;
    DWORD dwNegotiatedVersion;
    HANDLE hClientHandle;
    DOT11_SSID Ssid;
    WLAN_RAW_DATA RawData;
    const GUID InterfaceGuid = {0x439b20af, 0x8955, 0x405b, {0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43}};
    
    ret = WlanOpenHandle(1, NULL, &dwNegotiatedVersion, &hClientHandle);
    if (ret != ERROR_SUCCESS)
    {
        skip("WlanOpenHandle failed. Skipping wlanapi_WlanDisconnect tests\n");
        return;
    }
    
    /* invalid pReserved */
    ret = WlanScan(hClientHandle, &InterfaceGuid, &Ssid, &RawData, (PVOID) 1);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    /* invalid InterfaceGuid */
    ret = WlanScan(hClientHandle, NULL, &Ssid, &RawData, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");

    /* invalid hClientHandle */
    ret = WlanScan(NULL, &InterfaceGuid, &Ssid, &RawData, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected failure\n");
    
    WlanCloseHandle(hClientHandle, NULL);    
}


START_TEST(wlanapi)
{
    WlanOpenHandle_test();
    WlanCloseHandle_test();
    WlanConnect_test();
    WlanDisconnect_test();
    WlanScan_test();
}
