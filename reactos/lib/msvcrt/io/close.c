#include <crtdll/io.h>
#include <windows.h>
#include <crtdll/internal/file.h>


int     _close(int _fd)
{
	if ( _fd == -1 )
		return -1;
        if ( CloseHandle(_get_osfhandle(_fd)) == FALSE )
        	return -1;
        return __fileno_close(_fd);
                
}
