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
    using namespace ATL;

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

    CA2CA a2ca("TEST123");
    ok_str((LPCSTR)a2ca, "TEST123");
    ok_str(a2ca.m_psz, "TEST123");

    CW2CW w2cw(L"TEST123");
    ok_wstr((LPCWSTR)w2cw, L"TEST123");
    ok_wstr(w2cw.m_psz, L"TEST123");
}
