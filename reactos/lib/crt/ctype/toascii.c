/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/toascii.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int __toascii(int c)
{
    return((unsigned)(c) & 0x7F);
}
