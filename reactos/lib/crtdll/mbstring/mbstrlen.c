#include <crtdll/mbstring.h>
#include <crtdll/stdlib.h>

size_t _mbstrlen( const char *string )
{
	char *s = (char *)string;
	size_t i;
	while ( *s != 0 ) {
		if ( _ismbblead(*s) )
			s++;
		s++;
		i++;
	}
	return i;
}