/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


#undef isupper
/*
 * @implemented
 */
int isupper(int c)
{
    return _isctype(c, _UPPER);
}

/*
 * @implemented
 */
int iswupper(wint_t c)
{
    return iswctype(c, _UPPER);
}
