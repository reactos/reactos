/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isdigit.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

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
