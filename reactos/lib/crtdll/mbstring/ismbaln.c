#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>


int _ismbbalnum(unsigned char c)
{
	return (isalnum(c) || _ismbbkalnum(c));
}

