#include <direct.h>
#include <windows.h>

#undef rmdir
int rmdir( const char *_path )
{
	return _rmdir(_path);
}

int _rmdir( const char *_path )
{
	if (!RemoveDirectoryA(_path))
		return -1;
	return 0;
}