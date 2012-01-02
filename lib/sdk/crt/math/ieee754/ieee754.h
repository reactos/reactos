#pragma once

typedef __int32 int32_t;
typedef unsigned __int32 u_int32_t;

typedef union
{
  double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
  } parts;
} ieee_double_shape_type;

#define EXTRACT_WORDS(ix0,ix1,d)	\
do {								\
  ieee_double_shape_type ew_u;		\
  ew_u.value = (d);					\
  (ix0) = ew_u.parts.msw;			\
  (ix1) = ew_u.parts.lsw;			\
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)			\
do {								\
  ieee_double_shape_type gh_u;		\
  gh_u.value = (d);				    \
  (i) = gh_u.parts.msw;				\
} while (0)

#define GET_LOW_WORD(i,d)			\
do {								\
  ieee_double_shape_type gl_u;		\
  gl_u.value = (d);					\
  (i) = gl_u.parts.lsw;				\
} while (0)

static __inline double __ieee754_sqrt(double x) {return sqrt(x);}
static __inline double __ieee754_log(double x) {return log(x);}
static __inline double __cos(double x) {return cos(x);}
static __inline void __sincos(double x, double *s, double *c)
{
    *s = sin(x);
    *c = cos(x);
}

double __ieee754_j0(double);
double __ieee754_j1(double);
double __ieee754_jn(int, double);
double __ieee754_y0(double);
double __ieee754_y1(double);
double __ieee754_yn(int, double);
