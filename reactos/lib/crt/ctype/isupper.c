/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isupper.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int iswupper(wint_t c)
{
    return iswctype(c, _UPPER);
}
