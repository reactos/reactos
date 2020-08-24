/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * PURPOSE:     Copies a multi byte character
 * PROGRAMERS:
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <mbstring.h>
#include <string.h>

/*
 * @implemented
 */
void _mbccpy(unsigned char *dst, const unsigned char *src)
{
  *dst = *src;
  if(_ismbblead(*src))
    *++dst = *++src; /* MB char */
}
