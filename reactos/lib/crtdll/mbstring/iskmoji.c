#include <crtdll/mbctype.h>

int _ismbbkalpha(unsigned char c)
{
	return (0xA7 <= c <= 0xDF);
}