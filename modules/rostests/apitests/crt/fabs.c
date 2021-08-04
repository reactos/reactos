/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for fabs / fabsf
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* Don't use the inline ceilf, unless required */
#if defined(TEST_STATIC_CRT) || defined(_M_ARM)
#define _CRTBLD
#endif
#include "math_helpers.h"

#ifdef _MSC_VER
#pragma function(fabs)
// fabsf is not available as an intrinsic
#endif

static TESTENTRY_DBL s_fabs_tests[] =
{
    /* Special values */
    { 0x7FF0000000000000ull /* INF        */, 0x7FF0000000000000ull /* INF        */ },
#ifdef _M_AMD64
    { 0x7FF0000000000001ull /* NAN(SNAN)  */, 0x7FF0000000000001ull /* NAN(SNAN) */ },
    { 0x7FF7FFFFFFFFFFFFull /* NAN(SNAN)  */, 0x7FF7FFFFFFFFFFFFull /* NAN(SNAN) */ },
#else
    { 0x7FF0000000000001ull /* NAN(SNAN)  */, 0x7FF8000000000001ull /* NAN       */ },
    { 0x7FF7FFFFFFFFFFFFull /* NAN(SNAN)  */, 0x7FFFFFFFFFFFFFFFull /* NAN       */ },
#endif
    { 0x7FF8000000000000ull /* NAN        */, 0x7FF8000000000000ull /* NAN       */ },
    { 0x7FF8000000000001ull /* NAN        */, 0x7FF8000000000001ull /* NAN       */ },
    { 0x7FFFFFFFFFFFFFFFull /* NAN        */, 0x7FFFFFFFFFFFFFFFull /* NAN       */ },
    { 0xFFF0000000000000ull /* -INF       */, 0x7FF0000000000000ull /* INF       */ },
#ifdef _M_AMD64
    { 0xFFF0000000000001ull /* -NAN(SNAN) */, 0xFFF0000000000001ull /* NAN(SNAN) */ },
    { 0xFFF7FFFFFFFFFFFFull /* -NAN(SNAN) */, 0xFFF7FFFFFFFFFFFFull /* NAN(SNAN) */ },
#else
    { 0xFFF0000000000001ull /* -NAN(SNAN) */, 0xFFF8000000000001ull /* -NAN       */ },
    { 0xFFF7FFFFFFFFFFFFull /* -NAN(SNAN) */, 0xFFFFFFFFFFFFFFFFull /* -NAN       */ },
#endif
    { 0xFFF8000000000000ull /* -NAN(IND)  */, 0xFFF8000000000000ull /* -NAN(IND)  */ },
    { 0xFFF8000000000001ull /* -NAN       */, 0xFFF8000000000001ull /* -NAN       */ },
    { 0xFFFFFFFFFFFFFFFFull /* -NAN       */, 0xFFFFFFFFFFFFFFFFull /* -NAN       */ },

    /* Some random floats */
    { 0x0000000000000000 /*  0.000000000000000e+000 */, 0x0000000000000000 /* 0.000000000000000e+000 */ },
    { 0x8000000000000000 /* -0.000000000000000e+000 */, 0x0000000000000000 /* 0.000000000000000e+000 */ },
    { 0x0123456789abcdef /*  3.512700564088504e-303 */, 0x0123456789abcdef /* 3.512700564088504e-303 */ },
    { 0x8123456789abcdef /* -3.512700564088504e-303 */, 0x0123456789abcdef /* 3.512700564088504e-303 */ },
    { 0x472ad8b31f506c9e /*  6.969745516432332e+034 */, 0x472ad8b31f506c9e /* 6.969745516432332e+034 */ },
    { 0xc72ad8b31f506c9e /* -6.969745516432332e+034 */, 0x472ad8b31f506c9e /* 6.969745516432332e+034 */ },
    { 0x1d289e506fa47cb3 /*  3.261613668384938e-168 */, 0x1d289e506fa47cb3 /* 3.261613668384938e-168 */ },
    { 0x9d289e506fa47cb3 /* -3.261613668384938e-168 */, 0x1d289e506fa47cb3 /* 3.261613668384938e-168 */ },
};


void Test_fabs(void)
{
    int i;

    for (i = 0; i < _countof(s_fabs_tests); i++)
    {
        double x = u64_to_dbl(s_fabs_tests[i].x);
        double z = fabs(x);
        ok_eq_dbl_exact("fabs", s_fabs_tests[i].x, z, s_fabs_tests[i].result);
    }
}

static TESTENTRY_FLT s_fabsf_tests[] =
{
    /* Special values */
    { 0x7F800000 /* INF        */, 0x7F800000 /* INF  */ },
    { 0x7F800001 /* NAN(SNAN)  */, 0x7FC00001 /* NAN  */ },
    { 0x7FBFFFFF /* NAN(SNAN)  */, 0x7FFFFFFF /* NAN  */ },
    { 0x7FC00000 /* NAN        */, 0x7FC00000 /* NAN  */ },
    { 0x7FC00001 /* NAN        */, 0x7FC00001 /* NAN  */ },
    { 0x7FCFFFFF /* NAN        */, 0x7FCFFFFF /* NAN  */ },
    { 0xFF800000 /* -INF       */, 0x7F800000 /* INF  */ },
    { 0xFF800001 /* -NAN(SNAN) */, 0xFFC00001 /* -NAN */ },
    { 0xFFBFFFFF /* -NAN(SNAN) */, 0xFFFFFFFF /* -NAN */ },
    { 0xFFC00000 /* -NAN(IND)  */, 0xFFC00000 /* -NAN */ },
    { 0xFFC00001 /* -NAN       */, 0xFFC00001 /* -NAN */ },
    { 0xFFCFFFFF /* -NAN       */, 0xFFCFFFFF /* -NAN */ },

    /* Some random floats */
    { 0x00000000 /*  0.000000e+000 */, 0x00000000 /* 0.000000e+000 */ },
    { 0x80000000 /* -0.000000e+000 */, 0x00000000 /* 0.000000e+000 */ },
    { 0x01234567 /*  2.998817e-038 */, 0x01234567 /* 2.998817e-038 */ },
    { 0x81234567 /* -2.998817e-038 */, 0x01234567 /* 2.998817e-038 */ },
    { 0x472ad8b3 /*  4.373670e+004 */, 0x472ad8b3 /* 4.373670e+004 */ },
    { 0xc72ad8b3 /* -4.373670e+004 */, 0x472ad8b3 /* 4.373670e+004 */ },
    { 0x1d289e50 /*  2.231646e-021 */, 0x1d289e50 /* 2.231646e-021 */ },
    { 0x9d289e50 /* -2.231646e-021 */, 0x1d289e50 /* 2.231646e-021 */ },
};

void Test_fabsf(void)
{
    int i;

    for (i = 0; i < _countof(s_fabsf_tests); i++)
    {
        float x = u32_to_flt(s_fabsf_tests[i].x);
        float z = fabsf(x);
        ok_eq_flt_exact("fabsf", s_fabsf_tests[i].x, z, s_fabsf_tests[i].result);
    }
}

START_TEST(fabs)
{
    Test_fabs();
    Test_fabsf();
}
