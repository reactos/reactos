/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>
#define MAXLOG	7.09782712893383996843E2
#define MINLOG	-7.08396418532264106224E2

static double c0 = 1.44268798828125L;
static double c1 = 7.05260771340735992468e-6L;

static double
__exp(double x)
{
  double res = 0.0L;
#ifdef __GNUC__
  asm ("fldl2e\n\t"             /* 1  log2(e)         */
       "fmul %%st(1),%%st\n\t"  /* 1  x log2(e)       */
       "frndint\n\t"            /* 1  i               */
       "fld %%st(1)\n\t"        /* 2  x               */
       "frndint\n\t"            /* 2  xi              */
       "fld %%st(1)\n\t"        /* 3  i               */
       "fldt %2\n\t"            /* 4  c0              */
       "fld %%st(2)\n\t"        /* 5  xi              */
       "fmul %%st(1),%%st\n\t"  /* 5  c0 xi           */
       "fsubp %%st,%%st(2)\n\t" /* 4  f = c0 xi  - i  */
       "fld %%st(4)\n\t"        /* 5  x               */
       "fsub %%st(3),%%st\n\t"  /* 5  xf = x - xi     */
       "fmulp %%st,%%st(1)\n\t" /* 4  c0 xf           */
       "faddp %%st,%%st(1)\n\t" /* 3  f = f + c0 xf   */
       "fldt %3\n\t"            /* 4                  */
       "fmul %%st(4),%%st\n\t"  /* 4  c1 * x          */
       "faddp %%st,%%st(1)\n\t" /* 3  f = f + c1 * x  */
       "f2xm1\n\t"		/* 3 2^(fract(x * log2(e))) - 1 */
       "fld1\n\t"               /* 4 1.0              */
       "faddp\n\t"		/* 3 2^(fract(x * log2(e))) */
       "fstp	%%st(1)\n\t"    /* 2  */
       "fscale\n\t"	        /* 2 scale factor is st(1); e^x */
       "fstp	%%st(1)\n\t"    /* 1  */
       "fstp	%%st(1)\n\t"    /* 0  */
       : "=t" (res) : "0" (x), "m" (c0), "m" (c1) : "ax", "dx");
#endif
  return res;
}

double exp (double x)
{
  if (x > MAXLOG)
    return INFINITY;
  else if (x < MINLOG)
    return 0.0L;
  else
    return __exp (x);
}
