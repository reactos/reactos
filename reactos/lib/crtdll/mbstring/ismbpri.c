#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int _ismbbprint(unsigned char c)
{
	return (isprint(c) || _ismbbkana(c));
}
