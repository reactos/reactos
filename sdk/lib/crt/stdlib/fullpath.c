/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/stdlib/fullpath.c
 * PURPOSE:     Gets the fullpathname
 * PROGRAMER:   Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
_TCHAR* _tfullpath(_TCHAR* absPath, const _TCHAR* relPath, size_t maxLength)
{
    _TCHAR* lpBuffer;
    _TCHAR* lpFilePart;
    DWORD retval;

    /* First check if entry relative path was given */
    if (!relPath || relPath[0] == 0)
    {
        /* If not, just try to return current dir */
        return _tgetcwd(absPath, (int)maxLength);
    }

    /* If no output buffer was given */
    if (!absPath)
    {
        /* Allocate one with fixed length */
        maxLength = MAX_PATH;
        lpBuffer = malloc(maxLength);
        if (!lpBuffer)
        {
            errno = ENOMEM;
            return NULL;
        }
    }
    else
    {
        lpBuffer = absPath;
    }

    /* Really get full path */
    retval = GetFullPathName(relPath, (DWORD)maxLength, lpBuffer, &lpFilePart);
    /* Check for failures */
    if (retval > maxLength)
    {
        /* Path too long, free (if needed) and return */
        if (!absPath)
        {
            free(lpBuffer);
        }

        errno = ERANGE;
        return NULL;
    }
    else if (!retval)
    {
        /* Other error, free (if needed), translate error, and return */
        if (!absPath)
        {
            free(lpBuffer);
        }

        _dosmaperr(GetLastError());
        return NULL;
    }

    /* Return buffer. Up to the caller to free if needed */
    return lpBuffer;
}
