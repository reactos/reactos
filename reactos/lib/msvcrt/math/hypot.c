/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/*
 * hypot() function for DJGPP.
 *
 * hypot() computes sqrt(x^2 + y^2).  The problem with the obvious
 * naive implementation is that it might fail for very large or
 * very small arguments.  For instance, for large x or y the result
 * might overflow even if the value of the function should not,
 * because squaring a large number might trigger an overflow.  For
 * very small numbers, their square might underflow and will be
 * silently replaced by zero; this won't cause an exception, but might
 * have an adverse effect on the accuracy of the result.
 *
 * This implementation tries to avoid the above pitfals, without
 * inflicting too much of a performance hit.
 *
 */
#include <msvcrti.h>

 
/* Approximate square roots of DBL_MAX and DBL_MIN.  Numbers
   between these two shouldn't neither overflow nor underflow
   when squared.  */
#define __SQRT_DBL_MAX 1.3e+154
#define __SQRT_DBL_MIN 2.3e-162
 
double
_hypot(double x, double y)
{
  double abig = fabs(x), asmall = fabs(y);
  double ratio;
 
  /* Make abig = max(|x|, |y|), asmall = min(|x|, |y|).  */
  if (abig < asmall)
    {
      double temp = abig;
 
      abig = asmall;
      asmall = temp;
    }
 
  /* Trivial case.  */
  if (asmall == 0.)
    return abig;
 
  /* Scale the numbers as much as possible by using its ratio.
     For example, if both ABIG and ASMALL are VERY small, then
     X^2 + Y^2 might be VERY inaccurate due to loss of
     significant digits.  Dividing ASMALL by ABIG scales them
     to a certain degree, so that accuracy is better.  */
 
  if ((ratio = asmall / abig) > __SQRT_DBL_MIN && abig < __SQRT_DBL_MAX)
    return abig * sqrt(1.0 + ratio*ratio);
  else
    {
      /* Slower but safer algorithm due to Moler and Morrison.  Never
         produces any intermediate result greater than roughly the
         larger of X and Y.  Should converge to machine-precision
         accuracy in 3 iterations.  */
 
      double r = ratio*ratio, t, s, p = abig, q = asmall;
 
      do {
        t = 4. + r;
        if (t == 4.)
          break;
        s = r / t;
        p += 2. * s * p;
        q *= s;
        r = (q / p) * (q / p);
      } while (1);
 
      return p;
    }
}
 
#ifdef  TEST
 
#include <crtdll/stdio.h>
 
int
main(void)
{
  printf("hypot(3, 4) =\t\t\t %25.17e\n", _hypot(3., 4.));
  printf("hypot(3*10^150, 4*10^150) =\t %25.17g\n", _hypot(3.e+150, 4.e+150));
  printf("hypot(3*10^306, 4*10^306) =\t %25.17g\n", _hypot(3.e+306, 4.e+306));
  printf("hypot(3*10^-320, 4*10^-320) =\t %25.17g\n",_hypot(3.e-320, 4.e-320));
  printf("hypot(0.7*DBL_MAX, 0.7*DBL_MAX) =%25.17g\n",_hypot(0.7*DBL_MAX, 0.7*DBL_MAX));
  printf("hypot(DBL_MAX, 1.0) =\t\t %25.17g\n", _hypot(DBL_MAX, 1.0));
  printf("hypot(1.0, DBL_MAX) =\t\t %25.17g\n", _hypot(1.0, DBL_MAX));
  printf("hypot(0.0, DBL_MAX) =\t\t %25.17g\n", _hypot(0.0, DBL_MAX));
 
  return 0;
}
 
#endif
