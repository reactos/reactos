/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <io.h>
#include <windows.h>

size_t	read(int _fd, void *_buf, size_t _nbyte)
{
	return _read(_fd,_buf,_nbyte);
}
size_t	_read(int _fd, void *_buf, size_t _nbyte)
{
	size_t _rbyte;
	if ( !ReadFile(filehnd(_fd),_buf,_nbyte,&_rbyte,NULL) ) {
		printf("%d\n",GetLastError());
		return -1;
	}

	return _rbyte;
}
