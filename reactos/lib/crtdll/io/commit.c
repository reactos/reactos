#include <windows.h>
#include <crtdll/io.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

//fixme change this constant to _OCOMMIT
int _commode_dll = _IOCOMMIT;

int _commit(int _fd)
{
	if (! FlushFileBuffers(_get_osfhandle(_fd)) ) {
		__set_errno(EBADF);
		return -1;
	}

	return  0;
}
