/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

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
