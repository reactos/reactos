/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for statically linked __rt_sdiv/udiv/sdiv64/udiv64 on ARM
 * COPYRIGHT:       Copyright 2021 Stanislav Motylkov <x86corez@gmail.com>
 */

#include <apitest.h>

typedef struct _SDIV_TEST_DATA
{
    LONG dividend;
    LONG divisor;
    LONG expected_div;
    LONG expected_mod;
} SDIV_TEST_DATA;

typedef struct _UDIV_TEST_DATA
{
    ULONG dividend;
    ULONG divisor;
    ULONG expected_div;
    ULONG expected_mod;
} UDIV_TEST_DATA;

typedef struct _SDIV64_TEST_DATA
{
    LONGLONG dividend;
    LONGLONG divisor;
    LONGLONG expected_div;
    LONGLONG expected_mod;
} SDIV64_TEST_DATA;

typedef struct _UDIV64_TEST_DATA
{
    ULONGLONG dividend;
    ULONGLONG divisor;
    ULONGLONG expected_div;
    ULONGLONG expected_mod;
} UDIV64_TEST_DATA;

START_TEST(__rt_div)
{
    SDIV_TEST_DATA sdiv[] =
    {
        /* Dividend is larger than divisor */
        { 3425, 400, 8, 225 },
        { -3425, 400, -8, -225 },
        { 3425, -400, -8, 225 },
        { -3425, -400, 8, -225 },
        /* Divisor is larger than dividend */
        { 12, 42, 0, 12 },
        { -12, 42, 0, -12 },
        { 12, -42, 0, 12 },
        { -12, -42, 0, -12 },
        /* Division without remainder */
        { 16777216, 65536, 256, 0 },
        { -16777216, 65536, -256, 0 },
        { 16777216, -65536, -256, 0 },
        { -16777216, -65536, 256, 0 },
    };
    UDIV_TEST_DATA udiv[] =
    {
        /* Dividend is larger than divisor */
        { 3425, 400, 8, 225 },
        /* Divisor is larger than dividend */
        { 12, 42, 0, 12 },
        /* Division without remainder */
        { 16777216, 65536, 256, 0 },
    };
    SDIV64_TEST_DATA sdiv64[] =
    {
        /* Dividend is larger than divisor */
        { 34918215, 7, 4988316, 3 },
        { -34918215, 7, -4988316, -3 },
        { 34918215, -7, -4988316, 3 },
        { -34918215, -7, 4988316, -3 },
        /* Divisor is larger than dividend */
        { 12, 42, 0, 12 },
        { -12, 42, 0, -12 },
        { 12, -42, 0, 12 },
        { -12, -42, 0, -12 },
        /* Division without remainder */
        { 16777216, 65536, 256, 0 },
        { -16777216, 65536, -256, 0 },
        { 16777216, -65536, -256, 0 },
        { -16777216, -65536, 256, 0 },

        /* Big 64-bit numbers */

        /* Dividend is larger than divisor */
        { 0x2AFFFFFFFLL * 100, 400, 2885681151LL, 300 },
        { -0x2AFFFFFFFLL * 100, 400, -2885681151LL, -300 },
        { 0x2AFFFFFFFLL * 100, -400, -2885681151LL, 300 },
        { -0x2AFFFFFFFLL * 100, -400, 2885681151LL, -300 },
        /* Divisor is larger than dividend */
        { 0x2AFFFFFFFLL * 50, 0x2AFFFFFFFLL * 100, 0, 0x2AFFFFFFFLL * 50 },
        { -0x2AFFFFFFFLL * 50, 0x2AFFFFFFFLL * 100, 0, -0x2AFFFFFFFLL * 50 },
        { 0x2AFFFFFFFLL * 50, -0x2AFFFFFFFLL * 100, 0, 0x2AFFFFFFFLL * 50 },
        { -0x2AFFFFFFFLL * 50, -0x2AFFFFFFFLL * 100, 0, -0x2AFFFFFFFLL * 50 },
        /* Division without remainder */
        { 0x2AFFFFFFFLL * 100, 100, 0x2AFFFFFFFLL, 0 },
        { -0x2AFFFFFFFLL * 100, 100, -0x2AFFFFFFFLL, 0 },
        { 0x2AFFFFFFFLL * 100, -100, -0x2AFFFFFFFLL, 0 },
        { -0x2AFFFFFFFLL * 100, -100, 0x2AFFFFFFFLL, 0 },
    };
    UDIV64_TEST_DATA udiv64[] =
    {
        /* Dividend is larger than divisor */
        { 34918215, 7, 4988316, 3 },
        /* Divisor is larger than dividend */
        { 12, 42, 0, 12 },
        /* Division without remainder */
        { 16777216, 65536, 256, 0 },

        /* Big 64-bit numbers */

        /* Dividend is larger than divisor */
        { 0x2AFFFFFFFULL * 100, 400, 2885681151ULL, 300 },
        /* Divisor is larger than dividend */
        { 0x2AFFFFFFFULL * 50, 0x2AFFFFFFFULL * 100, 0, 0x2AFFFFFFFULL * 50 },
        /* Division without remainder */
        { 0x2AFFFFFFFULL * 100, 100, 0x2AFFFFFFFULL, 0 },
    };
    INT i;

    for (i = 0; i < _countof(sdiv); i++)
    {
        LONG test_div, test_mod;

        test_div = sdiv[i].dividend / sdiv[i].divisor;
        test_mod = sdiv[i].dividend % sdiv[i].divisor;
        ok(test_div == sdiv[i].expected_div, "(sdiv) %d: Expected %ld, got %ld / %ld = %ld\n",
           i, sdiv[i].expected_div, sdiv[i].dividend, sdiv[i].divisor, test_div);
        ok(test_mod == sdiv[i].expected_mod, "(sdiv) %d: Expected %ld, got %ld %% %ld = %ld\n",
           i, sdiv[i].expected_mod, sdiv[i].dividend, sdiv[i].divisor, test_mod);
    }

    for (i = 0; i < _countof(udiv); i++)
    {
        ULONG test_div, test_mod;

        test_div = udiv[i].dividend / udiv[i].divisor;
        test_mod = udiv[i].dividend % udiv[i].divisor;
        ok(test_div == udiv[i].expected_div, "(udiv) %d: Expected %lu, got %lu / %lu = %lu\n",
           i, udiv[i].expected_div, udiv[i].dividend, udiv[i].divisor, test_div);
        ok(test_mod == udiv[i].expected_mod, "(udiv) %d: Expected %lu, got %lu %% %lu = %lu\n",
           i, udiv[i].expected_mod, udiv[i].dividend, udiv[i].divisor, test_mod);
    }

    for (i = 0; i < _countof(sdiv64); i++)
    {
        LONGLONG test_div, test_mod;

        test_div = sdiv64[i].dividend / sdiv64[i].divisor;
        test_mod = sdiv64[i].dividend % sdiv64[i].divisor;
        ok(test_div == sdiv64[i].expected_div, "(sdiv64) %d: Expected %lld, got %lld / %lld = %lld\n",
           i, sdiv64[i].expected_div, sdiv64[i].dividend, sdiv64[i].divisor, test_div);
        ok(test_mod == sdiv64[i].expected_mod, "(sdiv64) %d: Expected %lld, got %lld %% %lld = %lld\n",
           i, sdiv64[i].expected_mod, sdiv64[i].dividend, sdiv64[i].divisor, test_mod);
    }

    for (i = 0; i < _countof(udiv64); i++)
    {
        ULONGLONG test_div, test_mod;

        test_div = udiv64[i].dividend / udiv64[i].divisor;
        test_mod = udiv64[i].dividend % udiv64[i].divisor;
        ok(test_div == udiv64[i].expected_div, "(udiv64) %d: Expected %llu, got %llu / %llu = %llu\n",
           i, udiv64[i].expected_div, udiv64[i].dividend, udiv64[i].divisor, test_div);
        ok(test_mod == udiv64[i].expected_mod, "(udiv64) %d: Expected %llu, got %llu %% %llu = %llu\n",
           i, udiv64[i].expected_mod, udiv64[i].dividend, udiv64[i].divisor, test_mod);
    }
}

