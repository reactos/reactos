#include <crtdll/mbctype.h>

int _ismbbalnum(unsigned char c)
{
	return (isalnum(c) || _ismbbkalnum(c));
}