/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int wcscmp(const wchar_t* cs,const wchar_t * ct)
{
  while (*cs == *ct)
  {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
  }
  return *cs - *ct;
}
