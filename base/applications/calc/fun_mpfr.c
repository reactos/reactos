#include "calc.h"
#include <limits.h>

void apply_int_mask(calc_number_t *r)
{
    mpz_t a, mask;

    switch (calc.size) {
    case IDC_RADIO_QWORD:
        mpz_init_set_str(mask, "FFFFFFFFFFFFFFFF", 16);
        break;
    case IDC_RADIO_DWORD:
        mpz_init_set_str(mask, "00000000FFFFFFFF", 16);
        break;
    case IDC_RADIO_WORD:
        mpz_init_set_str(mask, "000000000000FFFF", 16);
        break;
    case IDC_RADIO_BYTE:
        mpz_init_set_str(mask, "00000000000000FF", 16);
        break;
    default:
        mpz_init_set_si(mask, -1);
    }
    mpz_init(a);
    mpfr_get_z(a, r->mf, MPFR_DEFAULT_RND);
    mpz_and(a, a, mask);
    mpfr_set_z(r->mf, a, MPFR_DEFAULT_RND);
    mpz_clear(a);
    mpz_clear(mask);
}

void validate_rad2angle(calc_number_t *r)
{
    mpfr_t mult, divs;

    mpfr_init(mult);
    mpfr_init(divs);
    switch (calc.degr) {
    case IDC_RADIO_DEG:
        mpfr_set_ui(mult, 180, MPFR_DEFAULT_RND);
        mpfr_const_pi(divs, MPFR_DEFAULT_RND);
        break;
    case IDC_RADIO_RAD:
        mpfr_set_ui(mult, 1, MPFR_DEFAULT_RND);
        mpfr_set_ui(divs, 1, MPFR_DEFAULT_RND);
        break;
    case IDC_RADIO_GRAD:
        mpfr_set_ui(mult, 200, MPFR_DEFAULT_RND);
        mpfr_const_pi(divs, MPFR_DEFAULT_RND);
        break;
    }
    mpfr_mul(r->mf, r->mf, mult, MPFR_DEFAULT_RND);
    mpfr_div(r->mf, r->mf, divs, MPFR_DEFAULT_RND);

    mpfr_clear(mult);
    mpfr_clear(divs);
}

void validate_angle2rad(calc_number_t *r)
{
    mpfr_t mult, divs;

    if (!mpfr_number_p(r->mf)) {
        calc.is_nan = TRUE;
        return;
    }
    mpfr_init(mult);
    mpfr_init(divs);
    switch (calc.degr) {
    case IDC_RADIO_DEG:
        mpfr_const_pi(mult, MPFR_DEFAULT_RND);
        mpfr_set_ui(divs, 180, MPFR_DEFAULT_RND);
        break;
    case IDC_RADIO_RAD:
        mpfr_set_ui(mult, 1, MPFR_DEFAULT_RND);
        mpfr_set_ui(divs, 1, MPFR_DEFAULT_RND);
        break;
    case IDC_RADIO_GRAD:
        mpfr_const_pi(mult, MPFR_DEFAULT_RND);
        mpfr_set_ui(divs, 200, MPFR_DEFAULT_RND);
        break;
    }
    mpfr_mul(r->mf, r->mf, mult, MPFR_DEFAULT_RND);
    mpfr_div(r->mf, r->mf, divs, MPFR_DEFAULT_RND);

    mpfr_clear(mult);
    mpfr_clear(divs);
}

static void build_rad_const(
    mpfr_t *mp_pi,
    mpfr_t *mp_pi_2,
    mpfr_t *mp_3_pi_2,
    mpfr_t *mp_2_pi)
{
    mpfr_init(*mp_pi);
    mpfr_init(*mp_pi_2);
    mpfr_init(*mp_3_pi_2);
    mpfr_init(*mp_2_pi);
    mpfr_const_pi(*mp_pi, MPFR_DEFAULT_RND);
    mpfr_div_ui(*mp_pi_2, *mp_pi, 2, MPFR_DEFAULT_RND);
    mpfr_mul_ui(*mp_3_pi_2, *mp_pi, 3, MPFR_DEFAULT_RND);
    mpfr_div_ui(*mp_3_pi_2, *mp_3_pi_2, 2, MPFR_DEFAULT_RND);
    mpfr_mul_ui(*mp_2_pi, *mp_pi, 2, MPFR_DEFAULT_RND);
}

void rpn_sin(calc_number_t *c)
{
    mpfr_t mp_pi, mp_pi_2, mp_3_pi_2, mp_2_pi;

    validate_angle2rad(c);
    build_rad_const(&mp_pi, &mp_pi_2, &mp_3_pi_2, &mp_2_pi);

    if (rpn_is_zero(c) || !mpfr_cmp(c->mf, mp_pi) || !mpfr_cmp(c->mf, mp_2_pi))
        rpn_zero(c);
    else
    if (!mpfr_cmp(c->mf, mp_3_pi_2))
        mpfr_set_si(c->mf, -1, MPFR_DEFAULT_RND);
    else
    if (!mpfr_cmp(c->mf, mp_pi_2))
        mpfr_set_si(c->mf, 1, MPFR_DEFAULT_RND);
    else {
        mpfr_sin(c->mf, c->mf, MPFR_DEFAULT_RND);
        if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
    }
    mpfr_clear(mp_pi);
    mpfr_clear(mp_pi_2);
    mpfr_clear(mp_3_pi_2);
    mpfr_clear(mp_2_pi);
}
void rpn_cos(calc_number_t *c)
{
    mpfr_t mp_pi, mp_pi_2, mp_3_pi_2, mp_2_pi;

    validate_angle2rad(c);
    build_rad_const(&mp_pi, &mp_pi_2, &mp_3_pi_2, &mp_2_pi);

    if (!mpfr_cmp(c->mf, mp_pi_2) || !mpfr_cmp(c->mf, mp_3_pi_2))
        rpn_zero(c);
    else
    if (!mpfr_cmp(c->mf, mp_pi))
        mpfr_set_si(c->mf, -1, MPFR_DEFAULT_RND);
    else
    if (!mpfr_cmp(c->mf, mp_2_pi))
        mpfr_set_si(c->mf, 1, MPFR_DEFAULT_RND);
    else {
        mpfr_cos(c->mf, c->mf, MPFR_DEFAULT_RND);
        if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
    }
    mpfr_clear(mp_pi);
    mpfr_clear(mp_pi_2);
    mpfr_clear(mp_3_pi_2);
    mpfr_clear(mp_2_pi);
}
void rpn_tan(calc_number_t *c)
{
    mpfr_t mp_pi, mp_pi_2, mp_3_pi_2, mp_2_pi;

    validate_angle2rad(c);
    build_rad_const(&mp_pi, &mp_pi_2, &mp_3_pi_2, &mp_2_pi);

    if (!mpfr_cmp(c->mf, mp_pi_2) || !mpfr_cmp(c->mf, mp_3_pi_2))
        calc.is_nan = TRUE;
    else
    if (!mpfr_cmp(c->mf, mp_pi) || !mpfr_cmp(c->mf, mp_2_pi))
        rpn_zero(c);
    else {
        mpfr_tan(c->mf, c->mf, MPFR_DEFAULT_RND);
        if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
    }
    mpfr_clear(mp_pi);
    mpfr_clear(mp_pi_2);
    mpfr_clear(mp_3_pi_2);
    mpfr_clear(mp_2_pi);
}

void rpn_asin(calc_number_t *c)
{
    mpfr_asin(c->mf, c->mf, MPFR_DEFAULT_RND);
    validate_rad2angle(c);
}
void rpn_acos(calc_number_t *c)
{
    mpfr_acos(c->mf, c->mf, MPFR_DEFAULT_RND);
    validate_rad2angle(c);
}
void rpn_atan(calc_number_t *c)
{
    mpfr_atan(c->mf, c->mf, MPFR_DEFAULT_RND);
    validate_rad2angle(c);
}

void rpn_sinh(calc_number_t *c)
{
    mpfr_sinh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}
void rpn_cosh(calc_number_t *c)
{
    mpfr_cosh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}
void rpn_tanh(calc_number_t *c)
{
    mpfr_tanh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_asinh(calc_number_t *c)
{
    mpfr_asinh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}
void rpn_acosh(calc_number_t *c)
{
    mpfr_acosh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}
void rpn_atanh(calc_number_t *c)
{
    mpfr_atanh(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_int(calc_number_t *c)
{
    mpfr_trunc(c->mf, c->mf);
}

void rpn_frac(calc_number_t *c)
{
    mpfr_frac(c->mf, c->mf, MPFR_DEFAULT_RND);
}

void rpn_reci(calc_number_t *c)
{
    if (mpfr_sgn(c->mf) == 0)
        calc.is_nan = TRUE;
    else
        mpfr_ui_div(c->mf, 1, c->mf, MPFR_DEFAULT_RND);
}

void rpn_fact(calc_number_t *c)
{
    if (mpfr_sgn(c->mf) < 0) {
        calc.is_nan = TRUE;
        return;
    }

    mpfr_trunc(c->mf, c->mf);
    if (mpfr_fits_ulong_p(c->mf, MPFR_DEFAULT_RND) == 0)
        calc.is_nan = TRUE;
    else {
        mpfr_fac_ui(c->mf, mpfr_get_ui(c->mf, MPFR_DEFAULT_RND), MPFR_DEFAULT_RND);
        if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
    }
}

void rpn_not(calc_number_t *c)
{
    mpz_t a;

    mpz_init(a);
    mpfr_get_z(a, c->mf, MPFR_DEFAULT_RND);
    mpz_com(a, a);
    mpfr_set_z(c->mf, a, MPFR_DEFAULT_RND);
    mpz_clear(a);
}

void rpn_pi(calc_number_t *c)
{
    mpfr_const_pi(c->mf, MPFR_DEFAULT_RND);
}

void rpn_2pi(calc_number_t *c)
{
    mpfr_const_pi(c->mf, MPFR_DEFAULT_RND);
    mpfr_mul_ui(c->mf, c->mf, 2, MPFR_DEFAULT_RND);
}

void rpn_sign(calc_number_t *c)
{
    mpfr_mul_si(c->mf, c->mf, -1, MPFR_DEFAULT_RND);
}

void rpn_exp2(calc_number_t *c)
{
    mpfr_sqr(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_exp3(calc_number_t *c)
{
    mpfr_pow_ui(c->mf, c->mf, 3, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_sqrt(calc_number_t *c)
{
    mpfr_sqrt(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_cbrt(calc_number_t *c)
{
    mpfr_cbrt(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_exp(calc_number_t *c)
{
    mpfr_exp(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_exp10(calc_number_t *c)
{
    mpfr_exp10(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_ln(calc_number_t *c)
{
    mpfr_log(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

void rpn_log(calc_number_t *c)
{
    mpfr_log10(c->mf, c->mf, MPFR_DEFAULT_RND);
    if (!mpfr_number_p(c->mf)) calc.is_nan = TRUE;
}

static void stat_sum(mpfr_t sum)
{
    statistic_t *p = calc.stat;

    mpfr_set_ui(sum, 0, MPFR_DEFAULT_RND);
    while (p != NULL) {
        mpfr_add(sum, sum, p->num.mf, MPFR_DEFAULT_RND);
        p = (statistic_t *)(p->next);
    }
}

void rpn_ave(calc_number_t *c)
{
    int     n;

    stat_sum(c->mf);
    n = SendDlgItemMessage(calc.hStatWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);

    if (n)
        mpfr_div_ui(c->mf, c->mf, n, MPFR_DEFAULT_RND);

    if (calc.base != IDC_RADIO_DEC)
        mpfr_trunc(c->mf, c->mf);
}

void rpn_sum(calc_number_t *c)
{
    stat_sum(c->mf);

    if (calc.base != IDC_RADIO_DEC)
        mpfr_trunc(c->mf, c->mf);
}

static void rpn_s_ex(calc_number_t *c, int pop_type)
{
    mpfr_t        dev;
    mpfr_t        num;
    unsigned long n = 0;
    statistic_t  *p = calc.stat;

    n = SendDlgItemMessage(calc.hStatWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);
    if (n < 2) {
        mpfr_set_ui(c->mf, 0, MPFR_DEFAULT_RND);
        return;
    }

    stat_sum(c->mf);
    mpfr_div_ui(c->mf, c->mf, n, MPFR_DEFAULT_RND);

    mpfr_init(dev);
    mpfr_init(num);

    mpfr_set_ui(dev, 0, MPFR_DEFAULT_RND);
    p = calc.stat;
    while (p != NULL) {
        mpfr_sub(num, p->num.mf, c->mf, MPFR_DEFAULT_RND);
        mpfr_sqr(num, num, MPFR_DEFAULT_RND);
        mpfr_add(dev, dev, num, MPFR_DEFAULT_RND);
        p = (statistic_t *)(p->next);
    }
    mpfr_div_ui(c->mf, dev, pop_type ? n-1 : n, MPFR_DEFAULT_RND);
    mpfr_sqrt(c->mf, c->mf, MPFR_DEFAULT_RND);

    if (calc.base != IDC_RADIO_DEC)
        mpfr_trunc(c->mf, c->mf);

    mpfr_clear(dev);
    mpfr_clear(num);
}

void rpn_s(calc_number_t *c)
{
    rpn_s_ex(c, 0);
}

void rpn_s_m1(calc_number_t *c)
{
    rpn_s_ex(c, 1);
}

void rpn_dms2dec(calc_number_t *c)
{
    mpfr_t d, m, s;

    mpfr_init(d);
    mpfr_init(m);
    mpfr_init(s);

    mpfr_trunc(d, c->mf);
    mpfr_frac(m, c->mf, MPFR_DEFAULT_RND);
    mpfr_mul_ui(m, m, 100, MPFR_DEFAULT_RND);

    mpfr_frac(s, m, MPFR_DEFAULT_RND);
    mpfr_trunc(m, m);
    mpfr_mul_ui(s, s, 100, MPFR_DEFAULT_RND);
    mpfr_ceil(s, s);

    mpfr_div_ui(m, m, 60, MPFR_DEFAULT_RND);
    mpfr_div_ui(s, s, 3600, MPFR_DEFAULT_RND);
    mpfr_add(c->mf, d, m, MPFR_DEFAULT_RND);
    mpfr_add(c->mf, c->mf, s, MPFR_DEFAULT_RND);

    mpfr_clear(d);
    mpfr_clear(m);
    mpfr_clear(s);
}

void rpn_dec2dms(calc_number_t *c)
{
    mpfr_t d, m, s;

    mpfr_init(d);
    mpfr_init(m);
    mpfr_init(s);

    mpfr_trunc(d, c->mf);
    mpfr_frac(m, c->mf, MPFR_DEFAULT_RND);
    mpfr_mul_ui(m, m, 60, MPFR_DEFAULT_RND);

    mpfr_frac(s, m, MPFR_DEFAULT_RND);
    mpfr_trunc(m, m);
    mpfr_mul_ui(s, s, 60, MPFR_DEFAULT_RND);
    mpfr_ceil(s, s);

    mpfr_div_ui(m, m, 100, MPFR_DEFAULT_RND);
    mpfr_div_ui(s, s, 10000, MPFR_DEFAULT_RND);
    mpfr_add(c->mf, d, m, MPFR_DEFAULT_RND);
    mpfr_add(c->mf, c->mf, s, MPFR_DEFAULT_RND);

    mpfr_clear(d);
    mpfr_clear(m);
    mpfr_clear(s);
}

void rpn_zero(calc_number_t *c)
{
    mpfr_set_ui(c->mf, 0, MPFR_DEFAULT_RND);
}

void rpn_copy(calc_number_t *dst, calc_number_t *src)
{
    mpfr_set(dst->mf, src->mf, MPFR_DEFAULT_RND);
}

int rpn_is_zero(calc_number_t *c)
{
    return (mpfr_sgn(c->mf) == 0);
}

void rpn_alloc(calc_number_t *c)
{
    mpfr_init(c->mf);
}

void rpn_free(calc_number_t *c)
{
    mpfr_clear(c->mf);
}
