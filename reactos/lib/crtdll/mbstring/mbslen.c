#include <msvcrt/mbstring.h>

size_t _mbclen2(const unsigned int s);

size_t _mbslen(const unsigned char *str)
{
	int i = 0;
	unsigned char *s;

	if (str == 0)
		return 0;
		
	for (s = (unsigned char *)str; *s; s+=_mbclen2(*s),i++);
	return i;
}
