/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
int _wunlink(const wchar_t* filename)
{
    TRACE("_wunlink('%S')\n", filename);
    if (!DeleteFileW(filename)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}
