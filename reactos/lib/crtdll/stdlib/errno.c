/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <errno.h>

#undef errno
int errno;
int _doserrno;


int _errno(void)
{
	return errno;
}

