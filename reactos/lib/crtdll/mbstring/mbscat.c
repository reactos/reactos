#include <msvcrt/string.h>

unsigned char * _mbscat(unsigned char *dst, const unsigned char *src)
{
	return strcat(dst,src);
}
