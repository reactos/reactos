#include <errno.h>

#undef errno
int errno;
#undef _doserrno
int _doserrno;


int *_errno(void)
{
	return &errno;
}

int *__doserrno(void)
{
	return &_doserrno;
}

