/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isprint.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

#undef isprint
/*
 * @implemented
 */
int isprint(int c)
{
  return _isctype(c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}

/*
 * @implemented
 */
int iswprint(wint_t c)
{
  return iswctype((unsigned short)c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}
