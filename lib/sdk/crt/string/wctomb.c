/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/wctomb.c
 * PURPOSE:
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
int wctomb (char *string, wchar_t widechar)
{
    int c1, c2;

    if (string == 0)
	return 0;

    if (widechar & 0xff00) {

	c1 = (widechar >> 8) & 0xff;
	c2 = (widechar & 0xff);

	if (_ismbblead (c1) == 0 || _ismbbtrail (c2) == 0)
	    return -1;

	*string++ = (char) c1;
	*string   = (char) c2;

	return 2;

    }
    else {

	*string = (char) widechar & 0xff;

	return 1;

    }
}

