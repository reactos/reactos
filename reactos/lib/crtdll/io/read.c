/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <crtdll/io.h>
#include <windows.h>

size_t	_read(int _fd, void *_buf, size_t _nbyte)
{
	size_t _rbyte;
	if ( !ReadFile(_get_osfhandle(_fd),_buf,_nbyte,&_rbyte,NULL) ) {
		return -1;
	}

	return _rbyte;
}
