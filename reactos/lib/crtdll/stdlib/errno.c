#include <windows.h>
#include <errno.h>

#undef errno
int errno;

#undef _doserrno
int _doserrno;

#undef _fpecode
int fpecode;

int *_errno(void)
{
	return &errno;
}


int __set_errno (int error)
{
	errno = error;
	return error;
}



int * __fpecode ( void )
{
        return &fpecode;
}

int*	__doserrno(void)
{
	_doserrno = GetLastError();
	return &_doserrno;
}
