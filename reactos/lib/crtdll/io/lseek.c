#include <windows.h>
#include <io.h>

off_t	_lseek(int _fd, off_t _offset, int _whence)
{
	return _llseek((HFILE)_get_osfhandle(_fd),_offset,_whence);
}



