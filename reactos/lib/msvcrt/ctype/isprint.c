/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int iswprint(wint_t c)
{
  return iswctype((unsigned short)c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}
