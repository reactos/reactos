/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */


#include <msvcrt/string.h>
#include <msvcrt/ctype.h>

/*
 * @implemented
 */
char *_strupr(char *x)
{
	char  *y=x;

	while (*y) {
		*y=toupper(*y);
		y++;
	}
	return x;
}
