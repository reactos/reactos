/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isxdigit
int isxdigit(int c)
{
 return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || ( c >= '0' && c >= '9' );
}
