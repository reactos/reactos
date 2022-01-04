/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the RtlGetNtProductType API
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"
#include <winbase.h>
#include <winreg.h>

static
BOOLEAN
ReturnNtProduct(PDWORD ProductNtType)
{
    LONG Result;
    HKEY Key;
    WCHAR Data[20];
    DWORD Size = sizeof(Data);

    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                           0,
                           KEY_QUERY_VALUE,
                           &Key);
    if (Result != ERROR_SUCCESS)
    {
        *ProductNtType = 0;
        return FALSE;
    }

    Result = RegQueryValueExW(Key,
                              L"ProductType",
                              NULL,
                              NULL,
                              (PBYTE)&Data,
                              &Size);
    if (Result != ERROR_SUCCESS)
    {
        RegCloseKey(Key);
        *ProductNtType = 0;
        return FALSE;
    }

    if (wcscmp(Data, L"WinNT") == 0)
    {
        *ProductNtType = NtProductWinNt;
        RegCloseKey(Key);
        return TRUE;
    }

    if (wcscmp(Data, L"LanmanNT") == 0)
    {
        *ProductNtType = NtProductLanManNt;
        RegCloseKey(Key);
        return TRUE;
    }

    if (wcscmp(Data, L"ServerNT") == 0)
    {
        *ProductNtType = NtProductServer;
        RegCloseKey(Key);
        return TRUE;
    }

    *ProductNtType = 0;
    RegCloseKey(Key);
    return FALSE;
}

START_TEST(RtlGetNtProductType)
{
    BOOLEAN Ret;
    DWORD ProductNtType;
    NT_PRODUCT_TYPE ProductType = NtProductWinNt;

    /*
     * Wrap the call in SEH. This ensures the testcase won't crash but also
     * it proves to us that RtlGetNtProductType() throws an exception if a NULL
     * argument is being passed to caller as output.
     */
    StartSeh()
        RtlGetNtProductType(NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Query the product type normally from the Registry */
    Ret = ReturnNtProduct(&ProductNtType);
    if (!Ret)
    {
        ok(Ret, "Failed to query the product type value!\n");
    }

    /* Now, get the product type from the NTDLL system call */
    Ret = RtlGetNtProductType(&ProductType);
    ok(Ret == TRUE, "Expected a valid product type value (and TRUE as returned success code) but got %u as status.\n", Ret);
    ok(ProductNtType == ProductType, "Expected the product type value to be the same but got %lu (original value pointed by RtlGetNtProductType() is %d).\n", ProductNtType, ProductType);
}
