#include <crtdll/direct.h>
#include <windows.h>



int _rmdir( const char *_path )
{
	if (!RemoveDirectoryA(_path))
		return -1;
	return 0;
}
