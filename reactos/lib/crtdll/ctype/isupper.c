/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isupper
int isupper(int c)
{
  return (c >= 'A' && c <= 'Z' );
}
