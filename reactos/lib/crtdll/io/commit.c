#include <windows.h>
#include <io.h>
#include <errno.h>
#include <libc/file.h>

int _commit(int _fd)
{
	if (! FlushFileBuffers(_get_osfhandle(_fd)) ) {
		__set_errno(EBADF);
		return -1;
	}

	return  0;
}
