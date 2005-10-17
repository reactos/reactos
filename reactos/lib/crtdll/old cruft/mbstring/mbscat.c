#include <msvcrt/string.h>

/*
 * @implemented
 */
unsigned char * _mbscat(unsigned char *dst, const unsigned char *src)
{
	return strcat(dst,src);
}
