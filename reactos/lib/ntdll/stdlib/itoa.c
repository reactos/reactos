/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/ntdll/stdlib/itoa.c
 * PURPOSE:     converts an integer to ascii
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa Boudewijn Dekker
 *		2003: Corrected ltoa implementation - Steven Edwards
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
 * Copyright 2000 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
 * Copyright 2003 Thomas Mertes
 */
 
#include <stdlib.h>
#include <string.h>

/*
 * @implemented
 */
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


/*
 * @implemented
 */
char *
_ui64toa(unsigned __int64 value, char *string, int radix)
{
  char tmp[65];
  char *tp = tmp;
  __int64 i;
  unsigned __int64 v;
  char *sp;

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


/*
 * @implemented
 */
char *
_itoa(int value, char *string, int radix)
{
  return _ltoa(value, string, radix);
}


/*
 * @implemented
 */
char *
_ltoa(long value, char *string, int radix)
{
    unsigned long val;
    int negative;
    char buffer[33];
    char *pos;
    int digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    memcpy(string, pos, &buffer[32] - pos + 1);
    return string;
}


/*
 * @implemented
 */
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
