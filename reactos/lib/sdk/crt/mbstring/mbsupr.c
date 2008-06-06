/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsupr.c
 * PURPOSE:
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <precomp.h>
#include <mbstring.h>
#include <ctype.h>

unsigned int _mbbtoupper(unsigned int c)
{
	if (!_ismbblead(c) )
		return toupper(c);

	return c;
}

/*
 * @implemented
 */
unsigned int _mbctoupper(unsigned int c)
{
    return _ismbclower (c) ? c - 0x21 : c;
}

unsigned char *_mbset (unsigned char *string, int c)
{
    unsigned char *save = string;

    if (_MBIS16 (c)) {

	if (_MBLMASK (c) == 0) {
	    *string++ = '\0';
	    *string++ = '\0';
	}
	else {
	    *string++ = _MBGETH (c);
	    *string++ = _MBGETL (c);
	}

    }
    else {

	*string++ = c;

    }

    return save;
}

/*
 * @implemented
 */
unsigned char *_mbsupr (unsigned char *string)
{
    int c;
    unsigned char *save = string;

    while ((c = _mbsnextc (string))) {

	if (_MBIS16 (c) == 0)
	    c = toupper (c);

	_mbset (string, c);

	string = _mbsinc (string);

    }

    return save;
}
