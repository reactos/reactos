/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>

/*
 * @implemented
 */
size_t strcspn(const char *s1, const char *s2)
{
  const char *p, *spanp;
  char c, sc;

  for (p = s1;;)
  {
    c = *p++;
    spanp = s2;
    do {
      if ((sc = *spanp++) == c)
	return p - 1 - s1;
    } while (sc != 0);
  }
  /* NOTREACHED */
}
