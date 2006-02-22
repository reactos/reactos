#include <precomp.h>

/*
 * @implemented
 */
__int64 _filelengthi64(int _fd)
{
    DWORD lo_length, hi_length;

    lo_length = GetFileSize((HANDLE)_get_osfhandle(_fd), &hi_length);
    if (lo_length == INVALID_FILE_SIZE) {
    	DWORD oserror = GetLastError();
    	if (oserror != 0) {
    		_dosmaperr(oserror);
    		return (__int64)-1;
    	}
    }
    return((((__int64)hi_length) << 32) + lo_length);
}
