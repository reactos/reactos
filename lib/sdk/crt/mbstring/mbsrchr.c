/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsrchr.c
 * PURPOSE:     Searches for a character in reverse
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <stdlib.h>
#include <mbstring.h>

/*
 * @implemented
 */
unsigned char * _mbsrchr(const unsigned char *src, unsigned int val)
{
  unsigned int c;
  unsigned char *match = NULL;

  if (!src)
    return NULL;

  while (1)
  {
    c = _mbsnextc(src);
    if (c == val)
      match = (unsigned char*)src;
    if (!c)
      return match;
    src += (c > 255) ? 2 : 1;
  }
}
