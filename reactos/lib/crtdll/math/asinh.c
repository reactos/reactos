/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <math.h>

double
asinh(double x)
{
  return x>0 ? log(x + sqrt(x*x + 1)) : -log(sqrt(x*x+1)-x);
}

