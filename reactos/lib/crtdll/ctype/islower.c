/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef islower
int islower(int c)
{
  return (c >= 'a' && c <= 'z');
}
