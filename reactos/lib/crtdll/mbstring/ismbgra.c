#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int _ismbbgraph(unsigned char c)
{
	return (isgraph(c) || _ismbbkana(c));
}
