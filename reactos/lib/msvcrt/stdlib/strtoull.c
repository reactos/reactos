/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <limits.h>
#include <crtdll/ctype.h>
#include <crtdll/errno.h>
#include <crtdll/stdlib.h>
//#include <crtdll/unconst.h>

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
strtoull(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long)ULONG_MAX / base;
  cutlim = (unsigned long)ULONG_MAX % base;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
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
    errno = ERANGE;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? (char *)s - 1 : (char *)nptr;
  return acc;
}
