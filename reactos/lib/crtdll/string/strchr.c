/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */



char *strchr(const char *s, int c);

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

