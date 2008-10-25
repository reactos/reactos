#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
unsigned char *_mbsspnp (const unsigned char *str1, const unsigned char *str2)
{
    int c;
    unsigned char *ptr;

    while ((c = _mbsnextc (str1))) {

	if ((ptr = _mbschr (str2, c)) == 0)
	    return (unsigned char *) str1;

	str1 = _mbsinc ((unsigned char *) str1);

    }

    return 0;
}
