/*
 * $Id: strnlen.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

int strnlen(const char *str, size_t count)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s && count; ++s, count--);
  return s-str;
}

