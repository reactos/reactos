#include <msvcrti.h>


int _mkdir( const char *_path )
{
	if (!CreateDirectoryA(_path,NULL))
		return -1;
	return 0;
}

int _wmkdir( const wchar_t *_path )
{
	if (!CreateDirectoryW(_path,NULL))
		return -1;
	return 0;
}
