/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/putch.c
 * PURPOSE:     Checks if a character is alphanumeric
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <ctype.h>

#undef isalpha
int isalpha(int c)
{
 return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
