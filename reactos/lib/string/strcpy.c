/*
 * $Id: strcpy.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char* strcpy(char *to, const char *from)
{
  char *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
}
