/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
#include <libc/unconst.h>

void *
memchr(const void *s, int c, size_t n)
{
  if (n)
  {
    const char *p = s;
    do {
      if (*p++ == c)
	return unconst(p-1, void *);
    } while (--n != 0);
  }
  return 0;
}
