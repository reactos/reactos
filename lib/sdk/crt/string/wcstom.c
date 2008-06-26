/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/wcstom.c
 * PURPOSE:
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted  from PROJECT C Library
 *
 */

#include <precomp.h>
#include <mbctype.h>

/*
 * @implemented
 */
size_t wcstombs (char *string, const wchar_t *widechar, size_t count)
{
    int n, bytes;
    int cnt = 0;

    for (n = 0; n < count; n++) {

	if ((bytes = wctomb (string, *widechar)) < 0)
	    return -1;

	if (*string == 0)
	    return cnt;

	widechar++;
	string += bytes;
	cnt += bytes;
    }

    return cnt;
}


