/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include "precomp.h"
#include <stdlib.h>
#include <tchar.h>


/*
 * @implemented
 */
_TCHAR* _tfullpath(_TCHAR* absPath, const _TCHAR* relPath, size_t maxLength)
{
    _TCHAR* lpFilePart;

    if (GetFullPathName(relPath,maxLength,absPath,&lpFilePart) == 0)
        return NULL;

    return absPath;
}
