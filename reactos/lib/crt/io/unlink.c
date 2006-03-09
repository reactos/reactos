/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include "precomp.h"
#include <io.h>
#include <internal/file.h>

#define NDEBUG
#include <internal/debug.h>


/*
 * @implemented
 */
int _unlink(const char* filename)
{
    DPRINT("_unlink('%s')\n", filename);
    if (!DeleteFileA(filename)) {
		_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}
