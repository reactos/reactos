#include <msvcrti.h>


unsigned char * _mbsrev(unsigned char *s)
{
	unsigned char  *e;
	unsigned char  a;
	e=s;
	while (*e) {
		if ( _ismbblead(*e) ) {
			a = *e;
			*e = *++e;
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
