#include <crtdll/mbstring.h>
#include <crtdll/mbctype.h>
#include <crtdll/ctype.h>

int _ismbbprint(unsigned char c)
{
	return (isprint(c) || _ismbbkana(c));
}
