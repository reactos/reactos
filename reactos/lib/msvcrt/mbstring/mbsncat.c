/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsncat.c 
 * PURPOSE:     Concatenate two multi byte string to maximum of n characters or bytes
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>
#include <msvcrt/string.h>

size_t _mbclen2(const unsigned int s);

/*
 * @implemented
 */
unsigned char * _mbsncat(unsigned char *dst, const unsigned char *src, size_t n)
{
	char *d = (char *)dst;
	char *s = (char *)src;
	if (n != 0) {
		d = dst + strlen(dst); // get the end of string 
		d += _mbclen2(*d); // move 1 or 2 up

		do {
			if ((*d++ = *s++) == 0)
			{
				while (--n != 0)
					*d++ = 0;
				break;
			}
			if (!_ismbblead(*s) )
				n--;
		} while (n > 0);
	}
	return dst;
}

/*
 * @implemented
 */
unsigned char * _mbsnbcat(unsigned char *dst, const unsigned char *src, size_t n)
{
	char *d; 
	char *s = (char *)src;
	if (n != 0) {
		d = dst + strlen(dst); // get the end of string 
		d += _mbclen2(*d); // move 1 or 2 up

		do {
			if ((*d++ = *s++) == 0)
			{
				while (--n != 0)
					*d++ = 0;
				break;
			}
			if ( !(n==1 && _ismbblead(*s)) )
				n--;
		} while (n > 0);
	}
	return dst;
}
