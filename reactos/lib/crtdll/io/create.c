#include <io.h>
#include <fcntl.h>

#undef creat
int creat(const char *filename, int mode)
{
	return open(filename,_O_CREAT|_O_TRUNC,mode);
}
