#include <msvcrti.h>


unsigned char * _mbsdec(const unsigned char *str, const unsigned char *cur)
{
	unsigned char *s = (unsigned char *)cur;
	if ( str >= cur )
		return (void *)0;

	s--;
	if (_ismbblead(*(s-1)) )
		s--;

	return s; 
}
