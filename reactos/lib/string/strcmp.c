/*
 * $Id: strcmp.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

int strcmp(const char* s1, const char* s2)
{
  while (*s1 == *s2)
  {
    if (*s1 == 0)
      return 0;
    s1++;
    s2++;
  }
  return *(unsigned const char*)s1 - *(unsigned const char*)(s2);
}
