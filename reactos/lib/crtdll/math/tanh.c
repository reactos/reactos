/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/math.h>

double tanh(double x)
{
  if (x > 50)
    return 1;
  else if (x < -50)
    return -1;
  else
  {
    const double ebig = exp(x);
    const double esmall = 1.0/ebig;
    return (ebig - esmall) / (ebig + esmall);
  }
}

