/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isprint
int isprint(int c)
{
  return c;
}
