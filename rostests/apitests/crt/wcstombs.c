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

#define StrROS L"ReactOS"


START_TEST(wcstombs)
{
    size_t len;

    len = wcstombs(NULL, StrROS, sizeof(StrROS) / sizeof(StrROS[0]));
    ok(len == 7, "Got len = %u, excepting 7\n", len);
    len = wcstombs(NULL, StrROS, sizeof(StrROS) / sizeof(StrROS[0]) - 1);
    ok(len == 7, "Got len = %u, excepting 7\n", len);
}
