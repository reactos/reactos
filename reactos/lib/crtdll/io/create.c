#include <crtdll/io.h>
#include <crtdll/fcntl.h>

#undef creat
int creat(const char *filename, int mode)
{
	return open(filename,_O_CREAT|_O_TRUNC,mode);
}
