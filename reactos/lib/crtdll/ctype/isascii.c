/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <ctype.h>

int __isascii(int c)
{
  return  (!((c)&(~0x7f))) ;
}

int iswascii(int c)
{
	return __isascii(c);
}








