/*
 * Unicode string manipulation functions
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>

#define WINE_UNICODE_INLINE  /* nothing */
#include "wine/unicode.h"

int strcmpiW( const WCHAR *str1, const WCHAR *str2 )
{
    for (;;)
    {
        int ret = tolowerW(*str1) - tolowerW(*str2);
        if (ret || !*str1) return ret;
        str1++;
        str2++;
    }
}

int strncmpiW( const WCHAR *str1, const WCHAR *str2, int n )
{
    int ret = 0;
    for ( ; n > 0; n--, str1++, str2++)
        if ((ret = tolowerW(*str1) - tolowerW(*str2)) || !*str1) break;
    return ret;
}

int memicmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    int ret = 0;
    for ( ; n > 0; n--, str1++, str2++)
        if ((ret = tolowerW(*str1) - tolowerW(*str2))) break;
    return ret;
}

WCHAR *strstrW( const WCHAR *str, const WCHAR *sub )
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (WCHAR *)str;
        str++;
    }
    return NULL;
}

/* strtolW and strtoulW implementation based on the GNU C library code */
/* Copyright (C) 1991,92,94,95,96,97,98,99,2000,2001 Free Software Foundation, Inc. */

long int strtolW( const WCHAR *nptr, WCHAR **endptr, int base )
{
  int negative;
  register unsigned long int cutoff;
  register unsigned int cutlim;
  register unsigned long int i;
  register const WCHAR *s;
  register WCHAR c;
  const WCHAR *save, *end;
  int overflow;

  if (base < 0 || base == 1 || base > 36) return 0;

  save = s = nptr;

  /* Skip white space.  */
  while (isspaceW (*s))
    ++s;
  if (!*s) goto noconv;

  /* Check for a sign.  */
  negative = 0;
  if (*s == '-')
    {
      negative = 1;
      ++s;
    }
  else if (*s == '+')
    ++s;

  /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
  if (*s == '0')
    {
      if ((base == 0 || base == 16) && toupperW(s[1]) == 'X')
	{
	  s += 2;
	  base = 16;
	}
      else if (base == 0)
	base = 8;
    }
  else if (base == 0)
    base = 10;

  /* Save the pointer so we can check later if anything happened.  */
  save = s;
  end = NULL;

  cutoff = ULONG_MAX / (unsigned long int) base;
  cutlim = ULONG_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  c = *s;
  for (;c != '\0'; c = *++s)
  {
      if (s == end)
          break;
      if (c >= '0' && c <= '9')
          c -= '0';
      else if (isalphaW (c))
          c = toupperW (c) - 'A' + 10;
      else
          break;
      if ((int) c >= base)
          break;
      /* Check for overflow.  */
      if (i > cutoff || (i == cutoff && c > cutlim))
          overflow = 1;
      else
      {
          i *= (unsigned long int) base;
          i += c;
      }
  }

  /* Check if anything actually happened.  */
  if (s == save)
    goto noconv;

  /* Store in ENDPTR the address of one character
     past the last character we converted.  */
  if (endptr != NULL)
    *endptr = (WCHAR *)s;

  /* Check for a value that is within the range of
     `unsigned LONG int', but outside the range of `LONG int'.  */
  if (overflow == 0
      && i > (negative
	      ? -((unsigned long int) (LONG_MIN + 1)) + 1
	      : (unsigned long int) LONG_MAX))
    overflow = 1;

  if (overflow)
    {
      return negative ? LONG_MIN : LONG_MAX;
    }

  /* Return the result of the appropriate sign.  */
  return negative ? -i : i;

noconv:
  /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are not
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
  if (endptr != NULL)
    {
      if (save - nptr >= 2 && toupperW (save[-1]) == 'X'
	  && save[-2] == '0')
	*endptr = (WCHAR *)&save[-1];
      else
	/*  There was no number to convert.  */
	*endptr = (WCHAR *)nptr;
    }

  return 0L;
}


unsigned long int strtoulW( const WCHAR *nptr, WCHAR **endptr, int base )
{
  int negative;
  register unsigned long int cutoff;
  register unsigned int cutlim;
  register unsigned long int i;
  register const WCHAR *s;
  register WCHAR c;
  const WCHAR *save, *end;
  int overflow;

  if (base < 0 || base == 1 || base > 36) return 0;

  save = s = nptr;

  /* Skip white space.  */
  while (isspaceW (*s))
    ++s;
  if (!*s) goto noconv;

  /* Check for a sign.  */
  negative = 0;
  if (*s == '-')
    {
      negative = 1;
      ++s;
    }
  else if (*s == '+')
    ++s;

  /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
  if (*s == '0')
    {
      if ((base == 0 || base == 16) && toupperW(s[1]) == 'X')
	{
	  s += 2;
	  base = 16;
	}
      else if (base == 0)
	base = 8;
    }
  else if (base == 0)
    base = 10;

  /* Save the pointer so we can check later if anything happened.  */
  save = s;
  end = NULL;

  cutoff = ULONG_MAX / (unsigned long int) base;
  cutlim = ULONG_MAX % (unsigned long int) base;

  overflow = 0;
  i = 0;
  c = *s;
  for (;c != '\0'; c = *++s)
  {
      if (s == end)
          break;
      if (c >= '0' && c <= '9')
          c -= '0';
      else if (isalphaW (c))
          c = toupperW (c) - 'A' + 10;
      else
          break;
      if ((int) c >= base)
          break;
      /* Check for overflow.  */
      if (i > cutoff || (i == cutoff && c > cutlim))
          overflow = 1;
      else
      {
          i *= (unsigned long int) base;
          i += c;
      }
  }

  /* Check if anything actually happened.  */
  if (s == save)
    goto noconv;

  /* Store in ENDPTR the address of one character
     past the last character we converted.  */
  if (endptr != NULL)
    *endptr = (WCHAR *)s;

  if (overflow)
    {
      return ULONG_MAX;
    }

  /* Return the result of the appropriate sign.  */
  return negative ? -i : i;

noconv:
  /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are not
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
  if (endptr != NULL)
    {
      if (save - nptr >= 2 && toupperW (save[-1]) == 'X'
	  && save[-2] == '0')
	*endptr = (WCHAR *)&save[-1];
      else
	/*  There was no number to convert.  */
	*endptr = (WCHAR *)nptr;
    }

  return 0L;
}


int vsnprintfW(WCHAR *str, size_t len, const WCHAR *format, va_list valist)
{
    unsigned int written = 0;
    const WCHAR *iter = format;
    char bufa[256], fmtbufa[64], *fmta;

    while (*iter)
    {
        while (*iter && *iter != '%')
        {
            if (written++ >= len)
                return -1;
            *str++ = *iter++;
        }
        if (*iter == '%')
        {
            if (iter[1] == '%')
            {
                if (written++ >= len)
                    return -1;
                *str++ = '%'; /* "%%"->'%' */
                iter += 2;
                continue;
            }

            fmta = fmtbufa;
            *fmta++ = *iter++;
            while (*iter == '0' ||
                   *iter == '+' ||
                   *iter == '-' ||
                   *iter == ' ' ||
                   *iter == '*' ||
                   *iter == '#')
            {
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    int fieldlen = va_arg(valist, int);
                    sprintf(buffiter, "%d", fieldlen);
                    while (*buffiter)
                        *fmta++ = *buffiter++;
                }
                else
                    *fmta++ = *iter;
                iter++;
            }

            while (isdigit(*iter))
                *fmta++ = *iter++;

            if (*iter == '.')
            {
                *fmta++ = *iter++;
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    int fieldlen = va_arg(valist, int);
                    sprintf(buffiter, "%d", fieldlen);
                    while (*buffiter)
                        *fmta++ = *buffiter++;
                }
                else
                    while (isdigit(*iter))
                        *fmta++ = *iter++;
            }
            if (*iter == 'h' || *iter == 'l')
                *fmta++ = *iter++;

            switch (*iter)
            {
            case 's':
            {
                static const WCHAR none[] = { '(','n','u','l','l',')',0 };
                const WCHAR *wstr = va_arg(valist, const WCHAR *);
                const WCHAR *striter = wstr ? wstr : none;
                while (*striter)
                {
                    if (written++ >= len)
                        return -1;
                    *str++ = *striter++;
                }
                iter++;
                break;
            }

            case 'c':
                if (written++ >= len)
                    return -1;
                *str++ = (WCHAR)va_arg(valist, int);
                iter++;
                break;

            default:
            {
                /* For non wc types, use system sprintf and append to wide char output */
                /* FIXME: for unrecognised types, should ignore % when printing */
                char *bufaiter = bufa;
                if (*iter == 'p')
                    sprintf(bufaiter, "%08lX", va_arg(valist, long));
                else
                {
                    *fmta++ = *iter;
                    *fmta = '\0';
                    if (*iter == 'a' || *iter == 'A' ||
                        *iter == 'e' || *iter == 'E' ||
                        *iter == 'f' || *iter == 'F' || 
                        *iter == 'g' || *iter == 'G')
                        sprintf(bufaiter, fmtbufa, va_arg(valist, double));
                    else
                    {
                        /* FIXME: On 32 bit systems this doesn't handle int 64's.
                         *        on 64 bit systems this doesn't work for 32 bit types
			 */
                        sprintf(bufaiter, fmtbufa, va_arg(valist, void *));
                    }
                }
                while (*bufaiter)
                {
                    if (written++ >= len)
                        return -1;
                    *str++ = *bufaiter++;
                }
                iter++;
                break;
            }
            }
        }
    }
    if (written >= len)
        return -1;
    *str++ = 0;
    return (int)written;
}

int vsprintfW( WCHAR *str, const WCHAR *format, va_list valist )
{
    return vsnprintfW( str, INT_MAX, format, valist );
}

int snprintfW( WCHAR *str, size_t len, const WCHAR *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = vsnprintfW(str, len, format, valist);
    va_end(valist);
    return retval;
}

int sprintfW( WCHAR *str, const WCHAR *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = vsnprintfW(str, INT_MAX, format, valist);
    va_end(valist);
    return retval;
}
