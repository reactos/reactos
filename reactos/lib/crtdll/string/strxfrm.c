/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>

size_t
strxfrm(dst, src, n)
     char *dst;
     const char *src;
     size_t n;
{
  size_t r = 0;
  int c;

  if (n != 0) {
    while ((c = *src++) != 0)
    {
      r++;
      if (--n == 0)
      {
	while (*src++ != 0)
	  r++;
	break;
      }
      *dst++ = c;
    }
    *dst = 0;
  }
  return r;
}
