#include <windows.h>
#include <io.h>

int
_chmod(const char *filename, int func)
{
	DWROD FileAttributes = 0;
	if ( func == _S_IREAD )
		FileAttributes &= FILE_ATTRIBUTE_READONLY;
	if ( ((func & _S_IREAD) == _S_IREAD) && ((func & _S_IWRITE) == _S_IWRITE) )
		FileAttributes &= FILE_ATTRIBUTE_NORMAL;
		


	if ( SetFileAttributes(filename,func) == FALSE )
		return -1;

	return 1;
}
