/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _mbsstr
 * COPYRIGHT:       Copyright 2024 Thomas Faber (thomas.faber@reactos.org)
 */

#include <apitest.h>
#include <mbstring.h>
#define WIN32_NO_STATUS
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>

START_TEST(_mbsstr)
{
    unsigned char *haystack;
    unsigned char *ret;

    /* NULL pointers are not handled */
    StartSeh()
        (void)_mbsstr(NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        haystack = (unsigned char *)"hello";
        (void)_mbsstr(haystack, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        haystack = (unsigned char *)"";
        (void)_mbsstr(haystack, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Empty needle returns haystack, empty haystack returns NULL... */
    haystack = (unsigned char *)"hello";
    ret = _mbsstr(haystack, (const unsigned char *)"");
    ok(ret == haystack, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"";
    ret = _mbsstr(haystack, (const unsigned char *)"a");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);

    /* ... but if both are empty, behavior differs */
    haystack = (unsigned char *)"";
    ret = _mbsstr(haystack, (const unsigned char *)"");
#ifdef TEST_CRTDLL
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);
#else
    ok(ret == haystack, "ret = %p, haystack = %p\n", ret, haystack);
#endif

    /* Simple "found" cases */
    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"abc");
    ok(ret == haystack, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"g");
    ok(ret == haystack + 6, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"abcdefg");
    ok(ret == haystack, "ret = %p, haystack = %p\n", ret, haystack);

    /* Simple "not found" cases */
    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"h");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"gh");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"abcD");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);

    /* Needle longer than haystack */
    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"abcdefgh");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);

    haystack = (unsigned char *)"abcdefg";
    ret = _mbsstr(haystack, (const unsigned char *)"xxxxxxxx");
    ok(ret == NULL, "ret = %p, haystack = %p\n", ret, haystack);
}

