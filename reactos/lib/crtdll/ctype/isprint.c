/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isprint
int isprint(int c)
{
  return _isctype((unsigned char)c,_PRINT);
}

int iswprint(int c)
{
  return iswctype((unsigned short)c,_PRINT);
}