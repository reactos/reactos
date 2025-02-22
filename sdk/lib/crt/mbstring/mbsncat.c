/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsncat.c
 * PURPOSE:     Concatenate two multi byte string to maximum of n characters or bytes
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <precomp.h>
#include <mbstring.h>
#include <string.h>

size_t _mbclen2(const unsigned int s);
unsigned char *_mbset (unsigned char *string, int c);

/*
 * @implemented
 */
unsigned char *_mbsncat (unsigned char *dst, const unsigned char *src, size_t n)
{
    int c;
    unsigned char *save = dst;

    while ((c = _mbsnextc (dst)))
	dst = _mbsinc (dst);

    while (n-- > 0 && (c = _mbsnextc (src))) {

	_mbset (dst, c);

	dst = _mbsinc (dst);

	src = _mbsinc ((unsigned char *) src);

    }

    *dst = '\0';

    return save;
}

/*
 * @implemented
 */
unsigned char * _mbsnbcat(unsigned char *dst, const unsigned char *src, size_t n)
{
	unsigned char *d;
	const unsigned char *s = src;
	if (n != 0) {
		d = dst + _mbslen(dst); // get the end of string
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
