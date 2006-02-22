#include <precomp.h>

/*
 * @implemented
 */
long _filelength(int _fd)
{
    DWORD len = GetFileSize((HANDLE)_get_osfhandle(_fd), NULL);
    if (len == INVALID_FILE_SIZE) {
    	DWORD oserror = GetLastError();
    	if (oserror != 0) {
    		_dosmaperr(oserror);
    		return -1L;
    	}
    }
    return (long)len;
}
