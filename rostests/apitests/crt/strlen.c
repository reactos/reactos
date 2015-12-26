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

typedef size_t (*PFN_STRLEN)(const char *);

void
Test_strlen(PFN_STRLEN pstrlen)
{
    size_t len;

    /* basic parameter tests */
    StartSeh()
        len = pstrlen(NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    (void)len;

    ok_int((int)pstrlen("test"), 4);
}

START_TEST(strlen)
{
    Test_strlen(strlen);
#ifdef __GNUC__
    Test_strlen(GCC_builtin_strlen);
#endif // __GNUC__
}
