#include <msvcrt/string.h>

/*
 * @implemented
 */
char * _strrev(char *s)
{
	char a, *b, *e;
	b=e=s;
	while (*e)
		e++;
	e--; /* start at last char, not NULL char */
	while ( b < e )
	{
		a=*b;
		*b=*e;
		*e=a;
		b++;
		e--;
	}
	return s; /* return ptr to beginning of string */
}
