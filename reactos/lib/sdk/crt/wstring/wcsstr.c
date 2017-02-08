/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/wstring/wcsstr.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
wchar_t * CDECL wcsstr(const wchar_t *s,const wchar_t *b)
{
	wchar_t *x;
	wchar_t *y;
	wchar_t *c;
	x=(wchar_t *)s;
	while (*x) {
		if (*x==*b) {
			y=x;
			c=(wchar_t *)b;
			while (*y && *c && *y==*c) {
				c++;
				y++;
			}
			if (!*c)
				return x;
		}
		x++;
	}
	return NULL;
}
