#include 	<sys/types.h>
#include	<sys/stat.h>
#include 	<fcntl.h>
#include	<io.h>

#undef stat
int stat( const char *path, struct stat *buffer )
{
	return _stat(path,buffer);
}

int _stat( const char *path, struct stat *buffer )
{
	int handle = _open(path,_O_RDONLY);
	int ret;
	
	ret = fstat(handle,buffer);
	_close(handle);

	return ret;

}