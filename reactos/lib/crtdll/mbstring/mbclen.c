#include <crtdll/mbstring.h>

#include <crtdll/stdlib.h>


size_t _mbclen(const unsigned char *s)
{
	return (_ismbblead(*s>>8) && _ismbbtrail(*s&0x00FF)) ? 2 : 1;
}

size_t _mbclen2(const unsigned int s)
{
	return (_ismbblead(s>>8) && _ismbbtrail(s&0x00FF)) ? 2 : 1;
}

// assume MB_CUR_MAX == 2
int mblen( const char *s, size_t count )
{
	size_t l;
	if ( s == NULL )
		return 0;

	l =  _mbclen(s);
	if ( l < count )
		return -1;
	return l;
}
