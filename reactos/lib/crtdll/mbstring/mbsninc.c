#include <crtdll/mbstring.h>

unsigned char * _mbsninc(const unsigned char *str, size_t n)
{
	unsigned char *s = (unsigned char *)str;
	while(*s != 0 && n > 0) {
		if (!_ismbblead(*s) )
			n--;
		s++;
	}
	
	return s;
}