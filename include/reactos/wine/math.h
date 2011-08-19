#ifndef __WINE_MATH_H_
#define __WINE_MATH_H_

#include <crt/math.h>

#ifdef _MSC_VER
__forceinline float _NaN()
{
    unsigned long NaN = 0x7fc00000;
    return *(float*)&NaN;
}
#define NAN _NaN()

__forceinline float _Infinity()
{
    unsigned long Infinity = 0x7f800000;
    return *(float*)&Infinity;
}
#define INFINITY _Infinity()

#else
#define NAN (0.0f / 0.0f)
#define INFINITY (1.0F/0.0F)
#endif

#endif /* __WINE_MATH_H_ */
