/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef islower
int islower(int c)
{
  return _isctype((unsigned char)c,_LOWER);
}

int iswlower(int c)
{
  return iswctype((unsigned short)c,_LOWER);
}