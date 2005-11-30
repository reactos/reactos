/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
wchar_t * _wcsrev(wchar_t *s)
{
	wchar_t  *e;
	wchar_t   a;
	e=s;
	while (*e)
		e++;
	while (s<e) {
		a=*s;
		*s=*e;
		*e=a;
		s++;
		e--;
	}
	return s;
}
