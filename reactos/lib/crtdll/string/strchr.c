/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
#include <libc/unconst.h>

char *
strchr(const char *s, int c)
{
  char cc = c;
  while (*s)
  {
    if (*s == cc)
      return unconst(s, char *);
    s++;
  }
  if (cc == 0)
    return unconst(s, char *);
  return 0;
}

