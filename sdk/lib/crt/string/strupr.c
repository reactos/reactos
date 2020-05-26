/*
 * The C RunTime DLL
 *
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */

#include <precomp.h>

/*
 * @implemented
 */
char * CDECL _strupr(char *x)
{
	char  *y=x;
	char ch, upper;

	while (*y) {
		ch = *y;
		upper = toupper(ch);
		if (ch != upper)
			*y = upper;
		y++;
	}
	return x;
}
