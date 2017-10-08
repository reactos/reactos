/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for wcstombs
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <specstrings.h>

#define LStrROS L"ReactOS"
#define StrROS "ReactOS"

START_TEST(wcstombs)
{
    size_t len;
    char out[ARRAYSIZE(StrROS)];

    len = wcstombs(NULL, LStrROS, 0);
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = wcstombs(NULL, LStrROS, 0);
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = wcstombs(NULL, LStrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = wcstombs(NULL, LStrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = wcstombs(out, LStrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    ok_str(out, StrROS);
    memset(out, 0, sizeof(out));
    len = wcstombs(out, LStrROS, ARRAYSIZE(out));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    ok_str(out, StrROS);
}
