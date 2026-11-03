//
// libm.h
//
//      Partly imported from musl libc
//
// Support header for musl math code.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <stdint.h>
#include <float.h>
#include <math.h>

// Define _STATIC_ASSERT for compatibility with legacy CRT headers
#ifndef _STATIC_ASSERT
#define _STATIC_ASSERT(expr) extern char (*__static_assert__(void)) [(expr) ? 1 : -1]
#endif

// Make sure the sizes are correct
_STATIC_ASSERT(sizeof(float) == 4);
_STATIC_ASSERT(sizeof(double) == 8);
_STATIC_ASSERT(sizeof(long double) == 8);

_Check_return_ int __cdecl _isnanf(_In_ float _X);
#define isnan _isnan
#define isnanf _isnanf

// musl has this in math.h
static __inline uint32_t __FLOAT_BITS(float __f)
{
	union {float __f; uint32_t __i;} __u;
	__u.__f = __f;
	return __u.__i;
}

static __inline uint64_t __DOUBLE_BITS(double __f)
{
	union {double __f; uint64_t __i;} __u;
	__u.__f = __f;
	return __u.__i;
}

#define signbit(x) \
	((sizeof(x) == sizeof(float)) ? (int)(__FLOAT_BITS(x)>>31) : \
	                                (int)(__DOUBLE_BITS(x)>>63))

static inline void fp_force_evalf(float x)
{
	volatile float y;
	y = x;
    (void)y;
}

static inline void fp_force_eval(double x)
{
	volatile double y;
	y = x;
    (void)y;
}

#define FORCE_EVAL(x) do {                    \
	if (sizeof(x) == sizeof(float)) {         \
		fp_force_evalf(x);                    \
	} else {                                  \
		fp_force_eval(x);                     \
	}                                         \
} while(0)

// musl visibility attribute, everything is in one library here
#define hidden

/* Support non-nearest rounding mode.  */
#define WANT_ROUNDING 1
/* Support signaling NaNs.  */
#define WANT_SNAN 0

#define issignalingf_inline(x) 0
#define issignaling_inline(x) 0

#ifndef TOINT_INTRINSICS
#define TOINT_INTRINSICS 0
#endif

/* Helps static branch prediction so hot path can be better optimized.  */
#ifdef __GNUC__
#define predict_true(x) __builtin_expect(!!(x), 1)
#define predict_false(x) __builtin_expect(x, 0)
#else
#define predict_true(x) (x)
#define predict_false(x) (x)
#endif

static inline float eval_as_float(float x)
{
	float y = x;
	return y;
}

static inline double eval_as_double(double x)
{
	double y = x;
	return y;
}

static inline float fp_barrierf(float x)
{
	volatile float y = x;
	return y;
}

static inline double fp_barrier(double x)
{
	volatile double y = x;
	return y;
}

// Functions instead of musl's compound literals, which MSVC warns about
static inline uint32_t asuint(float f)
{
	union {float _f; uint32_t _i;} u;
	u._f = f;
	return u._i;
}

static inline float asfloat(uint32_t i)
{
	union {uint32_t _i; float _f;} u;
	u._i = i;
	return u._f;
}

static inline uint64_t asuint64(double f)
{
	union {double _f; uint64_t _i;} u;
	u._f = f;
	return u._i;
}

static inline double asdouble(uint64_t i)
{
	union {uint64_t _i; double _f;} u;
	u._i = i;
	return u._f;
}

#define EXTRACT_WORDS(hi,lo,d)                    \
do {                                              \
  uint64_t __u = asuint64(d);                     \
  (hi) = __u >> 32;                               \
  (lo) = (uint32_t)__u;                           \
} while (0)

#define GET_HIGH_WORD(hi,d)                       \
do {                                              \
  (hi) = asuint64(d) >> 32;                       \
} while (0)

#define GET_LOW_WORD(lo,d)                        \
do {                                              \
  (lo) = (uint32_t)asuint64(d);                   \
} while (0)

#define INSERT_WORDS(d,hi,lo)                     \
do {                                              \
  (d) = asdouble(((uint64_t)(hi)<<32) | (uint32_t)(lo)); \
} while (0)

#define SET_HIGH_WORD(d,hi)                       \
  INSERT_WORDS(d, hi, (uint32_t)asuint64(d))

#define SET_LOW_WORD(d,lo)                        \
  INSERT_WORDS(d, asuint64(d)>>32, lo)

#define GET_FLOAT_WORD(w,d)                       \
do {                                              \
  (w) = asuint(d);                                \
} while (0)

#define SET_FLOAT_WORD(d,w)                       \
do {                                              \
  (d) = asfloat(w);                               \
} while (0)

#ifndef FP_ILOGB0
#define FP_ILOGB0 (-0x7fffffff - 1)
#endif
#ifndef FP_ILOGBNAN
#define FP_ILOGBNAN 0x7fffffff
#endif

// math.h keeps these from CRT builds
_Check_return_ int __cdecl _finite(_In_ double _X);
#ifndef isfinite
#define isfinite(x) (_finite((double)(x)) != 0)
#endif

int ilogb(double);
double logb(double);
double scalbn(double, int);

hidden int    __rem_pio2_large(double*,double*,int,int,int);
hidden int    __rem_pio2(double,double*);
hidden double __tan(double,double,int);

/* error handling functions */
hidden double __math_xflow(uint32_t, double);
hidden double __math_uflow(uint32_t);
hidden double __math_oflow(uint32_t);
hidden double __math_divzero(uint32_t);
hidden double __math_invalid(double);
