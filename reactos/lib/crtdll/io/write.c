/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/write.c
 * PURPOSE:     Writes to a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <io.h>
#include <windows.h>

int	write(int _fd, const void *_buf,int _nbyte)
{
	return _write(_fd,_buf,_nbyte);
}

size_t	_write(int _fd, const void *_buf, size_t _nbyte)
{
	size_t _wbyte;
	if ( !WriteFile(filehnd(_fd),_buf,_nbyte,&_wbyte,NULL) ) {
		printf("%d\n",GetLastError());
		return -1;
	}
	return _wbyte;
}
