/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isprint.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

/*
 * @implemented
 */
int iswprint(wint_t c)
{
  return iswctype((unsigned short)c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}
