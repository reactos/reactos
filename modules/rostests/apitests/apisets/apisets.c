/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Validate the apiset lookup in ApiSetResolveToHost
 * COPYRIGHT:   Copyright 2024 Mark Jansen <mark.jansen@reactos.org>
 */

#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>
#include <apitest.h>
#include "apisetsp.h"

static void
resolve_single(PCUNICODE_STRING Apiset)
{
    UNICODE_STRING Tmp = {0};
    NTSTATUS Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, Apiset, &Tmp);
    ok_ntstatus(Status, STATUS_SUCCESS);

    BOOLEAN Resolved = FALSE;
    UNICODE_STRING Result = {0};

    Status = ApiSetResolveToHost(~0u, &Tmp, &Resolved, &Result);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_bool(Resolved, TRUE);
    ok(Result.Buffer != NULL, "Got NULL\n");
}

static void
resolve_fail_single(PCUNICODE_STRING Apiset)
{
    UNICODE_STRING Tmp = {0};
    NTSTATUS Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, Apiset, &Tmp);
    ok_ntstatus(Status, STATUS_SUCCESS);

    BOOLEAN Resolved = FALSE;
    UNICODE_STRING Result = {0, 0, (PWSTR)0xbadbeef};

    Status = ApiSetResolveToHost(~0u, &Tmp, &Resolved, &Result);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_bool(Resolved, FALSE);
    ok_ptr(Result.Buffer, (PWSTR)0xbadbeef);
}

static void
test_single(PCUNICODE_STRING Apiset)
{
    winetest_push_context("%S", Apiset->Buffer);
    resolve_single(Apiset);
    winetest_pop_context();
}

static void
fail_single(PCUNICODE_STRING Apiset)
{
    winetest_push_context("%S", Apiset->Buffer);
    resolve_fail_single(Apiset);
    winetest_pop_context();
}


START_TEST(apisets)
{
    // Ensure we can find some manually selected ones (both with and without extension):
    UNICODE_STRING Console1 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Console-L1-1-0.dll");
    UNICODE_STRING Console2 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Console-L1-1-0");
    UNICODE_STRING Handle1 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Handle-L1-1-0.dll");
    UNICODE_STRING Handle2 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Handle-L1-1-0");
    UNICODE_STRING Heap1 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Heap-L1-1-0.dll");
    UNICODE_STRING Heap2 = RTL_CONSTANT_STRING(L"api-MS-Win-Core-Heap-L1-1-0");

    test_single(&Console1);
    test_single(&Console2);
    test_single(&Handle1);
    test_single(&Handle2);
    test_single(&Heap1);
    test_single(&Heap2);

    // Walk the entire table, to ensure we can find each entry
    for (DWORD n = 0; n < g_ApisetsCount; ++n)
    {
        PCUNICODE_STRING Apiset = &g_Apisets[n].Name;
        winetest_push_context("%ld: %S", n, Apiset->Buffer);
        resolve_single(Apiset);
        winetest_pop_context();
    }

    UNICODE_STRING Fail1 = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING Fail2 = RTL_CONSTANT_STRING(L"api-");
    UNICODE_STRING Fail3 = RTL_CONSTANT_STRING(L"ext");

    fail_single(&Fail1);
    fail_single(&Fail2);
    fail_single(&Fail3);
}
