/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsdup.c
 * PURPOSE:     Duplicates a multi byte string
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
		Modified from DJGPP strdup
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>
#include <msvcrt/stdlib.h>

unsigned char * _mbsdup(const unsigned char *_s)
{
	char *rv;
	if (_s == 0)
		return 0;
	rv = (char *)malloc(_mbslen(_s) + 1);
	if (rv == 0)
		return 0;
	_mbscpy(rv, _s);
	return rv;
}
