/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */

#include <crtdll/wchar.h>

int wcscoll(const wchar_t *a1,const wchar_t *a2)
{
	/* FIXME: handle collates */
	return wcscmp(a1,a2);
}
