/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef ispunct
int ispunct(int c)
{
  return _isctype(c,_PUNCT);
}

#undef iswpunct
int iswpunct(int c)
{
  return iswctype(c,_PUNCT);
}
