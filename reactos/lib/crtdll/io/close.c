#include <io.h>
#include <windows.h>
#include <libc/file.h>


int	_close(int _fd)
{
	CloseHandle(_get_osfhandle(_fd));
	return __fileno_close(_fd);
		
}
