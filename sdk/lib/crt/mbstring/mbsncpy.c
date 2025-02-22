/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsncpy.c
 * PURPOSE:     Copies a string to a maximum of n bytes or characters
 * PROGRAMERS:
 *              Copyright 1999 Ariadne
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <precomp.h>
#include <mbstring.h>

/*********************************************************************
 *		_mbsncpy(MSVCRT.@)
 * REMARKS
 *  The parameter n is the number or characters to copy, not the size of
 *  the buffer. Use _mbsnbcpy for a function analogical to strncpy
 */
unsigned char* CDECL _mbsncpy(unsigned char* dst, const unsigned char* src, size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if (get_mbcinfo()->ismbcodepage)
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

/*********************************************************************
 *              _mbsnbcpy_s(MSVCRT.@)
 * REMARKS
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

    if(get_mbcinfo()->ismbcodepage)
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

/*********************************************************************
 *              _mbsnbcpy(MSVCRT.@)
 * REMARKS
 *  Like strncpy this function doesn't enforce the string to be
 *  NUL-terminated
 */
unsigned char* CDECL _mbsnbcpy(unsigned char* dst, const unsigned char* src, size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if(get_mbcinfo()->ismbcodepage)
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

