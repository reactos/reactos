/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for ceil / ceilf
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#if !defined(_CRTBLD) && !defined(_M_IX86)
#define _CRTBLD // we don't want inline ceilf!
#endif
#include "math_helpers.h"

#ifdef _MSC_VER
#pragma function(ceil)
// ceilf is not available as an intrinsic
#endif

static TESTENTRY_DBL s_ceil_tests[] =
{
    /* Special values */
    { 0x0000000000000000 /*  0.000000000000000e+000 */, 0x0000000000000000 /*  0.000000000000000e+000 */ },
    { 0x8000000000000000 /* -0.000000000000000e+000 */, 0x8000000000000000 /*  0.000000000000000e+000 */ },
    { 0x7ff0000000000000 /*  1.#INF00000000000e+000 */, 0x7ff0000000000000 /*  1.#INF00000000000e+000 */ },
    { 0x7ff0000000000001 /*  1.#QNAN0000000000e+000 */, 0x7ff8000000000001 /*  1.#QNAN0000000000e+000 */ },
    { 0x7ff7ffffffffffff /*  1.#QNAN0000000000e+000 */, 0x7fffffffffffffff /*  1.#QNAN0000000000e+000 */ },
    { 0x7ff8000000000000 /*  1.#QNAN0000000000e+000 */, 0x7ff8000000000000 /*  1.#QNAN0000000000e+000 */ },
    { 0x7ff8000000000001 /*  1.#QNAN0000000000e+000 */, 0x7ff8000000000001 /*  1.#QNAN0000000000e+000 */ },
    { 0x7fffffffffffffff /*  1.#QNAN0000000000e+000 */, 0x7fffffffffffffff /*  1.#QNAN0000000000e+000 */ },
    { 0xfff0000000000000 /* -1.#INF00000000000e+000 */, 0xfff0000000000000 /* -1.#INF00000000000e+000 */ },
    { 0xfff0000000000001 /* -1.#QNAN0000000000e+000 */, 0xfff8000000000001 /* -1.#QNAN0000000000e+000 */ },
    { 0xfff7ffffffffffff /* -1.#QNAN0000000000e+000 */, 0xffffffffffffffff /* -1.#QNAN0000000000e+000 */ },
    { 0xfff8000000000000 /* -1.#IND00000000000e+000 */, 0xfff8000000000000 /* -1.#IND00000000000e+000 */ },
    { 0xfff8000000000001 /* -1.#QNAN0000000000e+000 */, 0xfff8000000000001 /* -1.#QNAN0000000000e+000 */ },
    { 0xffffffffffffffff /* -1.#QNAN0000000000e+000 */, 0xffffffffffffffff /* -1.#QNAN0000000000e+000 */ },

    /* Some random floats */
    { 0x84be2329aed66ce1 /* -7.916792434840887e-286 */, 0x8000000000000000 /* -0.000000000000000e+000 */ },
    { 0xf1499052ebe9bbf1 /* -5.202012813127544e+237 */, 0xf1499052ebe9bbf1 /* -5.202012813127544e+237 */ },
    { 0x3cdba6b3993e0c87 /*  1.534948721304537e-015 */, 0x3ff0000000000000 /*  1.000000000000000e+000 */ },
    { 0x1c0d5e24de47b706 /*  1.484236768428990e-173 */, 0x3ff0000000000000 /*  1.000000000000000e+000 */ },
    { 0xc84d12b3a68bbb43 /* -1.978609508743937e+040 */, 0xc84d12b3a68bbb43 /* -1.978609508743937e+040 */ },
    { 0x7d5a031f1f253809 /*  6.645271626742043e+295 */, 0x7d5a031f1f253809 /*  6.645271626742043e+295 */ },
    { 0xfccbd45d3b45f596 /* -1.388583322422121e+293 */, 0xfccbd45d3b45f596 /* -1.388583322422121e+293 */ },
    { 0x0a890d1332aedb1c /*  6.517185427488806e-258 */, 0x3ff0000000000000 /*  1.000000000000000e+000 */ },
    { 0xee509a20fd367840 /* -2.400484647490954e+223 */, 0xee509a20fd367840 /* -2.400484647490954e+223 */ },
    { 0xf6324912dc497d9e /* -2.249167320514119e+261 */, 0xf6324912dc497d9e /* -2.249167320514119e+261 */ },
};


void Test_ceil(void)
{
    int i;

    for (i = 0; i < _countof(s_ceil_tests); i++)
    {
        double x = u64_to_dbl(s_ceil_tests[i].x);
        double z = ceil(x);
        ok_eq_dbl_exact("ceil", s_ceil_tests[i].x, z, s_ceil_tests[i].result);
    }
}

static TESTENTRY_FLT s_ceilf_tests[] =
{
    /* Special values */
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /*  0.000000e+000 */ },
    { 0x80000000 /* -0.000000e+000 */, 0x80000000 /*  0.000000e+000 */ },
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /*  0.000000e+000 */ },
    { 0x00000001 /*  1.401298e-045 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0xffffffff /* -1.#QNAN0e+000 */, 0xffffffff /* -1.#QNAN0e+000 */ },
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /*  0.000000e+000 */ },
    { 0x00000001 /*  1.401298e-045 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0xffffffff /* -1.#QNAN0e+000 */, 0xffffffff /* -1.#QNAN0e+000 */ },
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /*  0.000000e+000 */ },
    { 0x00000001 /*  1.401298e-045 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0xffffffff /* -1.#QNAN0e+000 */, 0xffffffff /* -1.#QNAN0e+000 */ },
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /*  0.000000e+000 */ },
    { 0x00000001 /*  1.401298e-045 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0xffffffff /* -1.#QNAN0e+000 */, 0xffffffff /* -1.#QNAN0e+000 */ },

    /* Some random floats */
    { 0xf2144fad /* -2.937607e+030 */, 0xf2144fad /* -2.937607e+030 */ },
    { 0xd0664044 /* -1.545189e+010 */, 0xd0664044 /* -1.545189e+010 */ },
    { 0xb730c46b /* -1.053615e-005 */, 0x80000000 /* -0.000000e+000 */ },
    { 0x22a13b32 /*  4.370181e-018 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0x9d9122f6 /* -3.841733e-021 */, 0x80000000 /* -0.000000e+000 */ },
    { 0xda1f8be1 /* -1.122708e+016 */, 0xda1f8be1 /* -1.122708e+016 */ },
    { 0x0299cab0 /*  2.259767e-037 */, 0x3f800000 /*  1.000000e+000 */ },
    { 0x499d72b9 /*  1.289815e+006 */, 0x499d72c0 /*  1.289816e+006 */ },
    { 0xc57e802c /* -4.072011e+003 */, 0xc57e8000 /* -4.072000e+003 */ },
    { 0x80e9d599 /* -2.147430e-038 */, 0x80000000 /* -0.000000e+000 */ },
};

void Test_ceilf(void)
{
    int i;

    for (i = 0; i < _countof(s_ceilf_tests); i++)
    {
        float x = u32_to_flt(s_ceilf_tests[i].x);
        float z = ceilf(x);
        ok_eq_flt_exact("ceilf", s_ceilf_tests[i].x, z, s_ceilf_tests[i].result);
    }
}

START_TEST(ceil)
{
    Test_ceil();
    Test_ceilf();
}
