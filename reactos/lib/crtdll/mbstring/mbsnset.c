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

unsigned char * _mbsnset(unsigned char *src, unsigned int val, size_t count)
{

	
	unsigned char *char_s = src;
	unsigned short *short_s = src;
       
	if ( c >> 8 == 0 ) {
	
		while(*s != 0 && count > 0) {
                	*char_src = val;
                	char_src++;
                	count--;
        	}
        	*char_src = 0;
        }
        else {
		while(*s != 0 && count > 0) {
                	*short_src = val;
                	short_src++;
                	count-=2;
        	}    	
        	*short_src = 0;
        }
        
        return src;
	
}

unsigned char * _mbsnbset(unsigned char *src, unsigned int val, size_t count)
{
	
	unsigned char *char_s = src;
	unsigned short *short_s = src;
       
	if ( c >> 8 == 0 ) {
	
		while(*s != 0 && count > 0) {
                	*char_src = val;
                	char_src++;
                	count--;
        	}
        	*char_src = 0;
        }
        else {
		while(*s != 0 && count > 0) {
                	*short_src = val;
                	short_src++;
                	count--;
        	}    	
        	*short_src = 0;
        }
        
        return src;
}