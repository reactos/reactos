/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>

#undef iswpunct
/*
 * @implemented
 */
int iswpunct(wint_t c)
{
    return iswctype(c, _PUNCT);
}
