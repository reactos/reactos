#include <crtdll/mbstring.h>

unsigned char * _mbsncpy(unsigned char *dst, const unsigned char *src, size_t n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	if (n != 0) {
		do {
			if ((*d++ = *s++) == 0)
			{
				while (--n != 0) {
					*d++ = 0;
				}
				break;
			}
			if (!_ismbblead(*s) )
				n--;
		} while (n > 0);
	}
	return dst;
}

unsigned char * _mbsnbcpy(unsigned char *src, const unsigned char *dst, size_t n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	if (n != 0) {
		do {
			if ((*d++ = *s++) == 0)
			{
				while (--n != 0) {
					*d++ = 0;
				}
				break;
			}
			if (!(n == 1 && _ismbblead(*s)) )
				n--;
		} while (n > 0);
	}
	return dst;
}