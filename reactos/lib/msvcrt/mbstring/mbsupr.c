/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsupr.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <msvcrt/mbstring.h>
#include <msvcrt/ctype.h>

unsigned int _mbbtoupper(unsigned int c)
{
	if (!_ismbblead(c) )
		return toupper(c);
	
	return c;
}

// codepage 952
#define CASE_DIFF (0x8281 - 0x8260)

unsigned int _mbctoupper(unsigned int c)
{

        if ((c & 0xFF00) != 0) {
// true multibyte case conversion needed
	if ( _ismbcupper(c) )
		return c + CASE_DIFF;

        } else
		return _mbbtoupper(c);

	return 0;
}

unsigned char * _mbsupr(unsigned char *x)
{
	unsigned char  *y=x;
        while (*y) {
		if (!_ismbblead(*y) )
			*y = toupper(*y);
		else {
                	*y=_mbctoupper(*(unsigned short *)y);
                	y++;
		}
        }
        return x;
}