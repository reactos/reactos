#include <windows.h>
#include <msvcrt/string.h>

#if 1
/*
 * @implemented
 */
size_t strxfrm( char *dest, const char *src, size_t n )
{
   strncpy(dest, src, n);
   return (strlen(dest));
}
#else
size_t strxfrm( char *dest, const char *src, size_t n )
{
	int ret = LCMapStringA(LOCALE_USER_DEFAULT,LCMAP_LOWERCASE,
                           src, strlen(src), dest, strlen(dest));

	if ( ret == 0 )
		return -1;
	return ret;

}
#endif
