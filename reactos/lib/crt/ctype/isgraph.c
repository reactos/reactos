/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isgraph.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

#undef iswgraph
/*
 * @implemented
 */
int iswgraph(wint_t c)
{
  return iswctype(c,_PUNCT | _ALPHA | _DIGIT);
}
