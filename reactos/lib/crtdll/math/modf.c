// from linux libc

#include <crtdll/math.h>

long double modfl(long double x,long double *pint);

/* Slooow version. */

double modf(double x,double *pint)
{
    if (x >= 0)
        *pint = floor(x);
    else
        *pint = ceil(x);
    return x - *pint;
}

long double modfl(long double x,long double *pint)
{
    if (x >= 0)
        *pint = floor(x);
    else
        *pint = ceil(x);
    return x - *pint;
}