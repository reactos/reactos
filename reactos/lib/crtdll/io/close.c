#include <crtdll/io.h>
#include <windows.h>
#include <crtdll/internal/file.h>


int	_close(int _fd)
{
	CloseHandle(_get_osfhandle(_fd));
	return __fileno_close(_fd);
		
}
