/*
 * $Id: wcscpy.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

wchar_t* wcscpy(wchar_t *to, const wchar_t *from)
{
  wchar_t *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
}
