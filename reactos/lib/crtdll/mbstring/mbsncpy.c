/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsncpy.c
 * PURPOSE:     Copies a string to a maximum of n bytes or characters
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>


unsigned char *_mbsncpy(unsigned char *str1, const unsigned char *str2, size_t n)
{
	unsigned char *s1 = (unsigned char *)str1;
	unsigned char *s2 = (unsigned char *)str2;

	unsigned short *short_s1, *short_s2;

	if (n == 0)
		return 0;
	do {
		
		if (*s2 == 0)
			break;	

		if (  !_ismbblead(*s2) ) {

			*s1 = *s2;
			s1 += 1;
			s2 += 1;
			n--;
		}
		else {
			short_s1 = (unsigned short *)s1;
			short_s2 = (unsigned short *)s2;
			*short_s1 = *short_s2;
			s1 += 2;
			s2 += 2;
			n--;
		}
	} while (n > 0);
	return str1;
}


//
//The _mbsnbcpy function copies count bytes from src to dest. If src is shorter 
//than dest, the string is padded with null characters. If dest is less than or 
//equal to count it is not terminated with a null character.
//

unsigned char * _mbsnbcpy(unsigned char *str1, const unsigned char *str2, size_t n)
{
	unsigned char *s1 = (unsigned char *)str1;
	unsigned char *s2 = (unsigned char *)str2;

	unsigned short *short_s1, *short_s2;

	if (n == 0)
		return 0;
	do {
		
        if (*s2 == 0) {
			*s1 = *s2;
			break;	
        }

		if (  !_ismbblead(*s2) ) {

			*s1 = *s2;
			s1 += 1;
			s2 += 1;
			n--;
		}
		else {
			short_s1 = (unsigned short *)s1;
			short_s2 = (unsigned short *)s2;
			*short_s1 = *short_s2;
			s1 += 2;
			s2 += 2;
			n-=2;
		}
	} while (n > 0);
	return str1;
}
