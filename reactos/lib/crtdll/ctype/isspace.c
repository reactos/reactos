/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isspace
int isspace(int c)
{
  return ( c == ' ' || c == '\t' );
}
