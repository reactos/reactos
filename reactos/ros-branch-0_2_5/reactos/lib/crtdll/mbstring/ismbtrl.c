/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/ismbtrail.c
 * PURPOSE:     Checks for a trailing byte 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>

size_t _mbclen2(const unsigned int s);

//  iskanji2()   : (0x40 <= c <= 0x7E 0x80  <=  c <= 0xFC) 

/*
 * @implemented
 */
int _ismbbtrail(unsigned int c)
{
	return ((_jctype+1)[(unsigned char)(c)] & _KNJ_2);
}

//int _ismbbtrail( unsigned int b)
//{
//	return ((b >= 0x40 && b <= 0x7e ) || (b >= 0x80 && b <= 0xfc ) );
//}


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
