#include <crtdll/direct.h>
#include <windows.h>


int _mkdir( const char *_path )
{
	if (!CreateDirectoryA(_path,NULL))
		return -1;
	return 0;
}
