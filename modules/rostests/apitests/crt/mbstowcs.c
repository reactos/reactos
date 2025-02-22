/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for mbstowcs
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <specstrings.h>

#define StrROS "ReactOS"
#define LStrROS L"ReactOS"

START_TEST(mbstowcs)
{
    size_t len;
    wchar_t out[ARRAYSIZE(LStrROS)];

    len = mbstowcs(NULL, StrROS, 0);
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = mbstowcs(NULL, StrROS, 0);
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = mbstowcs(NULL, StrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = mbstowcs(NULL, StrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = mbstowcs(out, StrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    ok_wstr(out, LStrROS);
    memset(out, 0, sizeof(out));
    len = mbstowcs(out, StrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    ok_wstr(out, LStrROS);
}
