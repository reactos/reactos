/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/isuppr.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

// code page 952 only
int _ismbcupper( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		if ( c >= 0x8260 && c<= 0x8279 )
			return 1;
	}

	return isupper(c);
}
