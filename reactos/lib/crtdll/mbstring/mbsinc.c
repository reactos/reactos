#include <crtdll/mbstring.h>

unsigned char * _mbsinc(const unsigned char *s)
{
	unsigned char *c = (unsigned char *)s;
	return c + (unsigned char *)mbclen(*c);
}
