#include <mbstring.h>

unsigned int _mbctolower(unsigned int c)
{
	if (!_ismbblead(c) )
		return tolower(c);
	return c;
}


unsigned char * _mbslwr(unsigned char *x)
{
        char  *y=x;

        while (*y) {
                *y=_mbctolower(*y);
                y++;
        }
        return x;
}