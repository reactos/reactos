/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/ctype/isspace.c
 * PURPOSE:     Checks for a whitespace characters
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


#undef isspace
int isspace(int c)
{
  return _isctype(c,_SPACE);
}

#undef iswspace
int iswspace(wint_t c)
{
  return iswctype(c,_SPACE);
}
