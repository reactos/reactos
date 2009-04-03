/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsninc.c
 * PURPOSE:     
 * PROGRAMERS:   
  *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <precomp.h>

#include <mbstring.h>

/*
 * @implemented
 */
unsigned char * _mbsninc(const unsigned char *str, size_t n)
{
  if(!str)
    return NULL;

  while (n > 0 && *str)
  {
    if (_ismbblead(*str))
    {
      if (!*(str+1))
         break;
      str++;
    }
    str++;
    n--;
  }

  return (unsigned char*)str;
}
