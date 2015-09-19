/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/wstring/wcscspn.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
size_t CDECL wcscspn(const wchar_t *str,const wchar_t *reject)
{
	wchar_t *s;
	wchar_t *t;
	s=(wchar_t *)str;
	while (*s) {
		t=(wchar_t *)reject;
		while (*t) {
			if (*t==*s)
				break;
			t++;
		}
		if (*t)
			break;
		s++;
	}
	return s-str; /* nr of wchars */
}
