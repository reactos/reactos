/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/ctype.h>

#undef isupper
int isupper(int c)
{
  return _isctype(c,_UPPER);
}

int iswupper(wint_t c)
{
  return iswctype(c,_UPPER);
}
