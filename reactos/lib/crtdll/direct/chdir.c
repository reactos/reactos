#include <crtdll/direct.h>
#include <windows.h>
#include <crtdll/ctype.h>

char _SetCurrentDirectory(char *dir);
int _GetCurrentDirectory(int count,char *buffer);

#undef chdir
int chdir( const char *_path )
{
	return _chdir(_path);
}

int _chdir( const char *_path )
{
	if ( _path[1] == ':')
		_chdrive(tolower(_path[0] - 'a')+1);
	if ( !SetCurrentDirectory((char *)_path) )
		return -1;

	return 0;
}



