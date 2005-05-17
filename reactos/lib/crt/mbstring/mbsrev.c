#include <mbstring.h>

/*
 * @implemented
 */
unsigned char * _mbsrev(unsigned char *s)
{
	unsigned char  *e;
	unsigned char  a;
	unsigned char  *e2;
	e=s;
	while (*e) {
		if ( _ismbblead(*e) ) {
			a = *e;
			e2 = e;
			*e2 = *++e;
			if ( *e == 0 )
				break;
			*e = a;
		}
		e++;
	}
	while (s<e) {
		a=*s;
		*s=*e;
		*e=a;
		s++;
		e--;
	}


	return s;
}
