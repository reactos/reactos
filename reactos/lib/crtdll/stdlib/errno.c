/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <windows.h>
#include <errno.h>

int __crtdll_errno;

int *_errno(void)
{
	__crtdll_errno = GetLastError();
	return &__crtdll_errno;
}

int *__dos_errno(void)
{
	__crtdll_errno = GetLastError();
	return &__crtdll_errno;
}

