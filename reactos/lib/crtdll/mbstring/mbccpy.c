#include <crtdll/mbstring.h>

void _mbccpy(unsigned char *dst, const unsigned char *src)
{
	if (!_ismbblead(*src) )
		return;
		
	memcpy(dst,src,_mbclen2(*src));
}