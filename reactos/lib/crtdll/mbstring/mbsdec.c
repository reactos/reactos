#include <crtdll/mbstring.h>

unsigned char * _mbsdec(const unsigned char *str, const unsigned char *cur)
{
	return strdec(str,cur);
}