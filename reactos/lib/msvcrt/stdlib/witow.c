/* $Id: witow.c,v 1.1 2002/11/24 18:42:25 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/itow.c
 * PURPOSE:     converts a integer to wchar_t
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa Boudewijn Dekker
 *              2000: derived from ./itoa.c by ea
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/errno.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/file.h>


wchar_t* _i64tow(__int64 value, wchar_t* string, int radix)
{
  wchar_t tmp[65];
  wchar_t* tp = tmp;
  int i;
  unsigned v;
  int sign;
  wchar_t* sp;

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
      *tp++ = i+L'0';
    else
      *tp++ = i + L'a' - 10;
  }

  if (string == 0)
    string = (wchar_t*)malloc(((tp-tmp)+sign+1)*sizeof(wchar_t));
  sp = string;

  if (sign)
    *sp++ = L'-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}

wchar_t* _ui64tow(unsigned __int64 value, wchar_t* string, int radix)
{
  wchar_t tmp[65];
  wchar_t* tp = tmp;
  long i;
  unsigned long v = value;
  wchar_t* sp;

  if (radix > 36 || radix <= 1)
  {
    __set_errno(EDOM);
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

  if (string == 0)
    string = (wchar_t*)malloc(((tp-tmp)+1)*sizeof(wchar_t));
  sp = string;

  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}
