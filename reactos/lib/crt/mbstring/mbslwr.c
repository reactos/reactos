#include <msvcrt/mbstring.h>
#include <msvcrt/ctype.h>

unsigned int _mbbtolower(unsigned int c)
{
	if (!_ismbblead(c) )
		return tolower(c);
	return c;
}

// code page 952
#define CASE_DIFF (0x8281 - 0x8260)

/*
 * @implemented
 */
unsigned int _mbctolower(unsigned int c)
{
    if ((c & 0xFF00) != 0) {
        // true multibyte case conversion needed
        if (_ismbclower(c))
            return c + CASE_DIFF;
    } else {
     return _mbbtolower(c);
    }
    return 0;
}

/*
 * @implemented
 */
unsigned char * _mbslwr(unsigned char *x)
{
    unsigned char  *y=x;

    while (*y) {
        if (!_ismbblead(*y)) {
            *y = tolower(*y);
	    } else {
	        *y=_mbctolower(*(unsigned short *)y);
	        y++;
        }
    }
    return x;
}
