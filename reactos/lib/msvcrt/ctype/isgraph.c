/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>

#undef iswgraph
/*
 * @implemented
 */
int iswgraph(wint_t c)
{
  return iswctype(c,_PUNCT | _ALPHA | _DIGIT);
}
