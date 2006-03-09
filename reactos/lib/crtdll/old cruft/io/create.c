#include <msvcrt/io.h>
#include <msvcrt/fcntl.h>

/*
 * @implemented
 */
int _creat(const char *filename, int mode)
{
	return open(filename,_O_CREAT|_O_TRUNC,mode);
}
