/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


#undef isdigit
/*
 * @implemented
 */
int isdigit(int c)
{
   return _isctype(c, _DIGIT);
}

#undef iswdigit
/*
 * @implemented
 */
int iswdigit(wint_t c)
{
   return iswctype(c, _DIGIT);
}
