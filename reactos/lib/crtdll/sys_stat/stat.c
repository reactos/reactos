#include <crtdll/sys/types.h>
#include <crtdll/sys/stat.h>
#include <crtdll/fcntl.h>
#include <crtdll/io.h>


int _stat( const char *path, struct stat *buffer )
{
	int handle = _open(path,_O_RDONLY);
	int ret;
	
	ret = fstat(handle,buffer);
	_close(handle);

	return ret;

}
