/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>


/*
 * @implemented
 */
double wcstod(const wchar_t *s, wchar_t **sret)
{
  long double r;		/* result */
  int e;			/* exponent */
  long double d;		/* scale */
  int sign;			/* +- 1.0 */
  int esign;
  int i;
  int flags=0;

  r = 0.0;
  sign = 1;
  e = 0;
  esign = 1;

  while ((*s == L' ') || (*s == L'\t'))
    s++;

  if (*s == L'+')
    s++;
  else if (*s == L'-')
  {
    sign = -1;
    s++;
  }

  while ((*s >= L'0') && (*s <= L'9'))
  {
    flags |= 1;
    r *= 10.0;
    r += *s - L'0';
    s++;
  }

  if (*s == L'.')
  {
    d = 0.1L;
    s++;
    while ((*s >= L'0') && (*s <= L'9'))
    {
      flags |= 2;
      r += d * (*s - L'0');
      s++;
      d *= 0.1L;
    }
  }

  if (flags == 0)
  {
    if (sret)
      *sret = (wchar_t *)s;
    return 0;
  }

  if ((*s == L'e') || (*s == L'E'))
  {
    s++;
    if (*s == L'+')
      s++;
    else if (*s == L'-')
    {
      s++;
      esign = -1;
    }
    if ((*s < L'0') || (*s > L'9'))
    {
      if (sret)
	*sret = (wchar_t *)s;
      return r;
    }

    while ((*s >= L'0') && (*s <= L'9'))
    {
      e *= 10;
      e += *s - L'0';
      s++;
    }
  }

  if (esign < 0)
    for (i = 1; i <= e; i++)
      r *= 0.1L;
  else
    for (i = 1; i <= e; i++)
      r *= 10.0;

  if (sret)
    *sret = (wchar_t *)s;
  return r * sign;
}
