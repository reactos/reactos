/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <windows.h>
#include <errno.h>

#undef _doserrno
int _doserrno;

#undef errno
unsigned int errno;


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
