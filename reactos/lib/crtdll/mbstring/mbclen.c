//#include <crtdll/mbstring.h>

#include <crtdll/stdlib.h>


size_t _mbclen(const unsigned char *s)
{
//	return _ismbblead(s) ? 2 : 1;
	return 1;
}


int mblen( const char *mbstr, size_t count )
{
	return 1;
}



