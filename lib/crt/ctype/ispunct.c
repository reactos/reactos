/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/ispunct.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

#undef ispunct
/*
 * @implemented
 */
int ispunct(int c)
{
    return _isctype(c, _PUNCT);
}

#undef iswpunct
/*
 * @implemented
 */
int iswpunct(wint_t c)
{
    return iswctype(c, _PUNCT);
}
