#include <msvcrt/math.h>

#pragma function(fmod,sqrt)
#pragma function(log,log10,pow,exp)
#pragma function(tan,atan,atan2,tanh)
#pragma function(cos,acos,cosh)
#pragma function(sin,asin,sinh)


double linkme_ceil(double __x)
{
    return ceil(__x);
}

double linkme_fabs(double __x)
{
    return fabs(__x);
}

double linkme_floor(double __x)
{
    return floor(__x);
}

double linkme_ldexp(double __x, int __y)
{
    return ldexp(__x, __y);
}

double linkme_log2(double __x)
{
    //return log2(__x);
    return 0;
}

double linkme_fmod(double __x, double __y)
{
    return fmod(__x, __y);
}

double linkme_sqrt(double __x)
{
    return sqrt(__x);
}

double linkme_log(double __x)
{
    return log(__x);
}

double linkme_log10(double __x)
{
    return log10(__x);
}

double linkme_pow(double __x, double __y)
{
    return pow(__x, __y);
}

double linkme_exp(double __x)
{
    return exp(__x);
}

double linkme_tan(double __x)
{
    return tan(__x);
}

double linkme_atan(double __x)
{
    return atan(__x);
}

double linkme_atan2(double __x, double __y)
{
    return atan2(__x, __y);
}

double linkme_tanh(double __x)
{
    return tanh(__x);
}

double linkme_cos(double __x)
{
    return cos(__x);
}

double linkme_acos(double __x)
{
    return acos(__x);
}

double linkme_cosh(double __x)
{
    return cosh(__x);
}

double linkme_sin(double __x)
{
    return sin(__x);
}

double linkme_asin(double __x)
{
    return asin(__x);
}

double linkme_sinh(double __x)
{
    return sinh(__x);
}
/*
linkme_log2
linkme_floor
_linkme_ldexp
_linkme_pow
 */
