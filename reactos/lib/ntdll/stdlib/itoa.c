/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/ntdll/stdlib/itoa.c
 * PURPOSE:     converts an integer to ascii
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa Boudewijn Dekker
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>


char *
_i64toa(__int64 value, char *string, int radix)
{
  char tmp[65];
  char *tp = tmp;
  __int64 i;
  unsigned __int64 v;
  __int64 sign;
  char *sp;

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
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


char *
_itoa(int value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  int i;
  unsigned v;
  int sign;
  char *sp;

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
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


char *
_ltoa(long value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  long i;
  unsigned long v;
  int sign;
  char *sp;

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
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;
  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}


char *
_ultoa(unsigned long value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  long i;
  unsigned long v = value;
  char *sp;

  if (radix > 36 || radix <= 1)
  {
    return 0;
  }

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}