/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef iscntrl
int iscntrl(int c)
{
  return ((c >=0x00 && c <= 0x1f) || c == 0x7f) ;
}
