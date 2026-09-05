/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for SetSearchPathMode
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <apitest.h>
#include <winbase.h>

START_TEST(SetSearchPathMode)
{
    BOOL Ret;

    /* Testing invalid flags */

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(0x0);
    ok(!Ret, "SetSearchPathMode unexpectedly succeeded with invalid parameter\n");
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(0xdeadbeef);
    ok(!Ret, "SetSearchPathMode unexpectedly succeeded with invalid parameter\n");
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                            BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE);
    ok(!Ret, "SetSearchPathMode unexpectedly succeeded with invalid parameter\n");
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_PERMANENT);
    ok(!Ret, "SetSearchPathMode unexpectedly succeeded with invalid parameter\n");
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE |
                            BASE_SEARCH_PATH_PERMANENT);
    ok(!Ret, "SetSearchPathMode unexpectedly succeeded with invalid parameter\n");
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Enabling and disabling safe search mode */

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE);
    ok(Ret, "ENABLE_SAFE_SEARCHMODE failed\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE);
    ok(Ret, "DISABLE_SAFE_SEARCHMODE failed\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE);
    ok(Ret, "ENABLE_SAFE_SEARCHMODE failed (second call)\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);
    ok(Ret, "ENABLE_SAFE_SEARCHMODE failed (second call)\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Testing permanent mode */

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                            BASE_SEARCH_PATH_PERMANENT);
    ok(Ret, "Permanent mode failed\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE);
    ok(!Ret, "ENABLE unexpectedly succeeded after permanent mode\n");
    ok_eq_ulong(GetLastError(), ERROR_ACCESS_DENIED);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE);
    ok(!Ret, "DISABLE unexpectedly succeeded after permanent mode\n");
    ok_eq_ulong(GetLastError(), ERROR_ACCESS_DENIED);

    SetLastError(0xdeadbeef);
    Ret = SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                            BASE_SEARCH_PATH_PERMANENT);
    ok(Ret, "Calling permanent mode again should succeed\n");
    ok_eq_ulong(GetLastError(), 0xdeadbeef);
}
