/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/iskpun.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <crtdll/mbstring.h>
#include <crtdll/mbctype.h>
#include <crtdll/ctype.h>
int colldif(unsigned short c1, unsigned short c2);
int _mbsicoll(const unsigned char *str1, const unsigned char *str2)
{
	unsigned char *s1 = (unsigned char *)str1;
	unsigned char *s2 = (unsigned char *)str2;

	unsigned short *short_s1, *short_s2;

	int l1, l2;

	while ( *s1 != 0 ) {
		
		if (*s1 == 0)
			break;	

		l1 = _ismbblead(*s1);
		l2 = _ismbblead(*s2);
		if ( !l1 &&  !l2  ) {

			if (toupper(*s1) != toupper(*s2))
				return colldif(*s1, *s2);
			else {
				s1 += 1;
				s2 += 1;
			}
		}
		else if ( l1 && l2 ){
			short_s1 = (unsigned short *)s1;
			short_s2 = (unsigned short *)s2;
			if ( _mbctoupper(*short_s1) != _mbctoupper(*short_s2 ))
				return colldif(*short_s1, *short_s2);
			else {
				s1 += 2;
				s2 += 2;

			}
		}
		else
			return colldif(*s1, *s2);
	} ;
	return 0;
}
