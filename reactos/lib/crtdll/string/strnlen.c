/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/string.h>

size_t
strnlen(const char *str, size_t count)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s && count; ++s, count--);
  return s-str;
}

