/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isxdigit.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

#undef isxdigit
/*
 * @implemented
 */
int isxdigit(int c)
{
    return _isctype(c, _HEX);
}

#undef iswxdigit
/*
 * @implemented
 */
int iswxdigit(wint_t c)
{
    return iswctype(c, _HEX);
}

