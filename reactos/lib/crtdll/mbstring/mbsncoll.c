/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsncoll.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <crtdll/mbstring.h>

int _mbsncoll(const unsigned char *, const unsigned char *, size_t)
{
	int l1, l2;
	int ret;
	
	l1 = mbslen(str1);
	l2 = mbslen(str2);
	ret = CompareStringA(LOCALE_USER_DEFAULT,0,str1,min(l1,n),str2,min(l2,n));
	
	if ( ret != 0 )
	
		return ret -2;
	
	return 0;
}

int _mbsnbcoll(const unsigned char *str1, const unsigned char *str2, size_t n)
{
	int l1, l2;
	int ret;
	
	l1 = strlen(str1);
	l2 = strlen(str2);
	ret = CompareStringA(LOCALE_USER_DEFAULT,0,str1,min(l1,n),str2,min(l2,n));
	
	if ( ret != 0 )
	
		return ret -2;
	
	return 0;
}