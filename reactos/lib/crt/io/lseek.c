#include "precomp.h"
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
long _lseek(int _fildes, long _offset, int _whence)
{
	DWORD newpos = SetFilePointer((HANDLE)filehnd(_fildes), _offset, NULL, _whence);
    if (newpos == INVALID_SET_FILE_POINTER) {
    	DWORD oserror = GetLastError();
    	if (oserror != 0) {
    		_dosmaperr(oserror);
    		return -1L;
    	}
    }
    return newpos;
}
