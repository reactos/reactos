#include <string.h>

/*
 * @implemented
 */
wchar_t *
_i64tow(__int64 value, wchar_t *string, int radix)
{
  wchar_t tmp[65];
  wchar_t *tp = tmp;
  __int64 i;
  unsigned __int64 v;
  __int64 sign;
  wchar_t *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned __int64)value;
  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = L'-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


/*
 * @implemented
 */
wchar_t *
_ui64tow(unsigned __int64 value, wchar_t *string, int radix)
{
  wchar_t tmp[65];
  wchar_t *tp = tmp;
  __int64 i;
  unsigned __int64 v;
  wchar_t *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  v = (unsigned __int64)value;
  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  sp = string;
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


/*
 * @implemented
 */
wchar_t *
_itow(int value, wchar_t *string, int radix)
{
  wchar_t tmp[33];
  wchar_t *tp = tmp;
  int i;
  unsigned v;
  int sign;
  wchar_t *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;
  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = L'-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


/*
 * @implemented
 */
wchar_t *
_ltow(long value, wchar_t *string, int radix)
{
  wchar_t tmp[33];
  wchar_t *tp = tmp;
  long i;
  unsigned long v;
  int sign;
  wchar_t *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned long)value;
  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = L'-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


/*
 * @implemented
 */
wchar_t *
_ultow(unsigned long value, wchar_t *string, int radix)
{
  wchar_t tmp[33];
  wchar_t *tp = tmp;
  long i;
  unsigned long v = value;
  wchar_t *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  sp = string;
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}
