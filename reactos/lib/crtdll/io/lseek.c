#include <windows.h>
#include <io.h>
#include <libc/file.h>

#undef lseek
long lseek(int _fildes, long _offset, int _whence)
{
	return _lseek(_fildes,_offset,_whence);
}

long _lseek(int _fildes, long _offset, int _whence)
{
	//return _llseek(filehnd(_fildes),_offset,_whence);
}



