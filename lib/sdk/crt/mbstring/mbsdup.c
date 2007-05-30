/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsdup.c
 * PURPOSE:     Duplicates a multi byte string
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
		Modified from DJGPP strdup
 *              12/04/99: Created
 */

#include <precomp.h>
#include <mbstring.h>
#include <stdlib.h>

/*
 * @implemented
 */
unsigned char * _mbsdup(const unsigned char *_s)
{
	unsigned char *rv;
	if (_s == 0)
		return 0;
	rv = (unsigned char *)malloc(_mbslen(_s) + 1);
	if (rv == 0)
		return 0;
	_mbscpy(rv, _s);
	return rv;
}
