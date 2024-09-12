/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Tests for IEEE floatting-point functions
 * PROGRAMMER:      Pierre Schweitzer (pierre@reactos.org)
 * REFERENCES:      http://msdn.microsoft.com/en-US/library/h7zkk1bz%28v=VS.80%29.aspx
 */

#include <apitest.h>

#include <float.h>
#include <math.h>

typedef union
{
    double d;
    long long l;
} ieee_double;

void test_finite(void)
{
    ieee_double tested;

    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFF8000000000001LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFF8000000000000LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFF0000000000001LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFF0000000000000LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x8010000000000000LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x800FFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x8000000000000001LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x8000000000000000LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x0000000000000000LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x0000000000000001LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x000FFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x0010000000000000LL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == TRUE, "_finite = FALSE\n");
    tested.l = 0x7FF0000000000000LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0x7FF0000000000001LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0x7FF8000000000000LL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    ok(_finite(tested.d) == FALSE, "_finite = TRUE\n");

    /* MSDN example */
    ok(_finite(2.387000) == TRUE, "_finite = FALSE\n");
}

void test_fpclass(void)
{
    int class;
    ieee_double tested;

    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0xFFF8000000000001LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0xFFF8000000000000LL;
    class = _fpclass(tested.d);
    /* Normally it has no class, but w2k3 defines it
     * like that
     */
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
#ifdef _M_AMD64
    ok(class == _FPCLASS_SNAN, "class = %d\n", class);
#else
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
#endif
    tested.l = 0xFFF0000000000001LL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
#ifdef _M_AMD64
    ok(class == _FPCLASS_SNAN, "class = %d\n", class);
#else
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
#endif
    tested.l = 0xFFF0000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_NINF, "class = %d\n", class);
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_NN, "class = %d\n", class);
    tested.l = 0x8010000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_NN, "class = %d\n", class);
    tested.l = 0x800FFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_ND, "class = %d\n", class);
    tested.l = 0x8000000000000001LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_ND, "class = %d\n", class);
    tested.l = 0x8000000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_NZ, "class = %d\n", class);
    tested.l = 0x0000000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PZ, "class = %d\n", class);
    tested.l = 0x0000000000000001LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PD, "class = %d\n", class);
    tested.l = 0x000FFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PD, "class = %d\n", class);
    tested.l = 0x0010000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PN, "class = %d\n", class);
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PN, "class = %d\n", class);
    tested.l = 0x7FF0000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_PINF, "class = %d\n", class);
    tested.l = 0x7FF0000000000001LL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
#ifdef _M_AMD64
    ok(class == _FPCLASS_SNAN, "class = %d\n", class);
#else
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
#endif
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
#ifdef _M_AMD64
    ok(class == _FPCLASS_SNAN, "class = %d\n", class);
#else
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
#endif
    tested.l = 0x7FF8000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);

    /* MSDN example */
    class = _fpclass(2.387000);
    ok(class == _FPCLASS_PN, "class = %d\n", class);
}

void test_isnan(void)
{
    ieee_double tested;

    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0xFFF8000000000001LL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0xFFF8000000000000LL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0xFFF0000000000001LL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0xFFF0000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x8010000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x800FFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x8000000000000001LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x8000000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x0000000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x0000000000000001LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x000FFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x0010000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x7FF0000000000000LL;
    ok(_isnan(tested.d) == FALSE, "_isnan = TRUE\n");
    tested.l = 0x7FF0000000000001LL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0x7FF8000000000000LL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    ok(_isnan(tested.d) == TRUE, "_isnan = FALSE\n");

    /* MSDN example */
    ok(_isnan(2.387000) == FALSE, "_isnan = TRUE\n");
}

void test_j0(void)
{
    ieee_double tested;
    ieee_double expected;
    ieee_double result;

    expected.l = 0;

    errno = 0xDEADBEEF;
    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0x8000000000000000LL;
#else
    expected.l = 0x1FE7206E1D6FDCFALL;
#endif
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8010000000000000LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x800FFFFFFFFFFFFFLL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000001LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000000LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000000LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000001LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x000FFFFFFFFFFFFFLL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0010000000000000LL;
    expected.l = 0x3FF0000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0x8000000000000000LL;
#else
    expected.l = 0x1FE7206E1D6FDCFALL;
#endif
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000001LL;
    expected.l = 0x7FF8000000000001LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF8000000000000LL;
    expected.l = 0x7FF8000000000000LL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);

    /* MSDN example */
    errno = 0xDEADBEEF;
    tested.d = 2.387000;
    expected.l = 0x3F83059F9F6F30CALL;
    result.d =  _j0(tested.d);
    ok(result.l == expected.l, "_j0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
}

void test_j1(void)
{
    ieee_double tested;
    ieee_double expected;
    ieee_double result;

    expected.l = 0;

    errno = 0xDEADBEEF;
    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0;
#else
    expected.l = 0x9FE7206E1D6FDCFALL;
#endif
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8010000000000000LL;
    expected.l = 0x8008000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x800FFFFFFFFFFFFFLL;
    expected.l = 0x8008000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000001LL;
    expected.l = 0x8000000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000000LL;
    expected.l = 0x8000000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000000LL;
    expected.l = 0x0000000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000001LL;
    expected.l = 0x0000000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x000FFFFFFFFFFFFFLL;
    expected.l = 0x0008000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0010000000000000LL;
    expected.l = 0x0008000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0x8000000000000000LL;
#else
    expected.l = 0x1FE7206E1D6FDCFALL;
#endif
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x, expected %I64x\n", result.l, expected.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000001LL;
    expected.l = 0x7FF8000000000001LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF8000000000000LL;
    expected.l = 0x7FF8000000000000LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);

    /* MSDN example */
    errno = 0xDEADBEEF;
    tested.d = 2.387000;
    expected.l = 0x3FE0BBEFC62ABAB1LL;
    result.d =  _j1(tested.d);
    ok(result.l == expected.l, "_j1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
}

void test_scalb(void)
{
    ieee_double tested;
    ieee_double expected;
    ieee_double result;

    expected.l = 0;

    errno = 0xDEADBEEF;
    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000000LL;
    expected.l = 0xFFF0000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    expected.l = 0xFFF0000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8010000000000000LL;
    expected.l = 0x8040000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x800FFFFFFFFFFFFFLL;
    expected.l = 0x803FFFFFFFFFFFFELL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000001LL;
    expected.l = 0x8000000000000008LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000000LL;
    expected.l = 0x8000000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000000LL;
    expected.l = 0x0000000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000001LL;
    expected.l = 0x0000000000000008LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x000FFFFFFFFFFFFFLL;
    expected.l = 0x003FFFFFFFFFFFFELL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0010000000000000LL;
    expected.l = 0x0040000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
    expected.l = 0x7FF0000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000000LL;
    expected.l = 0x7FF0000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000001LL;
    expected.l = 0x7FF8000000000001LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF8000000000000LL;
    expected.l = 0x7FF8000000000000LL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);

    /* MSDN example */
    errno = 0xDEADBEEF;
    tested.d = 2.387000;
    expected.l = 0x4033189374BC6A7FLL;
    result.d =  _scalb(tested.d, 3);
    ok(result.l == expected.l, "_scalb returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
}

void test_y0(void)
{
    ieee_double tested;
    ieee_double expected;
    ieee_double result;

    expected.l = 0;

    errno = 0xDEADBEEF;
    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8010000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x800FFFFFFFFFFFFFLL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000001LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000000LL;
    expected.l = 0xFFF0000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000000LL;
    expected.l = 0xFFF0000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000001LL;
    expected.l = 0xC07D9FFC3469E1B3LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x000FFFFFFFFFFFFFLL;
    expected.l = 0xC07C30D8F820740ELL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0010000000000000LL;
    expected.l = 0xC07C30D8F820740ELL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0;
#else
    expected.l = 0x9FD5A36F8428F58BLL;
#endif
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000001LL;
    expected.l = 0x7FF8000000000001LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF8000000000000LL;
    expected.l = 0x7FF8000000000000LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);

    /* MSDN example */
    errno = 0xDEADBEEF;
    tested.d = 2.387000;
    expected.l = 0x3FE05FB1B1E49E66LL;
    result.d =  _y0(tested.d);
    ok(result.l == expected.l, "_y0 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
}

void test_y1(void)
{
    ieee_double tested;
    ieee_double expected;
    ieee_double result;

    expected.l = 0;

    errno = 0xDEADBEEF;
    tested.l = 0xFFFFFFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF8000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF7FFFFFFFFFFFFLL;
    expected.l = 0xFFFFFFFFFFFFFFFFLL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000001LL;
    expected.l = 0xFFF8000000000001LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0xFFEFFFFFFFFFFFFFLL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8010000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x800FFFFFFFFFFFFFLL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000001LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x8000000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == ERANGE, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0000000000000001LL;
    expected.l = 0xFFF0000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x000FFFFFFFFFFFFFLL;
    expected.l = 0xFFC45F306DC9C884LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x0010000000000000LL;
    expected.l = 0xFFC45F306DC9C883LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FEFFFFFFFFFFFFFLL;
#ifdef _M_AMD64
    expected.l = 0;
#else
    expected.l = 0x9FD5A36F8428F58BLL;
#endif
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000000LL;
    expected.l = 0xFFF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF0000000000001LL;
    expected.l = 0x7FF8000000000001LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FF8000000000000LL;
    expected.l = 0x7FF8000000000000LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);
    errno = 0xDEADBEEF;
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    expected.l = 0x7FFFFFFFFFFFFFFFLL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == EDOM, "errno: %d\n", errno);

    /* MSDN example */
    errno = 0xDEADBEEF;
    tested.d = 2.387000;
    expected.l = 0x3FB828EC13723EE6LL;
    result.d =  _y1(tested.d);
    ok(result.l == expected.l, "_y1 returned: %I64x\n", result.l);
    ok(errno == 0xDEADBEEF, "errno: %d\n", errno);
}

START_TEST(ieee)
{
    test_finite();
    test_fpclass();
    test_isnan();
    test_j0();
    test_j1();
    test_scalb();
    test_y0();
    test_y1();
}
