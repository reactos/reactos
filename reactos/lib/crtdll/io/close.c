#include <windows.h>
#include <io.h>
#include <libc/file.h>

int	_close(int _fd)
{
	CloseHandle(_get_osfhandle(_fd));
	return __fileno_close(_fd);
		
}
