/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
#include <ctype.h>

/*
 * @implemented
 */
int _strnicmp(const char *s1, const char *s2, size_t n)
{

  if (n == 0)
    return 0;
  do {
    if (toupper(*s1) != toupper(*s2++))
      return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)--s2);
    if (*s1++ == 0)
      break;
  } while (--n != 0);
  return 0;
}
