#include <crtdll/wchar.h>

size_t wcsspn(const wchar_t *str,const wchar_t *accept)
{
	wchar_t  *s;
	wchar_t  *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)accept;
		while (*t) { 
			if (*t==*s) 
				break;
			t++;
		}
		if (!*t) 
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}
