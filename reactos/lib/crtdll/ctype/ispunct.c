/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef ispunct
int ispunct(int c)
{
  return (c == '.');
}
