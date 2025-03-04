/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for CertEnumSystemStoreLocation
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <apitest.h>
#include <windef.h>
#include <wincrypt.h>

#define ARG_CONTEXT ((PVOID)(ULONG_PTR)0x12345678)

typedef struct _CERT_SYSTEM_STORE_LOCATION
{
    DWORD dwFlags;
    PCWSTR pwszStoreLocation;
} CERT_SYSTEM_STORE_LOCATION, * PCERT_SYSTEM_STORE_LOCATION;

static const CERT_SYSTEM_STORE_LOCATION g_SystemStoreLocations[] = {
    { CERT_SYSTEM_STORE_CURRENT_USER, L"CurrentUser" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE, L"LocalMachine" },
    { CERT_SYSTEM_STORE_CURRENT_SERVICE, L"CurrentService" },
    { CERT_SYSTEM_STORE_SERVICES, L"Services" },
    { CERT_SYSTEM_STORE_USERS, L"Users" },
    { CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY, L"CurrentUserGroupPolicy" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY, L"LocalMachineGroupPolicy" },
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, L"LocalMachineEnterprise" },
};

static ULONG g_Index = 0;

static
BOOL
WINAPI
CertEnumSystemStoreLocationCallback(
    _In_ LPCWSTR pwszStoreLocation,
    _In_ DWORD dwFlags,
    _Reserved_ void* pvReserved,
    _Inout_opt_ void* pvArg)
{
    ok(pvReserved == NULL, "pvReserved is not NULL\n");
    ok(pvArg == ARG_CONTEXT, "pvArg incorrect\n");

    if (g_Index < ARRAYSIZE(g_SystemStoreLocations))
    {
        ok(dwFlags == g_SystemStoreLocations[g_Index].dwFlags, "#%lu dwFlags incorrect\n", g_Index);
        ok(wcscmp(pwszStoreLocation, g_SystemStoreLocations[g_Index].pwszStoreLocation) == 0,
           "#%lu pwszStoreLocation incorrect\n",
           g_Index);
    }

    g_Index++;
    return TRUE;
}

START_TEST(CertEnumSystemStoreLocation)
{
    BOOL bRet;

    /* dwFlags should be 0, otherwise fail with E_INVALIDARG */
    bRet = CertEnumSystemStoreLocation(1, ARG_CONTEXT, CertEnumSystemStoreLocationCallback);
    ok(bRet == FALSE && GetLastError() == E_INVALIDARG,
       "CertEnumSystemStoreLocation should failed with E_INVALIDARG when dwFlags is not 0\n");

    /* Start enumeration */
    bRet = CertEnumSystemStoreLocation(0, ARG_CONTEXT, CertEnumSystemStoreLocationCallback);
    ok(bRet != FALSE, "CertEnumSystemStoreLocation failed with 0x%08lX\n", GetLastError());
    ok(g_Index >= ARRAYSIZE(g_SystemStoreLocations), "Count of enumerated item incorrect\n");
}
