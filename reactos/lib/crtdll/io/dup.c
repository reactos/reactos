#include <windows.h>
#include <crtdll/io.h>

// fixme change type of mode argument to mode_t

int __fileno_alloc(HANDLE hFile, int mode);
int __fileno_getmode(int _fd);

int _dup( int handle )
{
	return  __fileno_alloc(_get_osfhandle(handle), __fileno_getmode(handle));
}
