/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsnset.c 
 * PURPOSE:     Fills a string with a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <msvcrt/mbstring.h>

size_t _mbclen2(const unsigned int s);

unsigned char * _mbsnset(unsigned char *src, unsigned int val, size_t count)
{
	unsigned char *char_src = (unsigned char *)src;
	unsigned short *short_src = (unsigned short *)src;

	if ( _mbclen2(val) == 1 ) {
	
		while(count > 0) {
			*char_src = val;
			char_src++;
			count--;
		}
		*char_src = 0;
	}
	else {
		while(count > 0) {
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
	unsigned char *char_src = (unsigned char *)src;
	unsigned short *short_src = (unsigned short *)src;

	if ( _mbclen2(val) == 1 ) {
	
		while(count > 0) {
			*char_src = val;
			char_src++;
			count--;
		}
		*char_src = 0;
	}
	else {
		while(count > 0) {
			*short_src = val;
			short_src++;
			count-=2;
		}
		*short_src = 0;
	}

	return src;
}
