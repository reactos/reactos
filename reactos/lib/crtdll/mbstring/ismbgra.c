#include <crtdll/mbstring.h>
#include <crtdll/mbctype.h>
#include <crtdll/ctype.h>

int _ismbbgraph(unsigned char c)
{
	return (isgraph(c) || _ismbbkana(c));
}