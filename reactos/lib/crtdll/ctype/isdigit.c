/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isdigit
int isdigit(int c)
{
   return (c >= '0' && c <= '9');
}
