/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <string.h>

size_t
strlen(const char *str)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s; ++s);
  return s-str;
}

