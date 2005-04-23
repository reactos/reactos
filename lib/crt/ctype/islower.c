/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/islower.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>


#undef islower
/*
 * @implemented
 */
int islower(int c)
{
    return _isctype(c, _LOWER);
}

/*
 * @implemented
 */
int iswlower(wint_t c)
{
    return iswctype(c, _LOWER);
}
