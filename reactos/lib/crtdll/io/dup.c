#include <windows.h>
#include <crtdll/io.h>


int _dup( int handle )
{
	return _open_osfhandle(_get_osfhandle(handle), 0666);
}
