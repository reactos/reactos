/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>


/*
 * Convert a unicode string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
wcstoul(const wchar_t *nptr, wchar_t **endptr, int base)
{
  const wchar_t *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (iswctype(c, _SPACE));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == L'+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == L'0' && (*s == L'x' || *s == L'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == L'0' ? 8 : 10;
  cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
  cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (iswctype(c, _DIGIT))
      c -= L'0';
    else if (iswctype(c, _ALPHA))
      c -= iswctype(c, _UPPER) ? L'A' - 10 : L'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = ULONG_MAX;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? (wchar_t *)s - 1 : (wchar_t *)nptr;
  return acc;
}
