/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isxdigit
int isxdigit(int c)
{
 return _isctype(c,_HEX);
}

#undef iswxdigit
int iswxdigit(int c)
{
 return iswctype(c,_HEX);
}

