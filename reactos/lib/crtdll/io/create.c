#include <io.h>
#include <fcntl.h>


int _creat(const char *filename, int mode)
{
	return _open(filename,_O_CREAT|_O_TRUNC,mode);
}
