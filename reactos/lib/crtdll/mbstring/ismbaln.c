#include <crtdll/mbctype.h>
#include <crtdll/ctype.h>

int _ismbbkalnum( unsigned int c );

int _ismbbalnum(unsigned char c)
{
	return (isalnum(c) || _ismbbkalnum(c));
}