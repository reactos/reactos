/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/ismblead.c
 * PURPOSE:     Converts a multi byte byte to a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

char _mbctype[256];

// multibyte byte to multibyte character ????
unsigned int _mbbtombc(unsigned int c)
{
	if (( c > = 0x20 && c <= 0x7E ) || ( c >= 0xA1  && c <= 0xDF )) {
		// convert		
	}
	return c;
}

unsigned int _mbctombb( unsigned int c )
{
	return c;
}


