/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <ctype.h>


int __toascii(int c)
{
  return ((unsigned)(c) & 0x7F );
}
