#include <errno.h>

#undef errno
int errno;

#undef _doserrno
int _doserrno;

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
        return NULL;
}

int*	__doserrno(void)
{
	_doserrno = GetLastError();
	return &_doserrno;
}
