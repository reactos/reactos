#include <msvcrt/string.h>

/*
 * @implemented
 */
size_t wcscspn(const wchar_t *str,const wchar_t *reject)
{
	wchar_t *s;
	wchar_t *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)reject;
		while (*t) { 
			if (*t==*s) 
				break;
			t++;
		}
		if (*t) 
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}
