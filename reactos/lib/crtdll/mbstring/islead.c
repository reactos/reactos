#include <windows.h>
#include <msvcrt/mbstring.h>

/*
 * @unimplemented
 */
int isleadbyte(char *mbstr)
{
	return 0;
	//return IsDBCSLeadByteEx(0,*c);
}
