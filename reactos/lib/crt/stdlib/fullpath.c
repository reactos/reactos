/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
_TCHAR* _tfullpath(_TCHAR* absPath, const _TCHAR* relPath, size_t maxLength)
{
    _TCHAR* lpFilePart;
    DWORD copied;

    if (!absPath)
    {
        maxLength = MAX_PATH;
        absPath = malloc(maxLength);
    }

    copied = GetFullPathName(relPath,maxLength,absPath,&lpFilePart);
    if (copied == 0 || copied > maxLength)
        return NULL;

    return absPath;
}
