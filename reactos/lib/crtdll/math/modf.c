// from linux libc

#include <math.h>

/* Slooow version. */

double modf(double x,double *pint)
{
    if (x >= 0)
        *pint = floor(x);
    else
        *pint = ceil(x);
    return x - *pint;
}