#include <msvcrti.h>


int _chdir( const char *_path )
{
	if ( _path[1] == ':')
		_chdrive(tolower(_path[0] - 'a')+1);
	if ( !SetCurrentDirectoryA((char *)_path) )
		return -1;

	return 0;
}

int _wchdir( const wchar_t *_path )
{
	if ( _path[1] == L':')
		_chdrive(towlower(_path[0] - L'a')+1);
	if ( !SetCurrentDirectoryW((wchar_t *)_path) )
		return -1;

	return 0;
}
