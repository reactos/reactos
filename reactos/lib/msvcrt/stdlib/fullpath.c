/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/stdlib.h>


/*
 * @implemented
 */
char* _fullpath(char* absPath, const char* relPath, size_t maxLength)
{
    char* lpFilePart;

    if (GetFullPathNameA(relPath,maxLength,absPath,&lpFilePart) == 0)
        return NULL;

    return absPath;
}
