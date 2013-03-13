/*
 * $Id: memccpy.c 30288 2007-11-09 11:22:29Z fireball $
 */

#include <string.h>


void *
_memccpy (void *to, const void *from,int c,size_t count)
{
  char t;
  size_t i;
  char *dst=(char*)to;
  const char *src=(const char*)from;

  for ( i = 0; i < count; i++ )
  {
    dst[i] = t = src[i];
    if ( t == '\0' )
      break;
    if ( t == c )
      return &dst[i+1];
  }
  return NULL; /* didn't copy c */
}
