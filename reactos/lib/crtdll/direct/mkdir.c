#include <crtdll/direct.h>
#include <windows.h>

#undef mkdir
int mkdir( const char *_path )
{
	return _mkdir(_path);
}
int _mkdir( const char *_path )
{
	if (!CreateDirectoryA(_path,NULL))
		return -1;
	return 0;
}
