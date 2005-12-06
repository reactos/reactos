/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/math.h>


double atanh(double x)
{
  return log((1+x)/(1-x)) / 2.0;
}
