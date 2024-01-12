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

/*
 * cmp functions can either return 1/-1 or the actual difference between the
 * first two differing characters.
 * On Win2003, both crtdll and msvcrt always return 1/-1.
 * On Win10, msvcrt returns the diff, crtdll returns 1/-1.
 */
#ifdef TEST_CRTDLL
#define RETURN_DIFF 0
#else
#define RETURN_DIFF (GetVersion() >= 0x0600)
#endif

#define DIFF_RETURN(sign, absolute) (sign (RETURN_DIFF ? absolute : 1))

START_TEST(_mbsncmp)
{
    int ret;

    /* Zero length always returns true */
    ret = _mbsncmp(NULL, NULL, 0);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp("a", "c", 0);
    ok(ret == 0, "ret = %d\n", ret);

    /* No null checks - length 1 crashes */
    StartSeh()
        (void)_mbsncmp("a", NULL, 1);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        (void)_mbsncmp(NULL, "c", 1);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Strings longer than or equal to length */
    ret = _mbsncmp("a", "c", 1);
    ok(ret == DIFF_RETURN(-, 2), "ret = %d\n", ret);

    ret = _mbsncmp("a", "a", 1);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp("ab", "aB", 1);
    ok(ret == 0, "ret = %d\n", ret);

    ret = _mbsncmp("aa", "ac", 2);
    ok(ret == DIFF_RETURN(-, 2), "ret = %d\n", ret);

    /* Length longer than one of the strings */
    ret = _mbsncmp("a", "ac", 2);
    ok(ret == DIFF_RETURN(-, 'c'), "ret = %d\n", ret);

    ret = _mbsncmp("aa", "a", 2);
    ok(ret == DIFF_RETURN(+, 'a'), "ret = %d\n", ret);

    ret = _mbsncmp("ab", "ab", 100);
    ok(ret == 0, "ret = %d\n", ret);
}
