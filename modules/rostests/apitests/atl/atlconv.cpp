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

    FillMemory(dataA, sizeof(dataA), 'A');
    dataA[_countof(dataA) - 1] = 0;

    for (size_t i = 0; i < _countof(dataW); ++i)
        dataW[i] = L'A';
    dataW[_countof(dataW) - 1] = 0;

    CA2A a2a("TEST123");
    ok_str((LPSTR)a2a, "TEST123");
    ok_str(a2a.m_psz, "TEST123");

    CW2A w2a(L"TEST123");
    ok_str((LPSTR)w2a, "TEST123");
    ok_str(w2a.m_psz, "TEST123");

    CA2W a2w("TEST123");
    ok_wstr((LPWSTR)a2w, L"TEST123");
    ok_wstr(a2w.m_psz, L"TEST123");

    CW2W w2w(L"TEST123");
    ok_wstr((LPWSTR)w2w, L"TEST123");
    ok_wstr(w2w.m_psz, L"TEST123");

    CA2A a2a2(dataA);
    ok_str((LPSTR)a2a2, dataA);
    ok_str(a2a2.m_psz, dataA);
    ok_str(a2a2.m_szBuffer, "");
    ok(a2a2.m_psz != a2a2.m_szBuffer, "a2a2.m_psz == a2a2.m_szBuffer\n");

    CW2A w2a2(dataW);
    ok_str((LPSTR)w2a2, dataA);
    ok_str(w2a2.m_psz, dataA);
    ok_str(w2a2.m_szBuffer, "");
    ok(w2a2.m_psz != w2a2.m_szBuffer, "w2a2.m_psz == w2a2.m_szBuffer\n");

    CA2W a2w2(dataA);
    ok_wstr((LPWSTR)a2w2, dataW);
    ok_wstr(a2w2.m_psz, dataW);
    ok_wstr(a2w2.m_szBuffer, L"");
    ok(a2w2.m_psz != a2w2.m_szBuffer, "a2w2.m_psz == a2w2.m_szBuffer\n");

    CW2W w2w2(dataW);
    ok_wstr((LPWSTR)w2w2, dataW);
    ok_wstr(w2w2.m_psz, dataW);
    ok_wstr(w2w2.m_szBuffer, L"");
    ok(w2w2.m_psz != w2w2.m_szBuffer, "w2w2.m_psz == w2w2.m_szBuffer\n");

    CA2CA a2ca("TEST123");
    ok_str((LPCSTR)a2ca, "TEST123");
    ok_str(a2ca.m_psz, "TEST123");

    CW2CW w2cw(L"TEST123");
    ok_wstr((LPCWSTR)w2cw, L"TEST123");
    ok_wstr(w2cw.m_psz, L"TEST123");
}
