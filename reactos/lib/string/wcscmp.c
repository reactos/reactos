/*
 * $Id: wcscmp.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

int wcscmp(const wchar_t* s1, const wchar_t* s2)
{
  while (*s1 == *s2)
  {
    if (*s1 == 0)
      return 0;
    s1++;
    s2++;
  }
  return *s1 - *s2;
}
