/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/putch.c
 * PURPOSE:     Checks if a character is alphanumeric
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <crtdll/ctype.h>

#undef isalpha
int isalpha(int c)
{
 	return _isctype(c,_ALPHA);
}

#undef iswalpha
int iswalpha(wint_t c)
{
	return iswctype(c,_ALPHA);
}
