/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsrchr.c 
 * PURPOSE:     Searches for a character in reverse
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

unsigned char * _mbsrchr(const unsigned char *str, unsigned int c)
{
	
	unsigned char *s = str;
	
	int count = mbblen(s);       

	s += count;
	if ( c >> 8 == 0 ) {
	
		while( count > 0 ) {
			if ( *s == c )
				return s;
                	count--;
			s--;
        	}
        }
        else {
		while( count > 0 ) {
			if ( *((short *)s) == c )
				return s;
                	count--;
			s--;
        	}
        }
        
        return src;

}