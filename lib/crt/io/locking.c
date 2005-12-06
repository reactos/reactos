#include <precomp.h>

/*
 * @implemented
 */
int _locking(int _fd, int mode, long nbytes)
{
	long offset = _lseek(_fd, 0L, 1);
	if (offset == -1L)
		return -1;
	if (!LockFile((HANDLE)_get_osfhandle(_fd),offset,0,nbytes,0)) {
  		_dosmaperr(GetLastError());
    	return -1;
    }

	return 0;
}
