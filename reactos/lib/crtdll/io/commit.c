#include <windows.h>
#include <io.h>

int _commit(int _fd)
{
	if (! FlushFileBuffers(_get_osfhandle(_fd)) )
		return -1;

	return  0;
}
