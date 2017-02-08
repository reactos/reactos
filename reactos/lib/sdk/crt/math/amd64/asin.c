/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS CRT
 * FILE:             lib/sdk/crt/math/amd64/asin.c
 * PURPOSE:          Generic C implementation of arc sine
 * PROGRAMMER:       Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define PRECISION 9

/*
 * The arc sine can be approximated with the following row:
 *
 *   asin(x) = a0*x + a1*x^3 + a2*x^5 + a3*x^7 + a4*x^9 + ...
 *
 * To reduce the number of multiplications the formula is transformed to
 *
 *   asin(x) = x * (1 + x^2*(a1 + x^2*(a2 + x^2*(a3 + ...) ) ) )
 *
 * The coefficients are:
 *   a0 = 1
 *   a1 = (1/2*3)
 *   a2 = (3*1/4*2*5)
 *   a3 = (5*3*1/6*4*2*7)
 *   a4 = (7*5*3*1/8*6*4*2*9)
 *   a5 = (9*7*5*3*1/10*8*6*4*2*11)
 *   ...
 */

double
asin(double x)
{
    double x2, result;

    /* Check range */
    if ((x > 1.) || (x < -1.)) return NaN;

    /* Calculate the square of x */
    x2 = (x * x);

    /* Start with 0, compiler will optimize this away */
    result = 0;

    result += (15*13*11*9*7*5*3*1./(16*14*12*10*8*6*4*2*17));
    result *= x2;

    result += (13*11*9*7*5*3*1./(14*12*10*8*6*4*2*15));
    result *= x2;

    result += (11*9*7*5*3*1./(12*10*8*6*4*2*13));
    result *= x2;

    result += (9*7*5*3*1./(10*8*6*4*2*11));
    result *= x2;

    result += (7*5*3*1./(8*6*4*2*9));
    result *= x2;

    result += (5*3*1./(6*4*2*7));
    result *= x2;

    result += (3*1./(4*2*5));
    result *= x2;

    result += (1./(2*3));
    result *= x2;

    result += 1.;
    result *= x;

    return result;
}

