#include <windows.h>
#include <io.h>



int _dup( int _fd )
{
	return _open_osfhandle(_get_osfhandle(_fd), 0666);
}
