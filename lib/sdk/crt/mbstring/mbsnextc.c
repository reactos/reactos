/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsnextc.c
 * PURPOSE:      Finds the next character in a string
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
unsigned int _mbsnextc (const unsigned char *str)
{
  if(_ismbblead(*str))
    return *str << 8 | str[1];
  return *str;
}
