/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/math.h>

double cosh(double x)
{
  const double ebig = exp(fabs(x));
  return (ebig + 1.0/ebig) / 2.0;
}
