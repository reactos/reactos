/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Tests for IEEE floatting-point functions
 * PROGRAMMER:      Pierre Schweitzer (pierre@reactos.org)
 */

#include <wine/test.h>
#include <float.h>

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
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0xFFF0000000000001LL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
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
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0x7FF7FFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    /* According to IEEE, it should be Signaling NaN, but
     * on w2k3, it's Quiet NAN
     * ok(class == _FPCLASS_SNAN, "class = %d\n", class);
     */
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0x7FF8000000000000LL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
    tested.l = 0x7FFFFFFFFFFFFFFFLL;
    class = _fpclass(tested.d);
    ok(class == _FPCLASS_QNAN, "class = %d\n", class);
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
}

START_TEST(ieee)
{
    test_finite();
    test_fpclass();
    test_isnan();
}
