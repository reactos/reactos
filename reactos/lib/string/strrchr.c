/*
 * $Id: strrchr.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char *
strrchr(const char *s, int c)
{
  char cc = c;
  const char *sp=(char *)0;
  while (*s)
  {
    if (*s == cc)
      sp = s;
    s++;
  }
  if (cc == 0)
    sp = s;
  return (char *)sp;
}

