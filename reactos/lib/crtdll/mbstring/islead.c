#include <windows.h>
#include <crtdll/mbstring.h>

int isleadbyte(char *mbstr)
{
	return IsDBCSLeadByteEx(0,*c);
}