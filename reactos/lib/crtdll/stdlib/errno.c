#include <windows.h>
#include <msvcrt/errno.h>

#undef errno
int errno;

#undef _doserrno
int _doserrno;

#undef _fpecode
int fpecode;

/*
 * @implemented
 */
int *_errno(void)
{
	return &errno;
}


int __set_errno (int error)
{
	errno = error;
	return error;
}



/*
 * @implemented
 */
int * __fpecode ( void )
{
        return &fpecode;
}

/*
 * @implemented
 */
int*	__doserrno(void)
{
	_doserrno = GetLastError();
	return &_doserrno;
}
