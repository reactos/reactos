#include <windows.h>
#include <io.h>

unsigned int _commit(int _fd)
{
	return FlushFileBuffers(_get_osfhandle(_fd)) ? 0 : GetLastError();
}
