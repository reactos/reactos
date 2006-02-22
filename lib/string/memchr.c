/*
 * $Id$
 */

#include <string.h>

void* memchr(const void *s, int c, size_t n)
{
  if (n)
  {
    const char *p = s;
    do {
      if (*p++ == c)
	return (void *)(p-1);
    } while (--n != 0);
  }
  return 0;
}
