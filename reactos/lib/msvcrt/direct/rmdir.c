#include <windows.h>
#include <msvcrt/direct.h>

int _rmdir( const char *_path )
{
	if (!RemoveDirectoryA(_path))
		return -1;
	return 0;
}

int _wrmdir( const wchar_t *_path )
{
	if (!RemoveDirectoryW(_path))
		return -1;
	return 0;
}
