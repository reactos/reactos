/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <math.h>

double
acosh(double x)
{
  return log(x + sqrt(x*x - 1));
}
