/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>

/*
 * @implemented
 */
char *
strncat(char *dst, const char *src, size_t n)
{
  if (n != 0)
  {
    char *d = dst;
    const char *s = src;

    while (*d != 0)
      d++;
    do {
      if ((*d = *s++) == 0)
	break;
      d++;
    } while (--n != 0);
    *d = 0;
  }
  return dst;
}
