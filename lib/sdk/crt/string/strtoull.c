#include <precomp.h>

unsigned long long
strtoull(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long long acc;
  int c;
  unsigned long long cutoff;
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
  cutoff = (unsigned long long)ULLONG_MAX / (unsigned long long)base;
  cutlim = (unsigned long long)ULLONG_MAX % (unsigned long long)base;
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
    acc = ULLONG_MAX;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? (char *)((size_t)(s - 1)) : (char *)((size_t)nptr);
  return acc;
}
