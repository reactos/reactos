/* $Id: dup.c,v 1.7 2002/11/24 18:42:13 robd Exp $ */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

// fixme change type of mode argument to mode_t

int __fileno_alloc(HANDLE hFile, int mode);
int __fileno_getmode(int _fd);

int _dup(int handle)
{
    HANDLE hFile;
    int fd;
    hFile = _get_osfhandle(handle);
	fd = __fileno_alloc(hFile, __fileno_getmode(handle));
    return fd;
}
