/*
 * $Id: strchr.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char *strchr(const char *s, int c)
{
  char cc = c;
  while (*s)
  {
    if (*s == cc)
      return (char *)s;
    s++;
  }
  if (cc == 0)
    return (char *)s;
  return 0;
}

