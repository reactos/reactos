/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _mbsncmp
 * COPYRIGHT:       Copyright 2024 Thomas Faber (thomas.faber@reactos.org)
 */

#include <apitest.h>
#include <mbstring.h>
#define WIN32_NO_STATUS
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>

ULONG g_WinVersion;

/*
 * cmp functions can either return 1/-1 or the actual difference between the
 * first two differing characters.
 * On Win2003, both crtdll and msvcrt always return 1/-1.
 * On Win10, msvcrt returns the diff, crtdll returns 1/-1.
 */
#ifdef TEST_CRTDLL
#define RETURN_DIFF 0
#else
#define RETURN_DIFF (g_WinVersion >= _WIN32_WINNT_VISTA)
#endif

#define DIFF_RETURN(sign, absolute) (sign (RETURN_DIFF ? absolute : 1))

START_TEST(_mbsncmp)
{
    int ret;

    ULONG Version = GetVersion();
    g_WinVersion = (Version & 0xFF) << 8 | (Version & 0xFF00) >> 8;

    /* Zero length always returns true */
    ret = _mbsncmp(NULL, NULL, 0);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"a", (const unsigned char *)"c", 0);
    ok(ret == 0, "ret = %d\n", ret);

    /* No null checks - length 1 crashes */
    StartSeh()
        (void)_mbsncmp((const unsigned char *)"a", NULL, 1);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        (void)_mbsncmp(NULL, (const unsigned char *)"c", 1);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Strings longer than or equal to length */
    ret = _mbsncmp((const unsigned char *)"a", (const unsigned char *)"c", 1);
    ok(ret == DIFF_RETURN(-, 2), "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"a", (const unsigned char *)"a", 1);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"ab", (const unsigned char *)"aB", 1);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"aa", (const unsigned char *)"ac", 2);
    ok(ret == DIFF_RETURN(-, 2), "ret = %d\n", ret);

    /* Length longer than one of the strings */
    ret = _mbsncmp((const unsigned char *)"a", (const unsigned char *)"ac", 2);
    ok(ret == DIFF_RETURN(-, 'c'), "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"aa", (const unsigned char *)"a", 2);
    ok(ret == DIFF_RETURN(+, 'a'), "ret = %d\n", ret);

    ret = _mbsncmp((const unsigned char *)"ab", (const unsigned char *)"ab", 100);
    ok(ret == 0, "ret = %d\n", ret);
}
