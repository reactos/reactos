#include <crtdll/wchar.h>

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
