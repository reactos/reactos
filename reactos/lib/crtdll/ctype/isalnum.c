/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/putch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <ctype.h>

#undef isalnum
int isalnum(int c)
{
   return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')  || (c >= '0' && c <= '9'));
}
