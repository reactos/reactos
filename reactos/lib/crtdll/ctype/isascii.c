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

#undef isascii
int isascii(int c)
{
  return  ( (unsigned)(c) <0x80 ) ;
}

int __isascii(int c)
{
  return  ( (unsigned)(c) <0x80 ) ;
}








