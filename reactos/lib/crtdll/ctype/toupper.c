/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef toupper
int toupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
