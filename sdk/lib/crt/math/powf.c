/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#if defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_AMD64))
#pragma warning(suppress:4164) /* intrinsic not declared */
#pragma function(powf)
#endif /* _MSC_VER */

double __cdecl pow(double x, double y);

float powf(float x, float y)
{
    return (float)pow((double)x, (double)y);
}
