/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/ctype/isspace.c
 * PURPOSE:     Test for a space character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <ctype.h>

#undef isspace
int isspace(int c)
{
  return _isctype((unsigned char)c,_SPACE);
}

#undef iswspace
int iswspace(int c)
{
  return iswctype((unsigned short)c,_SPACE);
}
