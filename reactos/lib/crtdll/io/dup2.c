#include <windows.h>
#include <io.h>

#undef dup2
int dup2( int handle1, int handle2 )
{
	return _dup2(handle1,handle2);
}


int _dup2( int handle1, int handle2 )
{
	return __fileno_dup2( handle1, handle2 );
}