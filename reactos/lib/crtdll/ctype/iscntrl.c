/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef iscntrl
int iscntrl(int c)
{
  return _isctype(c,_CONTROL);
}

#undef iswcntrl
int iswcntrl(int c)
{
  return iswctype(c,_CONTROL);
}
