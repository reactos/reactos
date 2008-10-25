/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismbtrl.c
 * PURPOSE:     Checks for a trailing byte
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <precomp.h>

size_t _mbclen2(const unsigned int s);

//  iskanji2()   : (0x40 <= c <= 0x7E 0x80  <=  c <= 0xFC)

/*
 * @implemented
 */
int _ismbbtrail(unsigned int c)
{
   return (_mbctype[c & 0xff] & _MTRAIL);
}


/*
 * @implemented
 */
int _ismbstrail( const unsigned char *str, const unsigned char *t)
{
	unsigned char *s = (unsigned char *)str;
	while(*s != 0 && s != t)
	{

		s+= _mbclen2(*s);
	}

	return _ismbbtrail(*s);
}
