/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>
#undef tolower
int tolower(int c)
{
 return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}
