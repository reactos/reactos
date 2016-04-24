/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbslen.c
 * PURPOSE:      Determines the length of a multi byte string
 * PROGRAMERS:
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <mbstring.h>

/*
 * @implemented
 */
size_t _mbslen(const unsigned char *str)
{
  size_t len = 0;
  while(*str)
  {
    if (_ismbblead(*str))
    {
      str++;
      if (!*str)  /* count only full chars */
        break;
    }
    str++;
    len++;
  }
  return len;
}
