/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsset.c 
 * PURPOSE:     Fills a string with a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>

size_t _mbclen2(const unsigned int s);

/*
 * @implemented
 */
unsigned char * _mbsset(unsigned char *src, unsigned int c)
{
	unsigned char *char_src = src;
	unsigned short *short_src = (unsigned short *)src;

	if ( _mbclen2(c) == 1 ) {
	
		while(*char_src != 0) {
			*char_src = c;
			char_src++;
		}
		*char_src = 0;
	}
	else {
		while(*short_src != 0) {
			*short_src = c;
			short_src++;
		}
		*short_src = 0;
	}

	return src;
}
