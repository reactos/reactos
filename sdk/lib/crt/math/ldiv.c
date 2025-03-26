/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#if (_MSC_VER >= 1920)
#pragma function(ldiv)
#endif

/*
 * @implemented
 */
ldiv_t
ldiv(long num, long denom)
{
  ldiv_t r;

  if (num > 0 && denom < 0)
  {
    num = -num;
    denom = -denom;
  }
  r.quot = num / denom;
  r.rem = num % denom;
  if (num < 0 && denom > 0)
  {
    if (r.rem > 0)
    {
      r.quot++;
      r.rem -= denom;
    }
  }
  return r;
}
