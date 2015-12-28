/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for strlen
 * PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>

#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ntstatus.h>
typedef _Return_type_success_(return >= 0) long NTSTATUS, *PNTSTATUS;

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wnonnull"

size_t
GCC_builtin_strlen(const char *str)
{
    return __builtin_strlen(str);
}
#endif

#define EFLAGS_DF 0x400L

typedef size_t (*PFN_STRLEN)(const char *);

void
Test_strlen(PFN_STRLEN pstrlen)
{
    size_t len;
#if defined(_M_IX86) || defined(_M_AMD64)
    volatile uintptr_t eflags;
    char *teststr = "a\0bcdefghijk";
#endif

    /* basic parameter tests */
    StartSeh()
        len = pstrlen(NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    (void)len;

    ok_int((int)pstrlen("test"), 4);

#if defined(_M_IX86) || defined(_M_AMD64)
    eflags = __readeflags();
    __writeeflags(eflags | EFLAGS_DF);
    len = pstrlen(teststr + 4);
    eflags = __readeflags();
    ok((eflags & EFLAGS_DF) != 0, "Direction flag in ELFAGS was changed.");

    /* Only test this for the exported versions, intrinsics might do it
       differently. It's up to us to not do fishy stuff! Also crtdll does
       not do it like this. */
#ifndef TEST_CRTDLL
    if (pstrlen == strlen)
    {
        ok(len == 8, "Should not have gone backwards (got len %i)", (int)len);
    }
#endif // TEST_CRTDLL
#endif
}

START_TEST(strlen)
{
    Test_strlen(strlen);
#ifdef __GNUC__
    Test_strlen(GCC_builtin_strlen);
#endif // __GNUC__
}
