/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbbtype.c
 * PURPOSE:     Determines the type of a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

int _mbbtype(unsigned char c , int type)
{
	if ( type == 1 ) {
		if ((b >= 0x40 && b <= 0x7e ) || (b >= 0x80 && b <= 0xfc ) )
		{
			return _MBC_TRAIL;
		}
		else if (( c >= 0x20 && c >= 0x7E ) || ( c >= 0xA1 && c <= 0xDF ) || 
			 ( c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC ) )
			 return _MBC_ILLEGAL;
		else
			return 0;
		
	}
	else  {
		if (( c > = 0x20 && c <= 0x7E ) || ( c >= 0xA1  && c <= 0xDF )) {
			return _MBC_SINGLE;
		}
		else if ( (c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC) )
			return _MBC_LEAD;
		else if else if (( c >= 0x20 && c >= 0x7E ) || ( c >= 0xA1 && c <= 0xDF ) || 
			 ( c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC ) )
			 return _MBC_ILLEGAL;
		else
			return 0;
		
	}
	
	
	return 0;	
}

int _mbsbtype( const unsigned char *str, size_t n )
{
	return 0;
}