#include "precomp.h"
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
long _filelength(int _fd)
{
    DWORD len = GetFileSize(_get_osfhandle(_fd), NULL);
    if (len == INVALID_FILE_SIZE) {
    	DWORD oserror = GetLastError();
    	if (oserror != 0) {
    		_dosmaperr(oserror);
    		return -1L;
    	}
    }
    return (long)len;
}
