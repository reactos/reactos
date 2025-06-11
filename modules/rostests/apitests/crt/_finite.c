/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _finite / _finitef
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#if !defined(_CRTBLD) && !defined(_M_IX86)
#define _CRTBLD // we don't want inline _finitef!
#endif
#include "math_helpers.h"

static TESTENTRY_DBL_INT s_finite_tests[] =
{
    /* Special values */
    { 0x0000000000000000 /*  0.000000000000000e+000 */, 1 },
    { 0x0000000000000001 /*  0.000000000000000e+000 */, 1 },
    { 0x000fffffffffffff /*  0.000000000000000e+000 */, 1 },
    { 0x8000000000000000 /* -0.000000000000000e+000 */, 1 },
    { 0x8000000000000001 /* -0.000000000000000e+000 */, 1 },
    { 0x800fffffffffffff /* -0.000000000000000e+000 */, 1 },
    { 0x7ff0000000000000 /*  1.#INF00000000000e+000 */, 0 },
    { 0x7ff0000000000001 /*  1.#QNAN0000000000e+000 */, 0 },
    { 0x7ff7ffffffffffff /*  1.#QNAN0000000000e+000 */, 0 },
    { 0x7ff8000000000000 /*  1.#QNAN0000000000e+000 */, 0 },
    { 0x7ff8000000000001 /*  1.#QNAN0000000000e+000 */, 0 },
    { 0x7fffffffffffffff /*  1.#QNAN0000000000e+000 */, 0 },
    { 0xfff0000000000000 /* -1.#INF00000000000e+000 */, 0 },
    { 0xfff0000000000001 /* -1.#QNAN0000000000e+000 */, 0 },
    { 0xfff7ffffffffffff /* -1.#QNAN0000000000e+000 */, 0 },
    { 0xfff8000000000000 /* -1.#IND00000000000e+000 */, 0 },
    { 0xfff8000000000001 /* -1.#QNAN0000000000e+000 */, 0 },
    { 0xffffffffffffffff /* -1.#QNAN0000000000e+000 */, 0 },

    /* Some random doubles */
    { 0x386580c747a3402b /*  5.055340589883462e-037 */, 1 },
    { 0xb74298e6627fb9ed /* -1.667860443847725e-042 */, 1 },
    { 0x0ef25f06e414aa2d /*  1.128498317470960e-236 */, 1 },
    { 0x24002a37167638b5 /*  2.780001692929186e-135 */, 1 },
    { 0x44258d1be62a0d22 /*  1.987747995999515e+020 */, 1 },
    { 0x9ed4e46a65aad464 /* -3.715074250469020e-160 */, 1 },
    { 0xc5afcd6f4ae4bf41 /* -4.921195330852160e+027 */, 1 },
    { 0x330fac896cbb01d2 /*  9.624395081137827e-063 */, 1 },
    { 0xc18026ab4c845405 /* -3.387120956461338e+007 */, 1 },
    { 0x2f42a7dc898a741a /*  4.916804395045249e-081 */, 1 },
};


void Test__finite(void)
{
    int i;

    for (i = 0; i < _countof(s_finite_tests); i++)
    {
        double x = u64_to_dbl(s_finite_tests[i].x);
        int r = _finite(x);
        ok(r == s_finite_tests[i].result, "Wrong result for %f [0x%016I64x]. Expected %d, got %d\n", x, s_finite_tests[i].x, s_finite_tests[i].result, r);
    }
}

#ifndef _M_IX86
static TESTENTRY_FLT s_finitef_tests[] =
{
    /* Special values */
    { 0x00000000 /*  0.000000 */, 1 },
    { 0x00000001 /*  0.000000 */, 1 },
    { 0x007FFFFF /*  0.000000 */, 1 },
    { 0x80000000 /* -0.000000 */, 1 },
    { 0x80000001 /* -0.000000 */, 1 },
    { 0x807FFFFF /* -0.000000 */, 1 },
    { 0x7f800000 /*  1.#INF00 */, 0 },
    { 0x7f800001 /*  1.#SNAN0 */, 0 },
    { 0x7fBFffff /*  1.#SNAN0 */, 0 },
    { 0x7fC00000 /*  1.#QNAN0 */, 0 },
    { 0x7fC80001 /*  1.#QNAN0 */, 0 },
    { 0x7fFfffff /*  1.#QNAN0 */, 0 },
    { 0xff800000 /* -1.#INF00 */, 0 },
    { 0xff800001 /* -1.#SNAN0 */, 0 },
    { 0xffBfffff /* -1.#SNAN0 */, 0 },
    { 0xffC00000 /* -1.#IND00 */, 0 },
    { 0xfff80001 /* -1.#QNAN0 */, 0 },
    { 0xffffffff /* -1.#QNAN0 */, 0 },

    /* Some random floats */
    { 0x386580c7 /*  5.471779e-005 */, 1 },
    { 0x47a3402b /*  8.358434e+004 */, 1 },
    { 0xb74298e6 /* -1.159890e-005 */, 1 },
    { 0x627fb9ed /*  1.179329e+021 */, 1 },
    { 0x0ef25f06 /*  5.974911e-030 */, 1 },
    { 0xe414aa2d /* -1.096952e+022 */, 1 },
    { 0x24002a37 /*  2.779133e-017 */, 1 },
    { 0x167638b5 /*  1.988962e-025 */, 1 },
    { 0x44258d1b /*  6.622048e+002 */, 1 },
    { 0xe62a0d22 /* -2.007611e+023 */, 1 },
};

void Test__finitef(void)
{
    int i;

    for (i = 0; i < _countof(s_finitef_tests); i++)
    {
        float x = u32_to_flt(s_finitef_tests[i].x);
        int r = _finitef(x);
        ok(r == s_finitef_tests[i].result, "Wrong result for %f [0x%08lx]. Expected %d, got %d\n", x, s_finitef_tests[i].x, s_finitef_tests[i].result, r);
    }
}
#endif // !_MIX86

START_TEST(_finite)
{
    Test__finite();
#ifndef _M_IX86
    Test__finitef();
#endif // !_MIX86
}
