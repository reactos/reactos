/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismblead.c
 * PURPOSE:      Checks for a leading byte
 * PROGRAMERS:
 *              Copyright 1999 Ariadne, Taiji Yamada
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *              Copyright 2008 Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>
#include <mbctype.h>

/*
 * @implemented
 */
int _ismbblead(unsigned int c)
{
  return (_mbctype[(c&0xff) + 1] & _M1) != 0;
}

/*
 * @implemented
 */
int _ismbslead( const unsigned char *start, const unsigned char *str)
{
  int lead = 0;

  /* Lead bytes can also be trail bytes so we need to analyse the string
   */
  while (start <= str)
  {
    if (!*start)
      return 0;
    lead = !lead && _ismbblead(*start);
    start++;
  }

  return lead ? -1 : 0;
}

/*
 * @implemented
 */
unsigned char *__p__mbctype(void)
{
  return _mbctype;
}


