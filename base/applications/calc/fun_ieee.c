/*
 * ReactOS Calc (Math functions, IEEE-754 engine)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

static double validate_rad2angle(double a);
static double validate_angle2rad(calc_number_t *c);

void apply_int_mask(calc_number_t *r)
{
    unsigned __int64 mask;

    switch (calc.size) {
    case IDC_RADIO_QWORD:
        mask = _UI64_MAX;
        break;
    case IDC_RADIO_DWORD:
        mask = ULONG_MAX;
        break;
    case IDC_RADIO_WORD:
        mask = USHRT_MAX;
        break;
    case IDC_RADIO_BYTE:
        mask = UCHAR_MAX;
        break;
    default:
        mask = (unsigned __int64)-1;
    }
    r->i &= mask;
}

double asinh(double x)
{
    return log(x+sqrt(x*x+1));
}

double acosh(double x)
{
    // must be x>=1, if not return Nan (Not a Number)
    if(!(x>=1.0)) return sqrt(-1.0);

    // return only the positive result (as sqrt does).
    return log(x+sqrt(x*x-1.0));
}

double atanh(double x)
{
    // must be x>-1, x<1, if not return Nan (Not a Number)
    if(!(x>-1.0 && x<1.0)) return sqrt(-1.0);

    return log((1.0+x)/(1.0-x))/2.0;
}

static double validate_rad2angle(double a)
{
    switch (calc.degr) {
    case IDC_RADIO_DEG:
        a = a * (180.0/CALC_PI);
        break;
    case IDC_RADIO_RAD:
        break;
    case IDC_RADIO_GRAD:
        a = a * (200.0/CALC_PI);
        break;
    }
    return a;
}

static double validate_angle2rad(calc_number_t *c)
{
    switch (calc.degr) {
    case IDC_RADIO_DEG:
        c->f = c->f * (CALC_PI/180.0);
        break;
    case IDC_RADIO_RAD:
        break;
    case IDC_RADIO_GRAD:
        c->f = c->f * (CALC_PI/200.0);
        break;
    }
    return c->f;
}

void rpn_sin(calc_number_t *c)
{
    double angle = validate_angle2rad(c);

    if (angle == 0 || angle == CALC_PI)
        c->f = 0;
    else
    if (angle == CALC_3_PI_2)
        c->f = -1;
    else
    if (angle == CALC_2_PI)
        c->f = 1;
    else
        c->f = sin(angle);
}
void rpn_cos(calc_number_t *c)
{
    double angle = validate_angle2rad(c);

    if (angle == CALC_PI_2 || angle == CALC_3_PI_2)
        c->f = 0;
    else
    if (angle == CALC_PI)
        c->f = -1;
    else
    if (angle == CALC_2_PI)
        c->f = 1;
    else
        c->f = cos(angle);
}
void rpn_tan(calc_number_t *c)
{
    double angle = validate_angle2rad(c);

    if (angle == CALC_PI_2 || angle == CALC_3_PI_2)
        calc.is_nan = TRUE;
    else
    if (angle == CALC_PI || angle == CALC_2_PI)
        c->f = 0;
    else
        c->f = tan(angle);
}

void rpn_asin(calc_number_t *c)
{
    c->f = validate_rad2angle(asin(c->f));
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_acos(calc_number_t *c)
{
    c->f = validate_rad2angle(acos(c->f));
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_atan(calc_number_t *c)
{
    c->f = validate_rad2angle(atan(c->f));
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}

void rpn_sinh(calc_number_t *c)
{
    c->f = sinh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_cosh(calc_number_t *c)
{
    c->f = cosh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_tanh(calc_number_t *c)
{
    c->f = tanh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}

void rpn_asinh(calc_number_t *c)
{
    c->f = asinh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_acosh(calc_number_t *c)
{
    c->f = acosh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}
void rpn_atanh(calc_number_t *c)
{
    c->f = atanh(c->f);
    if (_isnan(c->f))
        calc.is_nan = TRUE;
}

void rpn_int(calc_number_t *c)
{
    double int_part;

    modf(calc.code.f, &int_part);
    c->f = int_part;
}

void rpn_frac(calc_number_t *c)
{
    double int_part;

    c->f = modf(calc.code.f, &int_part);
}

void rpn_reci(calc_number_t *c)
{
    if (c->f == 0)
        calc.is_nan = TRUE;
    else
        c->f = 1./c->f;
}

void rpn_fact(calc_number_t *c)
{
    double fact, mult, num;

    if (calc.base == IDC_RADIO_DEC)
        num = c->f;
    else
        num = (double)c->i;
    if (num > 1000) {
        calc.is_nan = TRUE;
        return;
    }
    if (num < 0) {
        calc.is_nan = TRUE;
        return;
    } else
    if (num == 0)
        fact = 1;
    else {
        rpn_int(c);
        fact = 1;
        mult = 2;
        while (mult <= num) {
            fact *= mult;
            mult++;
        }
        c->f = fact;
    }
    if (_finite(fact) == 0)
        calc.is_nan = TRUE;
    else
    if (calc.base == IDC_RADIO_DEC)
        c->f = fact;
    else
        c->i = (__int64)fact;
}

__int64 logic_dbl2int(calc_number_t *a)
{
    double   int_part;
    int      width;

    modf(a->f, &int_part);
    width = (int_part==0) ? 1 : (int)log10(fabs(int_part))+1;
    if (width > 63) {
        calc.is_nan = TRUE;
        return 0;
    }
    return (__int64)int_part;
}

double logic_int2dbl(calc_number_t *a)
{
    return (double)a->i;
}

void rpn_not(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC) {
        calc_number_t n;
        n.i = logic_dbl2int(c);
        c->f = (long double)(~n.i);
    } else
        c->i = ~c->i;
}

void rpn_pi(calc_number_t *c)
{
    c->f = CALC_PI;
}

void rpn_2pi(calc_number_t *c)
{
    c->f = CALC_PI*2;
}

void rpn_sign(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC)
        c->f = 0-c->f;
    else
        c->i = 0-c->i;
}

void rpn_exp2(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC) {
        c->f *= c->f;
        if (_finite(c->f) == 0)
            calc.is_nan = TRUE;
    } else
        c->i *= c->i;
}

void rpn_exp3(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC) {
        c->f = pow(c->f, 3.);
        if (_finite(c->f) == 0)
            calc.is_nan = TRUE;
    } else
        c->i *= (c->i*c->i);
}

static __int64 myabs64(__int64 number)
{
    return (number < 0) ? 0-number : number;
}

static unsigned __int64 sqrti(unsigned __int64 number)
{
/* modified form of Newton's method for approximating roots */
#define NEXT(n, i)  (((n) + (i)/(n)) >> 1)
    unsigned __int64 n, n1;

#ifdef __GNUC__
    if (number == 0xffffffffffffffffULL)
#else
    if (number == 0xffffffffffffffffUI64)
#endif
        return 0xffffffff;

    n  = 1;
    n1 = NEXT(n, number);
    while (myabs64(n1 - n) > 1) {
        n  = n1;
        n1 = NEXT(n, number);
    }
    while((n1*n1) > number)
        n1--;
    return n1;
#undef NEXT
}

void rpn_sqrt(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC) {
        if (c->f < 0)
            calc.is_nan = TRUE;
        else
            c->f = sqrt(c->f);
    } else {
        c->i = sqrti(c->i);
    }
}

static __int64 cbrti(__int64 x) {
   __int64 s, y, b;

   s = 60;
   y = 0;
   while(s >= 0) {
      y = 2*y;
      b = (3*y*(y + 1) + 1) << s;
      s = s - 3;
      if (x >= b) {
         x = x - b;
         y = y + 1;
      }
   }
   return y;
}

void rpn_cbrt(calc_number_t *c)
{
    if (calc.base == IDC_RADIO_DEC)
#if defined(__GNUC__) && !defined(__REACTOS__)
        c->f = cbrt(c->f);
#else
        c->f = pow(c->f,1./3.);
#endif
    else {
        c->i = cbrti(c->i);
    }
}

void rpn_exp(calc_number_t *c)
{
    c->f = exp(c->f);
    if (_finite(c->f) == 0)
        calc.is_nan = TRUE;
}

void rpn_exp10(calc_number_t *c)
{
    double int_part;

    modf(c->f, &int_part);
    if (fmod(int_part, 2.) == 0.)
        calc.is_nan = TRUE;
    else {
        c->f = pow(10., c->f);
        if (_finite(c->f) == 0)
            calc.is_nan = TRUE;
    }
}

void rpn_ln(calc_number_t *c)
{
    if (c->f <= 0)
        calc.is_nan = TRUE;
    else
        c->f = log(c->f);
}

void rpn_log(calc_number_t *c)
{
    if (c->f <= 0)
        calc.is_nan = TRUE;
    else
        c->f = log10(c->f);
}

static double stat_sum(void)
{
    double       sum = 0;
    statistic_t *p = calc.stat;

    while (p != NULL) {
        if (p->base == IDC_RADIO_DEC)
            sum += p->num.f;
        else
            sum += p->num.i;
        p = (statistic_t *)(p->next);
    }
    return sum;
}

static double stat_sum2(void)
{
    double       sum = 0;
    statistic_t *p = calc.stat;

    while (p != NULL) {
        if (p->base == IDC_RADIO_DEC)
            sum += p->num.f * p->num.f;
        else
            sum += (double)p->num.i * (double)p->num.i;
        p = (statistic_t *)(p->next);
    }
    return sum;
}

void rpn_ave(calc_number_t *c)
{
    double       ave = 0;
    int          n;

    ave = stat_sum();
    n = SendDlgItemMessage(calc.hStatWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);

    if (n)
        ave = ave / (double)n;
    if (calc.base == IDC_RADIO_DEC)
        c->f = ave;
    else
        c->i = (__int64)ave;
}

void rpn_ave2(calc_number_t *c)
{
    double       ave = 0;
    int          n;

    ave = stat_sum2();
    n = SendDlgItemMessage(calc.hStatWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);

    if (n)
        ave = ave / (double)n;
    if (calc.base == IDC_RADIO_DEC)
        c->f = ave;
    else
        c->i = (__int64)ave;
}

void rpn_sum(calc_number_t *c)
{
    double sum = stat_sum();

    if (calc.base == IDC_RADIO_DEC)
        c->f = sum;
    else
        c->i = (__int64)sum;
}

void rpn_sum2(calc_number_t *c)
{
    double sum = stat_sum2();

    if (calc.base == IDC_RADIO_DEC)
        c->f = sum;
    else
        c->i = (__int64)sum;
}

static void rpn_s_ex(calc_number_t *c, int pop_type)
{
    double       ave = 0;
    double       n = 0;
    double       dev = 0;
    double       num = 0;
    statistic_t *p = calc.stat;

    ave = stat_sum();
    n = (double)SendDlgItemMessage(calc.hStatWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);

    if (n == 0) {
        c->f = 0;
        return;
    }
    ave = ave / n;

    dev = 0;
    p = calc.stat;
    while (p != NULL) {
        if (p->base == IDC_RADIO_DEC)
            num = p->num.f;
        else
            num = (double)p->num.i;
        dev += pow(num-ave, 2.);
        p = (statistic_t *)(p->next);
    }
    dev = sqrt(dev/(pop_type ? n-1 : n));
    if (calc.base == IDC_RADIO_DEC)
        c->f = dev;
    else
        c->i = (__int64)dev;
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
    double d, m, s;

    m = modf(c->f, &d) * 100;
    s = (modf(m, &m) * 100)+.5;
    modf(s, &s);

    m = m/60;
    s = s/3600;

    c->f = d + m + s;
}

void rpn_dec2dms(calc_number_t *c)
{
    double d, m, s;

    m = modf(c->f, &d) * 60;
    s = ceil(modf(m, &m) * 60);
    c->f = d + m/100. + s/10000.;
}

void rpn_zero(calc_number_t *c)
{
    c->f = 0;
}

void rpn_copy(calc_number_t *dst, calc_number_t *src)
{
    *dst = *src;
}

int rpn_is_zero(calc_number_t *c)
{
    return (c->f == 0);
}

void rpn_alloc(calc_number_t *c)
{
}

void rpn_free(calc_number_t *c)
{
}
