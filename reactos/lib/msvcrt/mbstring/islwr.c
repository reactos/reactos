/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsncmp.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

// code page 952 only
int _ismbclower( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		if ( c >= 0x829A && c<= 0x829A )
			return 1;
	}

	return islower(c);
}
