/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <math.h>

double
ldexp(double v, int e)
{
  double two = 2.0;

  if (e < 0)
  {
    e = -e; /* This just might overflow on two-complement machines.  */
    if (e < 0) return 0.0;
    while (e > 0)
    {
      if (e & 1) v /= two;
      two *= two;
      e >>= 1;
    }
  }
  else if (e > 0)
  {
    while (e > 0)
    {
      if (e & 1) v *= two;
      two *= two;
      e >>= 1;
    }
  }
  return v;
}

