#include <windows.h>


int remove(const char *fn)
{
	if (!DeleteFileA(fn))
		return -1;
	return 0;
}
