#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>


long _lseek(int _fildes, long _offset, int _whence)
{
	return _llseek((HFILE)filehnd(_fildes),_offset,_whence);
}
