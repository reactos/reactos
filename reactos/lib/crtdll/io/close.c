#include <io.h>
#include <windows.h>
#include <libc/file.h>


int	close(int _fd)
{
	return _close(_fd);
}

int	_close(int _fd)
{
	CloseHandle(filehnd(_fd));
	return __fileno_close(_fd);
		
}