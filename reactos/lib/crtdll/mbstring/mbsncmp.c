/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsncmp.c
 * PURPOSE:     Compares two strings to a maximum of n bytes or characters
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

int _mbsncmp(const unsigned char *str1, const unsigned char *str2, size_t n)
{
	unsigned char *s1;
	unsigned char *s2;
	if (n == 0)
		return 0;
	do {
		if (*s1 != *s2++)
			return *(unsigned const char *)s1 - *(unsigned const char *)--s2;
    		if (*s1++ == 0)
			break;
			
		if (!_ismbblead(*s1) )
			n--;
	} while (n > 0);
	return 0;		
}

int _mbsnbcmp(const unsigned char *str1, const unsigned char *str2, size_t n)
{
	unsigned char *s1;
	unsigned char *s2;
	if (n == 0)
		return 0;
	do {
		if (*s1 != *s2++)
			return *(unsigned const char *)s1 - *(unsigned const char *)--s2;
    		if (*s1++ == 0)
			break;
			
		if (!(n == 1 && _ismbblead(*s)) )
			n--;
	} while (n > 0);
	return 0;
}