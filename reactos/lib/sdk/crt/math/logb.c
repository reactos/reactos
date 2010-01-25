/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * Public domain.
 */

#include <math.h>

double
logb (double x)
{
  double res = 0.0;
#ifdef __GNUC__
  asm ("fxtract\n\t"
       "fstp	%%st" : "=t" (res) : "0" (x));
#endif
  return res;
}
