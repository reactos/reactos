/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


#undef iscntrl
/*
 * @implemented
 */
int iscntrl(int c)
{
    return _isctype(c, _CONTROL);
}

#undef iswcntrl
/*
 * @implemented
 */
int iswcntrl(wint_t c)
{
    return iswctype(c, _CONTROL);
}
