#include <crtdll/sys/types.h>
#include <crtdll/sys/stat.h>
#include <crtdll/fcntl.h>
#include <crtdll/io.h>


int _stat( const char *path, struct stat *buffer )
{
	int fd = _open(path,_O_RDONLY);
	int ret;
	
	ret = fstat(fd,buffer);
	_close(fd);

	return ret;

}
