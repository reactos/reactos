/***********************************************************************************/
/** MIT License **/
/** ----------- **/
/** **/  
/** Copyright (c) 2002-2019 Advanced Micro Devices, Inc. **/
/** **/
/** Permission is hereby granted, free of charge, to any person obtaining a copy **/
/** of this Software and associated documentaon files (the "Software"), to deal **/
/** in the Software without restriction, including without limitation the rights **/
/** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell **/
/** copies of the Software, and to permit persons to whom the Software is **/
/** furnished to do so, subject to the following conditions: **/
/** **/ 
/** The above copyright notice and this permission notice shall be included in **/
/** all copies or substantial portions of the Software. **/
/** **/
/** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR **/
/** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, **/
/** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE **/
/** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER **/
/** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, **/
/** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN **/
/** THE SOFTWARE. **/
/***********************************************************************************/

#ifndef LIBM_UTIL_AMD_H_INCLUDED
#define LIBM_UTIL_AMD_H_INCLUDED 1

#define inline __inline

#include <emmintrin.h>
#include <float.h>


#define MULTIPLIER_SP 24
#define MULTIPLIER_DP 53

static const double VAL_2PMULTIPLIER_DP = 9007199254740992.0;
static const double VAL_2PMMULTIPLIER_DP = 1.1102230246251565404236316680908e-16;
static const float VAL_2PMULTIPLIER_SP = 16777216.0F;
static const float VAL_2PMMULTIPLIER_SP = 5.9604645e-8F;

/* Definitions for double functions on 64 bit machines */
#define SIGNBIT_DP64      0x8000000000000000
#define EXPBITS_DP64      0x7ff0000000000000
#define MANTBITS_DP64     0x000fffffffffffff
#define ONEEXPBITS_DP64   0x3ff0000000000000
#define TWOEXPBITS_DP64   0x4000000000000000
#define HALFEXPBITS_DP64  0x3fe0000000000000
#define IMPBIT_DP64       0x0010000000000000
#define QNANBITPATT_DP64  0x7ff8000000000000
#define INDEFBITPATT_DP64 0xfff8000000000000
#define PINFBITPATT_DP64  0x7ff0000000000000
#define NINFBITPATT_DP64  0xfff0000000000000
#define EXPBIAS_DP64      1023
#define EXPSHIFTBITS_DP64 52
#define BIASEDEMIN_DP64   1
#define EMIN_DP64         -1022
#define BIASEDEMAX_DP64   2046
#define EMAX_DP64         1023
#define LAMBDA_DP64       1.0e300
#define MANTLENGTH_DP64   53
#define BASEDIGITS_DP64   15


/* These definitions, used by float functions,
   are for both 32 and 64 bit machines */
#define SIGNBIT_SP32      0x80000000
#define EXPBITS_SP32      0x7f800000
#define MANTBITS_SP32     0x007fffff
#define ONEEXPBITS_SP32   0x3f800000
#define TWOEXPBITS_SP32   0x40000000
#define HALFEXPBITS_SP32  0x3f000000
#define IMPBIT_SP32       0x00800000
#define QNANBITPATT_SP32  0x7fc00000
#define INDEFBITPATT_SP32 0xffc00000
#define PINFBITPATT_SP32  0x7f800000
#define NINFBITPATT_SP32  0xff800000
#define EXPBIAS_SP32      127
#define EXPSHIFTBITS_SP32 23
#define BIASEDEMIN_SP32   1
#define EMIN_SP32         -126
#define BIASEDEMAX_SP32   254
#define EMAX_SP32         127
#define LAMBDA_SP32       1.0e30
#define MANTLENGTH_SP32   24
#define BASEDIGITS_SP32   7

#define CLASS_SIGNALLING_NAN 1
#define CLASS_QUIET_NAN 2
#define CLASS_NEGATIVE_INFINITY 3
#define CLASS_NEGATIVE_NORMAL_NONZERO 4
#define CLASS_NEGATIVE_DENORMAL 5
#define CLASS_NEGATIVE_ZERO 6
#define CLASS_POSITIVE_ZERO 7
#define CLASS_POSITIVE_DENORMAL 8
#define CLASS_POSITIVE_NORMAL_NONZERO 9
#define CLASS_POSITIVE_INFINITY 10

#define OLD_BITS_SP32(x) (*((unsigned int *)&x))
#define OLD_BITS_DP64(x) (*((unsigned long long *)&x))

/* Alternatives to the above functions which don't have
   problems when using high optimization levels on gcc */
#define GET_BITS_SP32(x, ux) \
  { \
    volatile union {float f; unsigned int i;} _bitsy; \
    _bitsy.f = (x); \
    ux = _bitsy.i; \
  }
#define PUT_BITS_SP32(ux, x) \
  { \
    volatile union {float f; unsigned int i;} _bitsy; \
    _bitsy.i = (ux); \
     x = _bitsy.f; \
  }

#define GET_BITS_DP64(x, ux) \
  { \
    volatile union {double d; unsigned long long i;} _bitsy; \
    _bitsy.d = (x); \
    ux = _bitsy.i; \
  }
#define PUT_BITS_DP64(ux, x) \
  { \
    volatile union {double d; unsigned long long i;} _bitsy; \
    _bitsy.i = (ux); \
    x = _bitsy.d; \
  }


/* Processor-dependent floating-point status flags */
#define AMD_F_OVERFLOW  0x00000001
#define AMD_F_UNDERFLOW 0x00000002
#define AMD_F_DIVBYZERO 0x00000004
#define AMD_F_INVALID   0x00000008
#define AMD_F_INEXACT   0x00000010

/* Processor-dependent floating-point precision-control flags */
#define AMD_F_EXTENDED 0x00000300
#define AMD_F_DOUBLE   0x00000200
#define AMD_F_SINGLE   0x00000000

/* Processor-dependent floating-point rounding-control flags */
#define AMD_F_RC_NEAREST 0x00000000
#define AMD_F_RC_DOWN    0x00002000
#define AMD_F_RC_UP      0x00004000
#define AMD_F_RC_ZERO    0x00006000

#endif /* LIBM_UTIL_AMD_H_INCLUDED */
