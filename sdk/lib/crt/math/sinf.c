/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#define sinf _dummy_sinf
#include <math.h>
#undef sinf

#if defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_AMD64))
#pragma warning(suppress:4164) /* intrinsic not declared */
#pragma function(sinf)
#endif /* _MSC_VER */

float sinf(float x)
{
    return ((float)sin((double)x));
}
