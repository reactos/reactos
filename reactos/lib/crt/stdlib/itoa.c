/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/itoa.c
 * PURPOSE:     converts a integer to ascii
 * PROGRAMER:
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa by Ariadne
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <precomp.h>

/*
 * @implemented
 */
char* _itoa(int value, char* string, int radix)
{
  char tmp[33];
  char* tp = tmp;
  int i;
  unsigned v;
  int sign;
  char* sp;

  if (radix > 36 || radix <= 1)
  {
    __set_errno(EDOM);
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

  if (string == 0)
    string = (char*)malloc((tp-tmp)+sign+1);
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
char* _ltoa(long value, char* string, int radix)
{
  char tmp[33];
  char* tp = tmp;
  long i;
  unsigned long v;
  int sign;
  char* sp;

  if (radix > 36 || radix <= 1)
  {
     __set_errno(EDOM);
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

  if (string == 0)
    string = (char*)malloc((tp-tmp)+sign+1);
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
 * copy it from wine 0.9.0 with small modifcations do check for NULL
 */
char* ultoa(unsigned long value, char* string, int radix)
{
    char buffer[33];
    char *pos;
    int digit;
    
    pos = &buffer[32];
    *pos = '\0';

    if (string == NULL)
    {
      return NULL;         
    }
    
    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[32] - pos + 1);
    return string;
}
