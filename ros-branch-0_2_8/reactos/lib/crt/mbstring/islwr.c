/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/islwr.c
 * PURPOSE:
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <mbctype.h>
#include <ctype.h>

/*
 * code page 952 only
 *
 * @implemented
 */
int _ismbclower( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		if ( c >= 0x829A && c<= 0x829A )
			return 1;
	}

	return islower(c);
}
