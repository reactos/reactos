/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */

#include <crtdll/string.h>
#include <crtdll/ctype.h>

char * _strlwr(char *x)
{
	char  *y=x;

	while (*y) {
		*y=tolower(*y);
		y++;
	}
	return x;
}
