/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbscpn.c
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
size_t _mbscspn (const unsigned char *str1, const unsigned char *str2)
{
    int c;
    unsigned char *ptr;
    const unsigned char *save = str1;

    while ((c = _mbsnextc (str1))) {

	if ((ptr = _mbschr (str2, c)))
	    break;

	str1 = _mbsinc ((unsigned char *) str1);

    }

    return str1 - save;
}
