/* $Id: dup.c,v 1.8 2003/07/11 17:25:16 royce Exp $ */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

// fixme change type of mode argument to mode_t

int __fileno_alloc(HANDLE hFile, int mode);
int __fileno_getmode(int _fd);

/*
 * @implemented
 */
int _dup(int handle)
{
    HANDLE hFile;
    int fd;
    hFile = _get_osfhandle(handle);
	fd = __fileno_alloc(hFile, __fileno_getmode(handle));
    return fd;
}
