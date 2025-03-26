/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsspn.c
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
size_t _mbsspn (const unsigned char *str1, const unsigned char *str2)
{
    int c;
    const unsigned char *save = str1;

    while ((c = _mbsnextc (str1))) {

	if (_mbschr (str2, c) == 0)
	    break;

	str1 = _mbsinc ((unsigned char *) str1);

    }

    return str1 - save;
}
