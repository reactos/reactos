#include <msvcrt/string.h>

char * _strrev(char *s)
{
	char  *e;
	char   a;
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
