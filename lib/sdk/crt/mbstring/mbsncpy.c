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

#include <precomp.h>
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

/*
 * Unlike _mbsnbcpy this function does not pad the rest of the dest
 * string with 0
*/
int CDECL _mbsnbcpy_s(unsigned char* dst, size_t size, const unsigned char* src, size_t n)
{
    size_t pos = 0;

    if(!dst || size == 0)
        return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }
    if(!n)
        return 0;

    if(g_mbcp_is_multibyte)
    {
        int is_lead = 0;
        while (*src && n)
        {
            if(pos == size)
            {
                dst[0] = '\0';
                return ERANGE;
            }
            is_lead = (!is_lead && _ismbblead(*src));
            n--;
            dst[pos++] = *src++;
        }

        if (is_lead) /* if string ends with a lead, remove it */
            dst[pos - 1] = 0;
    }
    else
    {
        while (n)
        {
            n--;
            if(pos == size)
            {
                dst[0] = '\0';
                return ERANGE;
            }

            if(!(*src)) break;
            dst[pos++] = *src++;
        }
    }

    if(pos < size)
        dst[pos] = '\0';
    else
    {
        dst[0] = '\0';
        return ERANGE;
    }

    return 0;
}
