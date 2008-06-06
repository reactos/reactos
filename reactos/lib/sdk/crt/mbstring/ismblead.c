/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/iskana.c
 * PURPOSE: 
 * PROGRAMER:
 * UPDATE HISTORY:
 *              12/04/99:  Ariadne, Taiji Yamada Created
 *              05/30/08:  Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>

size_t _mbclen2(const unsigned int s);

/*
 * @implemented
 */
int _ismbblead(unsigned int c)
{
   return (_mbctype[c & 0xff] & _MLEAD);
}

/*
 * @implemented
 */
int _ismbslead( const unsigned char *str, const unsigned char *t)
{
	unsigned char *s = (unsigned char *)str;
	while(*s != 0 && s != t)
	{

		s+= _mbclen2(*s);
	}
	return _ismbblead( *s);
}

/*
 * @implemented
 */
const unsigned char *__p__mbctype(void)
{
	return _mbctype;
}


