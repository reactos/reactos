#include <windows.h>
#include <msvcrt/io.h>


int _locking(int _fd, int mode, long nbytes)
{
	long offset = _lseek(_fd, 0L, 1);
	if (!LockFile(_get_osfhandle(_fd),offset,0,nbytes,0))
		return -1;
 
	return 0;
}
