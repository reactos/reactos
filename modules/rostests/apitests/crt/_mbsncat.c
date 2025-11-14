/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for _mbsncat
 * COPYRIGHT:   Copyright 2025 Doug Lyons <douglyons@douglyons.com>
 *              Copyright 2025 Stanislav Motylkov <x86corez@gmail.com>
 */

#include <apitest.h>
#include <mbstring.h>
#define WIN32_NO_STATUS
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>

typedef unsigned char *(__cdecl *PFN__mbsncat)(unsigned char*, const unsigned char*, size_t);

#ifndef TEST_STATIC_CRT
static PFN__mbsncat Init(_In_ LPSTR fname)
{
    static PFN__mbsncat p__mbsncat;
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);

    p__mbsncat = (PFN__mbsncat)GetProcAddress(hdll, fname);
    ok(p__mbsncat != NULL, "Failed to load %s from %s\n", fname, TEST_DLL_NAME);
    return p__mbsncat;
}
#endif

static USHORT GetWinVersion(VOID)
{
    return ((GetVersion() & 0xFF) << 8) |
           ((GetVersion() >> 8) & 0xFF);
}

VOID mbsncat_PerformTests(_In_ LPSTR fname, _In_opt_ PFN__mbsncat func)
{
    unsigned char dest[16];
    const unsigned char first[] = "dinosaur";
    const unsigned char second[] = "duck";
    unsigned char *s;
    BOOL MsVcrt = FALSE, CrtDll = FALSE;

#ifdef TEST_MSVCRT
    MsVcrt = TRUE;
#endif
#ifdef TEST_CRTDLL
    CrtDll = TRUE;
#endif

#ifndef TEST_STATIC_CRT
    if (!func)
        func = Init(fname);
#endif
    if (!func)
    {
        skip("Skipping tests, because %s is not available\n", fname);
        return;
    }

    /* Test invalid arguments */
    StartSeh()
        s = func(NULL, NULL, 0);
    EndSeh(STATUS_SUCCESS);
    ok(s == NULL, "Expected %s to return NULL, got %p\n", fname, s);

    StartSeh()
        s = func(NULL, NULL, 10);
    EndSeh((CrtDll || (MsVcrt && GetWinVersion() <= 0x502)) ? STATUS_ACCESS_VIOLATION : STATUS_SUCCESS);
    ok(s == NULL, "Expected %s to return NULL, got %p\n", fname, s);

    memset(dest, 'X', sizeof(dest));
    StartSeh()
        s = func(dest, NULL, 0);
    EndSeh(STATUS_SUCCESS);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(dest[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(dest, 'X', sizeof(dest));
    StartSeh()
        s = func(dest, second, 0);
    EndSeh(STATUS_SUCCESS);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(dest[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(dest, 'X', sizeof(dest));
    s = NULL;
    StartSeh()
        s = func(dest, NULL, 10);
    EndSeh((CrtDll || (MsVcrt && GetWinVersion() <= 0x502)) ? STATUS_ACCESS_VIOLATION : STATUS_SUCCESS);
    ok(s == NULL, "Expected %s to return NULL, got %p\n", fname, s);
    ok(dest[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(dest, 'X', sizeof(dest));
    dest[0] = '\0';
    StartSeh()
        s = func(dest, second, sizeof(second));
    EndSeh(STATUS_SUCCESS);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, second, sizeof(second)),
       "Expected the output buffer string to be \"duck\", got '%s'\n", dest);

    /* Test source truncation behavior */
    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, 0);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, first, sizeof(first)),
       "Expected the output buffer string to be \"dinosaur\", got '%s'\n", dest);

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, sizeof(second));
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\", got '%s'\n", dest);

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, sizeof(second) + 1);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\", got '%s'\n", dest);

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, sizeof(second) - 1);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\", got '%s'\n", dest);

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, sizeof(second) - 2);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, "dinosaurduc", sizeof("dinosaurduc")),
       "Expected the output buffer string to be \"dinosaurduc\", got '%s'\n", dest);

    /* Test typical scenario */
    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    s = func(dest, second, sizeof(second) - 1);
    ok(s == dest, "Expected %s to return dest pointer, got %p\n", fname, s);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\", got '%s'\n", dest);

    /* TODO: Add some distinguishing tests (_mbsncat vs. _mbsnbcat) for concatenating chars vs. bytes,
     * should probably be done with actual multibyte-character strings. */
}

START_TEST(_mbsncat)
{
#ifndef TEST_STATIC_CRT
    #define _mbsncat NULL
#endif
    mbsncat_PerformTests("_mbsncat", _mbsncat);
}
