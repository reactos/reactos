#include <windows.h>
#include <io.h>

#undef dup
int dup( int handle )
{
	return _dup(handle);
}

int _dup( int handle )
{
	return _open_osfhandle(filehnd(handle), 0666);
}