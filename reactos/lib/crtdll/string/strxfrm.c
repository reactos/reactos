#include <windows.h>
#include <msvcrt/string.h>

/*
 * @unimplemented
 */
size_t strxfrm( char *dest, const char *src, size_t n )
{
#if 1
   strncpy(dest, src, n);
   return (strlen(dest));
#else
	int ret = LCMapStringA(LOCALE_USER_DEFAULT,LCMAP_LOWERCASE,
                           src, strlen(src), dest, strlen(dest));

	if ( ret == 0 )
		return -1;
	return ret;
#endif
}
