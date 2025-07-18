/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _isnan / _isnanf
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#if !defined(_CRTBLD) && !defined(_M_IX86)
#define _CRTBLD // we don't want inline _isnanf!
#endif
#include "math_helpers.h"

static TESTENTRY_DBL_INT s_isnan_tests[] =
{
    /* Special values */
    { 0x0000000000000000 /*  0.000000000000000e+000 */, 0 },
    { 0x0000000000000001 /*  0.000000000000000e+000 */, 0 },
    { 0x000fffffffffffff /*  0.000000000000000e+000 */, 0 },
    { 0x8000000000000000 /* -0.000000000000000e+000 */, 0 },
    { 0x8000000000000001 /* -0.000000000000000e+000 */, 0 },
    { 0x800fffffffffffff /* -0.000000000000000e+000 */, 0 },
    { 0x7ff0000000000000 /*  1.#INF00000000000e+000 */, 0 },
    { 0x7ff0000000000001 /*  1.#QNAN0000000000e+000 */, 1 },
    { 0x7ff7ffffffffffff /*  1.#QNAN0000000000e+000 */, 1 },
    { 0x7ff8000000000000 /*  1.#QNAN0000000000e+000 */, 1 },
    { 0x7ff8000000000001 /*  1.#QNAN0000000000e+000 */, 1 },
    { 0x7fffffffffffffff /*  1.#QNAN0000000000e+000 */, 1 },
    { 0xfff0000000000000 /* -1.#INF00000000000e+000 */, 0 },
    { 0xfff0000000000001 /* -1.#QNAN0000000000e+000 */, 1 },
    { 0xfff7ffffffffffff /* -1.#QNAN0000000000e+000 */, 1 },
    { 0xfff8000000000000 /* -1.#IND00000000000e+000 */, 1 },
    { 0xfff8000000000001 /* -1.#QNAN0000000000e+000 */, 1 },
    { 0xffffffffffffffff /* -1.#QNAN0000000000e+000 */, 1 },

    /* Some random doubles */
    { 0x386580c747a3402b /*  5.055340589883462e-037 */, 0 },
    { 0xb74298e6627fb9ed /* -1.667860443847725e-042 */, 0 },
    { 0x0ef25f06e414aa2d /*  1.128498317470960e-236 */, 0 },
    { 0x24002a37167638b5 /*  2.780001692929186e-135 */, 0 },
    { 0x44258d1be62a0d22 /*  1.987747995999515e+020 */, 0 },
    { 0x9ed4e46a65aad464 /* -3.715074250469020e-160 */, 0 },
    { 0xc5afcd6f4ae4bf41 /* -4.921195330852160e+027 */, 0 },
    { 0x330fac896cbb01d2 /*  9.624395081137827e-063 */, 0 },
    { 0xc18026ab4c845405 /* -3.387120956461338e+007 */, 0 },
    { 0x2f42a7dc898a741a /*  4.916804395045249e-081 */, 0 },
};


void Test__isnan(void)
{
    int i;

    for (i = 0; i < _countof(s_isnan_tests); i++)
    {
        double x = u64_to_dbl(s_isnan_tests[i].x);
        int r = _isnan(x);
        ok(r == s_isnan_tests[i].result, "Wrong result for %f [0x%016I64x]. Expected %d, got %d\n", x, s_isnan_tests[i].x, s_isnan_tests[i].result, r);
    }
}

#ifndef _M_IX86
static TESTENTRY_FLT s_isnanf_tests[] =
{
    /* Special values */
    { 0x00000000 /*  0.000000 */, 0 },
    { 0x00000001 /*  0.000000 */, 0 },
    { 0x007FFFFF /*  0.000000 */, 0 },
    { 0x80000000 /* -0.000000 */, 0 },
    { 0x80000001 /* -0.000000 */, 0 },
    { 0x807FFFFF /* -0.000000 */, 0 },
    { 0x7f800000 /*  1.#INF00 */, 0 },
    { 0x7f800001 /*  1.#SNAN0 */, 1 },
    { 0x7fBFffff /*  1.#SNAN0 */, 1 },
    { 0x7fC00000 /*  1.#QNAN0 */, 1 },
    { 0x7fC80001 /*  1.#QNAN0 */, 1 },
    { 0x7fFfffff /*  1.#QNAN0 */, 1 },
    { 0xff800000 /* -1.#INF00 */, 0 },
    { 0xff800001 /* -1.#SNAN0 */, 1 },
    { 0xffBfffff /* -1.#SNAN0 */, 1 },
    { 0xffC00000 /* -1.#IND00 */, 1 },
    { 0xfff80001 /* -1.#QNAN0 */, 1 },
    { 0xffffffff /* -1.#QNAN0 */, 1 },

    /* Some random floats */
    { 0x386580c7 /*  5.471779e-005 */, 0 },
    { 0x47a3402b /*  8.358434e+004 */, 0 },
    { 0xb74298e6 /* -1.159890e-005 */, 0 },
    { 0x627fb9ed /*  1.179329e+021 */, 0 },
    { 0x0ef25f06 /*  5.974911e-030 */, 0 },
    { 0xe414aa2d /* -1.096952e+022 */, 0 },
    { 0x24002a37 /*  2.779133e-017 */, 0 },
    { 0x167638b5 /*  1.988962e-025 */, 0 },
    { 0x44258d1b /*  6.622048e+002 */, 0 },
    { 0xe62a0d22 /* -2.007611e+023 */, 0 },
};

void Test__isnanf(void)
{
    int i;

    for (i = 0; i < _countof(s_isnanf_tests); i++)
    {
        float x = u32_to_flt(s_isnanf_tests[i].x);
        int r = _isnanf(x);
        ok(r == s_isnanf_tests[i].result, "Wrong result for %f [0x%08lx]. Expected %d, got %d\n", x, s_isnanf_tests[i].x, s_isnanf_tests[i].result, r);
    }
}
#endif // !_MIX86

START_TEST(_isnan)
{
    Test__isnan();
#ifndef _M_IX86
    Test__isnanf();
#endif // !_MIX86
}
