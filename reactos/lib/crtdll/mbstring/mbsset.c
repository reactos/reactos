/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsset.c 
 * PURPOSE:     Fills a string with a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

unsigned char * _mbsset(unsigned char *str, unsigned int c)
{
	unsigned char *char_s = src;
	unsigned short *short_s = src;
       
	if ( c >> 8 == 0 ) {
	
		while(*s != 0) {
                	*char_src = val;
                	char_src++;
        	}
        	*char_src = 0;
        }
        else {
		while(*s != 0) {
                	*short_src = val;
                	short_src++;
        	}    	
        	*short_src = 0;
        }
        
        return src;
}