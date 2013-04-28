#ifndef __CRT_INTERNAL_MATH_H
#define __CRT_INTERNAL_MATH_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

int     _isinf          (double); /* not exported */
int     _isnanl         (long double); /* not exported */
int     _isinfl         (long double); /* not exported */

#if defined(__GNUC__)
#define FPU_DOUBLE(var) double var; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )
#define FPU_DOUBLES(var1,var2) double var1,var2; \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
    __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )
#elif defined(_MSC_VER)
#define FPU_DOUBLE(var) double var; \
    __asm { fstp [var] }; __asm { fwait };
#define FPU_DOUBLES(var1,var2) double var1,var2; \
    __asm { fstp [var1] }; __asm { fwait }; \
    __asm { fstp [var2] }; __asm { fwait };
#endif

#endif
