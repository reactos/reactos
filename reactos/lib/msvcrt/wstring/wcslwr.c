/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */
#include <msvcrti.h>


wchar_t * _wcslwr(wchar_t *x)
{
	wchar_t  *y=x;

	while (*y) {
		*y=towlower(*y);
		y++;
	}
	return x;
}
