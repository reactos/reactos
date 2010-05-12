/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>


/*
 * @implemented
 */

int mbtowc (wchar_t *charptr, const char *address, size_t number)
{
    int bytes;

    if (address == 0)
	return 0;

    if ((bytes = mblen (address, number)) < 0)
	return bytes;

    if (charptr) {
	switch (bytes) {
	case 0:
	    if (number > 0) 
		*charptr = (wchar_t) '\0';
	    break;
	case 1:
	    *charptr = (wchar_t) ((unsigned char) address[0]);
	    break;
	case 2:
	    *charptr = (wchar_t) (((unsigned char) address[0] << 8)
				  | (unsigned char) address[1]);
	    break;
	}
    }

    return bytes;
}
