/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for <atlconv.h>
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif
#include <atlbase.h>
#include <atlconv.h>

START_TEST(atlconv)
{
    char dataA[MAX_PATH];
    wchar_t dataW[MAX_PATH];
    using namespace ATL;

    //
    // Initialize long data
    //
    FillMemory(dataA, sizeof(dataA), 'A');
    dataA[_countof(dataA) - 1] = 0;

    for (size_t i = 0; i < _countof(dataW); ++i)
        dataW[i] = L'A';
    dataW[_countof(dataW) - 1] = 0;

    //
    // The short strings
    //
    CA2A a2a("TEST123");
    ok_str((LPSTR)a2a, "TEST123");
    ok_str(a2a.m_psz, "TEST123");
    ok_ptr(a2a.m_psz, a2a.m_szBuffer);

    CW2A w2a(L"TEST123");
    ok_str((LPSTR)w2a, "TEST123");
    ok_str(w2a.m_psz, "TEST123");
    ok_ptr(w2a.m_psz, w2a.m_szBuffer);

    CA2W a2w("TEST123");
    ok_wstr((LPWSTR)a2w, L"TEST123");
    ok_wstr(a2w.m_psz, L"TEST123");
    ok_ptr(a2w.m_psz, a2w.m_szBuffer);

    CW2W w2w(L"TEST123");
    ok_wstr((LPWSTR)w2w, L"TEST123");
    ok_wstr(w2w.m_psz, L"TEST123");
    ok_ptr(w2w.m_psz, w2w.m_szBuffer);

    //
    // The long strings
    //
    CA2A a2a2(dataA);
    ok_str((LPSTR)a2a2, dataA);
    ok_str(a2a2.m_psz, dataA);
    ok_str(a2a2.m_szBuffer, "");
    ok(a2a2.m_psz != dataA, "a2a2.m_psz == dataA\n");
    ok(a2a2.m_psz != a2a2.m_szBuffer, "a2a2.m_psz == a2a2.m_szBuffer\n");

    CW2A w2a2(dataW);
    ok_str((LPSTR)w2a2, dataA);
    ok_str(w2a2.m_psz, dataA);
    ok_str(w2a2.m_szBuffer, "");
    ok(w2a2.m_psz != dataA, "w2a2.m_psz == dataA\n");
    ok(w2a2.m_psz != w2a2.m_szBuffer, "w2a2.m_psz == w2a2.m_szBuffer\n");

    CA2W a2w2(dataA);
    ok_wstr((LPWSTR)a2w2, dataW);
    ok_wstr(a2w2.m_psz, dataW);
    ok_wstr(a2w2.m_szBuffer, L"");
    ok(a2w2.m_psz != dataW, "a2w2.m_psz == dataW\n");
    ok(a2w2.m_psz != a2w2.m_szBuffer, "a2w2.m_psz == a2w2.m_szBuffer\n");

    CW2W w2w2(dataW);
    ok_wstr((LPWSTR)w2w2, dataW);
    ok_wstr(w2w2.m_psz, dataW);
    ok_wstr(w2w2.m_szBuffer, L"");
    ok(w2w2.m_psz != dataW, "w2w2.m_psz == dataW\n");
    ok(w2w2.m_psz != w2w2.m_szBuffer, "w2w2.m_psz == w2w2.m_szBuffer\n");

    //
    // The const strings
    //
    CA2CA a2ca(dataA);
    ok_str((LPCSTR)a2ca, dataA);
    ok_str(a2ca.m_psz, dataA);
    ok_ptr(a2ca.m_psz, dataA);

    CW2CW w2cw(dataW);
    ok_wstr((LPCWSTR)w2cw, dataW);
    ok_wstr(w2cw.m_psz, dataW);
    ok_ptr(w2cw.m_psz, dataW);
}
