#include <windows.h>
#include <io.h>
#include <libc/file.h>


int _dup2( int _fd1, int _fd2 )
{
	return __fileno_dup2( _fd1, _fd2 );
}
