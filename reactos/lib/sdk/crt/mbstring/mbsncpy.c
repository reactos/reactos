/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsncpy.c
 * PURPOSE:     Copies a string to a maximum of n bytes or characters
 * PROGRAMERS:
 *              Copyright 1999 Ariadne
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <mbstring.h>

extern int g_mbcp_is_multibyte;

/*
 * @implemented
 */
unsigned char* _mbsncpy(unsigned char *dst, const unsigned char *src, size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if (g_mbcp_is_multibyte)
  {
    while (*src && n)
    {
      n--;
      if (_ismbblead(*src))
      {
        if (!*(src+1))
        {
            *dst++ = 0;
            *dst++ = 0;
            break;
        }

        *dst++ = *src++;
      }

      *dst++ = *src++;
    }
  }
  else
  {
    while (n)
    {
        n--;
        if (!(*dst++ = *src++)) break;
    }
  }
  while (n--) *dst++ = 0;
  return ret;
}


/*
 * The _mbsnbcpy function copies count bytes from src to dest. If src is shorter
 * than dest, the string is padded with null characters. If dest is less than or
 * equal to count it is not terminated with a null character.
 *
 * @implemented
 */
unsigned char * _mbsnbcpy(unsigned char *dst, const unsigned char *src, size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if(g_mbcp_is_multibyte)
  {
    int is_lead = 0;
    while (*src && n)
    {
      is_lead = (!is_lead && _ismbblead(*src));
      n--;
      *dst++ = *src++;
    }

    if (is_lead) /* if string ends with a lead, remove it */
      *(dst - 1) = 0;
  }
  else
  {
    while (n)
    {
        n--;
        if (!(*dst++ = *src++)) break;
    }
  }
  while (n--) *dst++ = 0;
  return ret;
}
