#include <windows.h>
#include <crtdll/string.h>

size_t strxfrm( char *dest, const char *src, size_t n )
{
	
	int ret = LCMapStringA(LOCALE_USER_DEFAULT,LCMAP_LOWERCASE,	
    	src, strlen(src),	
    	dest, strlen(dest) );


	if ( ret == 0 )
		return -1;
	return ret;

}