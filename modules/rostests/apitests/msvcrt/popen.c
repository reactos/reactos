/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         Test for CRT process handling.
 * PROGRAMMER:      Andreas Maier <andy1.m@gmx.de>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>

static void Test_popen()
{
    FILE * f;
    int r;
    char str[20];

    /* NOTE: We suppose that the NT test installation has an accessible cmd.exe */
    f = _popen("cmd.exe /C \"echo Hallo\"", "r");
    ok(f != NULL, "_popen returns NULL!\n");

    ZeroMemory(str, sizeof(str));
    fgets(str, sizeof(str) - 1, f);
    ok(lstrcmp(str, "Hallo\n") == 0, "fgets: expected \"Hallo\", got %s.\n", str);

    r = _pclose(f);
    ok(r == 0, "_pclose: expected 0, got %i.\n", r);
    r = *_errno();
    ok(r == 0, "_errno: expected 0, got %i,\n", r);
}

START_TEST(popen)
{
    Test_popen();
}

/* EOF */
